#include "SmurfBuilder.h"

#include <G3Frame.h>
#include <G3Data.h>
#include <G3Timestream.h>
#include <G3Map.h>
#include <G3Timesample.h>

#include <chrono>
#include <string>
#include <thread>
#include <inttypes.h>


namespace bp = boost::python;

SmurfBuilder::SmurfBuilder() :
    G3EventBuilder(MAX_DATASOURCE_QUEUE_SIZE),
    out_num_(0), num_channels_(0),
    agg_duration_(3), debug_(false)
{
    process_stash_thread_ = std::thread(ProcessStashThread, this);

    // creates list of TES bias keys
    char buff[10];
    for (int i = 0; i < N_TES_BIAS; i++){
        sprintf(buff, "bias%02d", i);
        bias_keys_.push_back(buff);
    }
}

SmurfBuilder::~SmurfBuilder(){
    running_ = false;
    process_stash_thread_.join();
}

void SmurfBuilder::ProcessStashThread(SmurfBuilder *builder){
    builder->running_ = true;

    while (builder->running_) {

        auto start = std::chrono::system_clock::now();
        builder->FlushStash();
        auto end = std::chrono::system_clock::now();

        auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        std::chrono::milliseconds iter_time((int)(1000 * builder->agg_duration_));
        std::chrono::milliseconds sleep_time = iter_time - diff;

        if (sleep_time.count() > 0)
            std::this_thread::sleep_for(sleep_time);
    }
}

void SmurfBuilder::FlushStash(){
    auto start = std::chrono::system_clock::now();
    std::lock_guard<std::mutex> read_lock(read_stash_lock_);

    // Swaps stashes
    {
        std::lock_guard<std::mutex> write_lock(write_stash_lock_);
        write_stash_.swap(read_stash_);
    }

    if (read_stash_.empty()){
        G3FramePtr frame = boost::make_shared<G3Frame>();
        frame->Put("sostream_flowcontrol", boost::make_shared<G3Int>(FC_ALIVE));
        frame->Put("time", boost::make_shared<G3Time>(G3Time::Now()));
        FrameOut(frame);
        return;
    }

    int nchans = read_stash_.front()->sp->getHeader()->getNumberChannels();
    int nsamps = read_stash_.size();

    // Creates channel names
    if (nchans > chan_names_.size()){
        char name[10];
         for (int i = chan_names_.size(); i < nchans; i++){
            sprintf(name, "r%04d", i);
            chan_names_.push_back(name);
        }
    }

    // Generic Timestream used to initialize real timestreams
    G3Timestream ts_base(read_stash_.size(), NAN);
    ts_base.start = read_stash_.front()->GetTime();
    ts_base.stop = read_stash_.back()->GetTime();

    TimestampType timing_type = read_stash_.front()->GetTimingParadigm();
    G3TimestreamMapPtr data_map = G3TimestreamMapPtr(new G3TimestreamMap);

    for (int i = 0; i < nchans; i++){
        G3TimestreamPtr ts = G3TimestreamPtr(new G3Timestream(ts_base));
        int sample=0;
        for (auto x = read_stash_.begin(); x != read_stash_.end(); x++, sample++){
            (*ts)[sample] = (*x)->sp->getData(i);
        }
        data_map->insert(std::make_pair(chan_names_[i].c_str(), ts));
    }

    // Loades Header Data
    std::vector<std::string> primary_keys = {
        "UnixTime", "FluxRampIncrement", "FluxRampOffset", "Counter0",
        "Counter1", "Counter2", "AveragingResetBits", "FrameCounter",
        "TESRelaySetting"
    };
    std::vector<G3VectorIntPtr> primary_vec;
    G3VectorTime sample_times = G3VectorTime(nsamps);
    for (auto key : primary_keys)
        primary_vec.push_back(G3VectorIntPtr(new G3VectorInt(nsamps, 0)));

    boost::shared_ptr<G3TimesampleMap> primary_map = boost::make_shared<G3TimesampleMap>();

    int sample = 0;
    for (auto x = read_stash_.begin(); x != read_stash_.end(); x++, sample++){
        auto hdr = (*x)->sp->getHeader();

        sample_times[sample] = (*x)->GetTime();

        (*primary_vec[0])[sample] = hdr->getUnixTime();
        (*primary_vec[1])[sample] = hdr->getFluxRampIncrement();
        (*primary_vec[2])[sample] = hdr->getFluxRampOffset();
        (*primary_vec[3])[sample] = hdr->getCounter0();
        (*primary_vec[4])[sample] = hdr->getCounter1();
        (*primary_vec[5])[sample] = hdr->getCounter2();
        (*primary_vec[6])[sample] = hdr->getAveragingResetBits();
        (*primary_vec[7])[sample] = hdr->getFrameCounter();
        (*primary_vec[8])[sample] = hdr->getTESRelaySetting();
    }

    primary_map->times = sample_times;

    int i = 0;
    for (auto key : primary_keys){
        primary_map->insert(std::make_pair(key, primary_vec[i]));
        i++;
    }

    if (!primary_map->Check()){
        printf("Primary Timesample Map failed it's check!\n");
    }

    // Slow primary map
    G3MapIntPtr slow_primary_map = G3MapIntPtr(new G3MapInt);
    auto hdr = read_stash_.front()->sp->getHeader();
    slow_primary_map->insert(std::make_pair("Version", hdr->getVersion()));
    slow_primary_map->insert(std::make_pair("CrateID", hdr->getCrateID()));
    slow_primary_map->insert(std::make_pair("SlotNumber", hdr->getSlotNumber()));
    slow_primary_map->insert(std::make_pair("TimingConfiguration", hdr->getTimingConfiguration()));

    // Loads TES Biases
    G3TimestreamMapPtr tes_bias_map = G3TimestreamMapPtr(new G3TimestreamMap);
    for (int i = 0; i < N_TES_BIAS; i++){
        G3TimestreamPtr ts = G3TimestreamPtr(new G3Timestream(ts_base));
        int sample = 0;
        for (auto x = read_stash_.begin(); x != read_stash_.end(); x++, sample++){
            (*ts)[sample] = (*x)->sp->getHeader()->getTESBias(i);
        }
        tes_bias_map->insert(std::make_pair(bias_keys_[i].c_str(), ts));
    }

    G3FramePtr frame = boost::make_shared<G3Frame>(G3Frame::Scan);
    frame->Put("time", boost::make_shared<G3Time>(G3Time::Now()));
    frame->Put("timing_paradigm",
               boost::make_shared<G3String>(TimestampTypeStrings[timing_type])
    );
    frame->Put("data", data_map);
    frame->Put("tes_biases", tes_bias_map);
    frame->Put("num_samples", boost::make_shared<G3Int>(data_map->NSamples()));

    frame->Put("primary", primary_map);
    frame->Put("slow_primary", slow_primary_map);

    read_stash_.clear();
    FrameOut(frame);

    auto end = std::chrono::system_clock::now();
    if (debug_){
        printf("Frame Out (%d channels, %lu samples)\n", nchans, data_map->NSamples());
        long flush_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        printf("Flushed in %ld ms\n", flush_time);
        printf("%lu elements in queue...\n", queue_.size());
    }
}

void SmurfBuilder::ProcessNewData(){
    G3FrameObjectConstPtr pkt;
    G3TimeStamp ts;

    {
        std::lock_guard<std::mutex> lock(queue_lock_);
        ts = queue_.front().first;
        pkt = queue_.front().second;
        queue_.pop_front();
    }

    SmurfSampleConstPtr data_pkt;
    StatusSampleConstPtr status_pkt;

    if (status_pkt = boost::dynamic_pointer_cast<const StatusSample>(pkt)){

        G3FramePtr frame(boost::make_shared<G3Frame>(G3Frame::Wiring));
        frame->Put("time", boost::make_shared<G3Time>(G3Time::Now()));
        frame->Put("status", boost::make_shared<G3String>(status_pkt->status_));

        FrameOut(frame);
    }
    else if (data_pkt = boost::dynamic_pointer_cast<const SmurfSample>(pkt)){
        auto hdr = data_pkt->sp->getHeader();
        if (num_channels_ == 0){
            num_channels_ = hdr->getNumberChannels();
        }
        else if (hdr->getNumberChannels() != num_channels_){
            printf("num_channels has changed from %d to %d! Flushing stash...\n",
                    num_channels_, hdr->getNumberChannels());
            FlushStash();
            num_channels_ = hdr->getNumberChannels();
        }

        std::lock_guard<std::mutex> lock(write_stash_lock_);
        write_stash_.push_back(data_pkt);
    }
}

void SmurfBuilder::setup_python(){

    bp::class_< SmurfBuilder, bp::bases<G3EventBuilder>,
                SmurfBuilderPtr, boost::noncopyable>
    ("SmurfBuilder",
    "Takes transmitted smurf-packets and puts them into G3Frames, starting off "
    "the stream's G3Pipeline ",
    bp::init<>())
    .def("GetAggDuration", &SmurfBuilder::GetAggDuration)
    .def("SetAggDuration", &SmurfBuilder::SetAggDuration)
    .def("getDebug", &SmurfBuilder::getDebug)
    .def("setDebug", &SmurfBuilder::setDebug)
    ;

    bp::implicitly_convertible<SmurfBuilderPtr, G3ModulePtr>();
}
