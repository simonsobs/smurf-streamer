#include "SmurfBuilder.h"

#include <G3Frame.h>
#include <G3Data.h>
#include <G3Timestream.h>
#include <G3Map.h>

#include <chrono>
#include <string>
#include <thread>
#include <inttypes.h>


namespace bp = boost::python;

SmurfBuilder::SmurfBuilder() :
    G3EventBuilder(MAX_DATASOURCE_QUEUE_SIZE),
    out_num_(0), num_channels_(0),
    agg_duration_(3), debug_(true)
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

    int nchans = read_stash_.front()->NChannels();

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
    ts_base.start = read_stash_.front()->time_;
    ts_base.stop = read_stash_.back()->time_;

    TimestampType timing_type = read_stash_.front()->timing_type_;        
    G3TimestreamMapPtr data_map = G3TimestreamMapPtr(new G3TimestreamMap);

    for (int i = 0; i < nchans; i++){
        G3TimestreamPtr ts = G3TimestreamPtr(new G3Timestream(ts_base));
        int sample=0;
        for (auto x = read_stash_.begin(); x != read_stash_.end(); x++, sample++){
            (*ts)[sample] = (*x)->Channels()[i];
        }

        data_map->insert(std::make_pair(chan_names_[i].c_str(), ts));
    }

    // Loads TES Biases
    G3TimestreamMapPtr tes_bias_map = G3TimestreamMapPtr(new G3TimestreamMap);
    for (int i = 0; i < N_TES_BIAS; i++){
        G3TimestreamPtr ts = G3TimestreamPtr(new G3Timestream(ts_base));
        int sample = 0;
        for (auto x = read_stash_.begin(); x != read_stash_.end(); x++, sample++){
            (*ts)[sample] = (*x)->getTESBias(i);
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

    read_stash_.clear();
    FrameOut(frame);

    auto end = std::chrono::system_clock::now();
    if (debug_){
        printf("Frame Out (%d channels, %d samples)\n", nchans, data_map->NSamples());
        auto flush_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        printf("Flushed in %d ms\n", flush_time);
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
        if (num_channels_ == 0){
            num_channels_ = data_pkt->NChannels();
        }
        else if (data_pkt->NChannels() != num_channels_){
            printf("num_channels has changed from %d to %d! Flushing stash...\n",
                    num_channels_, data_pkt->NChannels());
            FlushStash();
            num_channels_ = data_pkt->NChannels();
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
    ;

    bp::implicitly_convertible<SmurfBuilderPtr, G3ModulePtr>();
}
