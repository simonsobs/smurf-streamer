#define NO_IMPORT_ARRAY
#include "SmurfBuilder.h"

#include <G3Frame.h>
#include <G3Data.h>
#include <G3Timestream.h>
#include <G3Map.h>
#include <G3Timesample.h>

#include <pybindings.h>
#include <boost/python.hpp>
#include <container_pybindings.h>

#include <chrono>
#include <string>
#include <thread>
#include <inttypes.h>
#include <G3SuperTimestream.h>


namespace bp = boost::python;

SmurfBuilder::SmurfBuilder() :
    G3EventBuilder(MAX_DATASOURCE_QUEUE_SIZE),
    out_num_(0), num_channels_(0),
    agg_duration_(3), debug_(false), encode_timestreams_(false),
    dropped_frames_(0)
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

G3FramePtr SmurfBuilder::FrameFromSamples(
        std::deque<SmurfSampleConstPtr>::iterator start,
        std::deque<SmurfSampleConstPtr>::iterator stop){
    // Time points used for debugging purposes
    std::chrono::time_point<std::chrono::system_clock>
        start_time, stop_time, alloc_start, copy_start, frame_start,
        compression_start, gil_ensure_start, gil_ensure_stop;
    start_time = std::chrono::system_clock::now();

    int nchans = (*start)->sp->getHeader()->getNumberChannels();
    int nsamps = stop - start;

    // Creates channel names if needed
    if (nchans > chan_names_.size()){
        char name[10];
         for (int i = chan_names_.size(); i < nchans; i++){
            sprintf(name, "r%04d", i);
            chan_names_.push_back(name);
        }
    }

    if (debug_)
        alloc_start = std::chrono::system_clock::now();


    // Initialize detector timestreams
    int32_t* data_buffer= (int32_t*) calloc(nchans * nsamps, sizeof(int32_t));
 
    int data_shape[2] = {nchans, nsamps};
    auto data_ts = G3SuperTimestreamPtr(new G3SuperTimestream());
    data_ts->names = G3VectorString();
    for (int i = 0; i < nchans; i++){
        data_ts->names.push_back(chan_names_[i]);
    }

    int32_t tes_buffer[N_TES_BIAS*nsamps];
    int tes_shape[2] = {N_TES_BIAS, nsamps};
    auto tes_ts = G3SuperTimestreamPtr(new G3SuperTimestream());
    tes_ts->names = G3VectorString();
    for (int i = 0; i < N_TES_BIAS; i++){
        tes_ts->names.push_back(bias_keys_[i]);
    }

    std::vector<std::string> primary_keys = {
        "UnixTime", "FluxRampIncrement", "FluxRampOffset", "Counter0",
        "Counter1", "Counter2", "TimingBits", "FrameCounter",
        "TESRelaySetting"
    };
    int num_keys = primary_keys.size();
    int64_t primary_buffer[num_keys*nsamps];
    int primary_shape[2] = {num_keys, nsamps};
    auto primary_ts = G3SuperTimestreamPtr(new G3SuperTimestream());
    primary_ts->names = G3VectorString();
    for (auto name : primary_keys)
        primary_ts->names.push_back(name);
    
    G3VectorTime sample_times = G3VectorTime(nsamps);

    if (debug_)
        copy_start = std::chrono::system_clock::now();


    TimestampType timing_type = Timing_LowPrecision;

    // Read data in to G3 Objects
    int sample = 0;
    for (auto it = start; it != stop; it++, sample++){
        sample_times[sample] = (*it)->GetTime();

        auto hdr = (*it)->sp->getHeader();
        primary_buffer[sample + 0 * nsamps] = hdr->getUnixTime();
        primary_buffer[sample + 1 * nsamps] = hdr->getFluxRampIncrement();
        primary_buffer[sample + 2 * nsamps] = hdr->getFluxRampOffset();
        primary_buffer[sample + 3 * nsamps] = hdr->getCounter0();
        primary_buffer[sample + 4 * nsamps] = hdr->getCounter1();
        primary_buffer[sample + 5 * nsamps] = hdr->getCounter2();
        primary_buffer[sample + 6 * nsamps] = hdr->getTimingBits();
        primary_buffer[sample + 7 * nsamps] = hdr->getFrameCounter();
        primary_buffer[sample + 8 * nsamps] = hdr->getTESRelaySetting();

        for (int i = 0; i < N_TES_BIAS; i++)
            tes_buffer[sample + i * nsamps] = hdr->getTESBias(i);

        for (int i = 0; i < nchans; i++){
            data_buffer[sample + i * nsamps] = (*it)->sp->getData(i);
        }

        if (hdr->getCounter2() != 0){
            timing_type = Timing_HighPrecision;
        }
    }

    if (debug_)
        gil_ensure_start = std::chrono::system_clock::now();

    PyGILState_STATE gstate;
    gstate = PyGILState_Ensure();

    if (debug_)
        gil_ensure_stop = std::chrono::system_clock::now();

    data_ts->times = sample_times;
    data_ts->SetDataFromBuffer((void*)data_buffer, 2, data_shape, NPY_INT32,
            std::pair<int,int>(0, nsamps));
    data_ts->Options(
        enable_compression_, flac_level_, bz2_work_factor_, data_encode_algo_,
        time_encode_algo_
    );

    tes_ts->times = G3VectorTime(sample_times);
    tes_ts->SetDataFromBuffer((void*)tes_buffer, 2, tes_shape, NPY_INT32,
            std::pair<int,int>(0, nsamps));
    tes_ts->Options(
        enable_compression_, flac_level_, bz2_work_factor_, tes_bias_encode_algo_,
        time_encode_algo_
    );

    primary_ts->times = G3VectorTime(sample_times);
    primary_ts->SetDataFromBuffer((void*)primary_buffer, 2, primary_shape,
            NPY_INT64, std::pair<int,int>(0, nsamps));
    primary_ts->Options(
        enable_compression_, flac_level_, bz2_work_factor_, primary_encode_algo_,
        time_encode_algo_
    );

    if (debug_)
        compression_start = std::chrono::system_clock::now();

    if (encode_timestreams_){
        data_ts->Encode();
        tes_ts->Encode();
        primary_ts->Encode();
    }

    PyGILState_Release(gstate);
    free(data_buffer);

    // Slow primary map
    G3MapIntPtr slow_primary_map = G3MapIntPtr(new G3MapInt);
    auto hdr = (*start)->sp->getHeader();
    slow_primary_map->insert(std::make_pair("Version", hdr->getVersion()));
    slow_primary_map->insert(std::make_pair("CrateID", hdr->getCrateID()));
    slow_primary_map->insert(std::make_pair("SlotNumber", hdr->getSlotNumber()));

    if (debug_)
        frame_start = std::chrono::system_clock::now();

    // Create and return G3Frame
    G3FramePtr frame = boost::make_shared<G3Frame>(G3Frame::Scan);
    frame->Put("time", boost::make_shared<G3Time>(G3Time::Now()));
    frame->Put("timing_paradigm",
               boost::make_shared<G3String>(TimestampTypeStrings[timing_type])
    );
    frame->Put("data", data_ts);
    frame->Put("tes_biases", tes_ts);
    frame->Put("num_samples", boost::make_shared<G3Int>(nsamps));

    frame->Put("primary", primary_ts);
    frame->Put("slow_primary", slow_primary_map);

    stop_time = std::chrono::system_clock::now();
    if (debug_){
        printf("Frame Out (%d channels, %d samples)\n", nchans, nsamps);
        printf("Total Time: %ld ms\n", (stop_time - start_time).count()/1000000);
        printf(" - Alloc Time: %ld ms\n", (copy_start - alloc_start).count()/1000000);
        printf(" - Copy Time: %ld ms\n", (compression_start - copy_start).count()/1000000);
        printf(" - GIL Wait Time: %ld ms\n", (gil_ensure_stop - gil_ensure_start).count()/1000000);
        printf(" - Compression Time: %ld ms\n", (frame_start - compression_start).count()/1000000);
        printf(" - Frame Time: %ld ms\n", (stop_time - frame_start).count()/1000000);
        printf("%lu elements in queue...\n", queue_.size());
        printf("dropped frames: %lu\n", dropped_frames_);
        printf(
            "Encoding Options:\n"
            " - enabled: %d\n"
            " - data_algo: %d\n"
            " - primary_algo: %d\n"
            " - tes_algo: %d\n"
            " - flac_level: %d\n"
            " - BZ2_WorkFactor: %d\n",
            enable_compression_, data_encode_algo_, primary_encode_algo_,
            time_encode_algo_, flac_level_, bz2_work_factor_
        );
    }

    return frame;
}

void SmurfBuilder::FlushStash(){
    // Swaps stashes
    std::lock_guard<std::mutex> read_lock(read_stash_lock_);
    {
        std::lock_guard<std::mutex> write_lock(write_stash_lock_);
        write_stash_.swap(read_stash_);
        queue_size_ = 0;
    }

    if (read_stash_.empty()){
        G3FramePtr frame = boost::make_shared<G3Frame>();
        frame->Put("sostream_flowcontrol", boost::make_shared<G3Int>(FC_ALIVE));
        frame->Put("time", boost::make_shared<G3Time>(G3Time::Now()));
        FrameOut(frame);
        return;
    }

    auto start = read_stash_.begin();
    auto stop = read_stash_.begin();
    int nchans = (*start)->sp->getHeader()->getNumberChannels();
    while(true){
        stop += 1;
        if (stop == read_stash_.end()){
            FrameOut(FrameFromSamples(start, stop));
            break;
        }
        int stop_nchans = (*stop)->sp->getHeader()->getNumberChannels();
        if (stop_nchans != nchans){
            printf("NumChannels has changed from %d to %d!!", nchans, stop_nchans);
            FrameOut(FrameFromSamples(start, stop));
            start = stop;
            nchans = stop_nchans;
        }
    }
    read_stash_.clear();
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
        std::lock_guard<std::mutex> lock(write_stash_lock_);
        if (queue_size_ < MAX_BUILDER_QUEUE_SIZE){
            write_stash_.push_back(data_pkt);
            queue_size_ += data_pkt->sp->getHeader()->getNumberChannels();
        }
        else{
            dropped_frames_++;
        }
    }
}

size_t SmurfBuilder::getDroppedFrames(){
    return dropped_frames_;
}

void SmurfBuilder::setDataEncodeAlgo(int algo){
    data_encode_algo_ = algo;
}

int SmurfBuilder::getDataEncodeAlgo() const{
    return data_encode_algo_;
}

void SmurfBuilder::setPrimaryEncodeAlgo(int algo){
    primary_encode_algo_ = algo;
}

int SmurfBuilder::getPrimaryEncodeAlgo() const{
    return primary_encode_algo_;
}

void SmurfBuilder::setTesBiasEncodeAlgo(int algo){
    tes_bias_encode_algo_ = algo;
}

int SmurfBuilder::getTesBiasEncodeAlgo() const{
    return tes_bias_encode_algo_;
}

void SmurfBuilder::setTimeEncodeAlgo(int algo){
    time_encode_algo_ = algo;
}

int SmurfBuilder::getTimeEncodeAlgo() const{
    return time_encode_algo_;
}

int SmurfBuilder::getEnableCompression() const{
    return enable_compression_;
}

void SmurfBuilder::setEnableCompression(int enable){
    enable_compression_ = enable;
}

int SmurfBuilder::getBz2WorkFactor() const{
    return bz2_work_factor_;
}

void SmurfBuilder::setBz2WorkFactor(int bz2_work_factor){
    bz2_work_factor_ = bz2_work_factor;
}

int SmurfBuilder::getFlacLevel() const{
    return flac_level_;
}

void SmurfBuilder::setFlacLevel(int flac_level){
    flac_level_ = flac_level;
}
// Assist with testing the pure C++ interface
static
G3SuperTimestreamPtr test_cxx_interface(int nsamps, int first, int second)
{
	int shape[2] = {3, nsamps};
	int typenum = NPY_INT32;
	int32_t buf[shape[0] * shape[1]] = {0};

	auto ts = G3SuperTimestreamPtr(new G3SuperTimestream());
	const char *chans[] = {"a", "b", "c"};
	ts->names = G3VectorString(chans, std::end(chans));
	ts->times = G3VectorTime();
	for (int i=first; i<second; i++) {
		ts->times.push_back(G3Time::Now());
		buf[i] = 77;
	}
	ts->SetDataFromBuffer((void*)buf, 2, shape, typenum,
			      std::pair<int,int>(first, second));

	return ts;
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
    .def("getEncode", &SmurfBuilder::getEncode)
    .def("setEncode", &SmurfBuilder::setEncode)
    .def("getDataEncodeAlgo", &SmurfBuilder::getDataEncodeAlgo)
    .def("setDataEncodeAlgo", &SmurfBuilder::setDataEncodeAlgo)
    .def("getPrimaryEncodeAlgo", &SmurfBuilder::getPrimaryEncodeAlgo)
    .def("setPrimaryEncodeAlgo", &SmurfBuilder::setPrimaryEncodeAlgo)
    .def("getTesBiasEncodeAlgo", &SmurfBuilder::getTesBiasEncodeAlgo)
    .def("setTesBiasEncodeAlgo", &SmurfBuilder::setTesBiasEncodeAlgo)
    .def("getTimeEncodeAlgo", &SmurfBuilder::getTimeEncodeAlgo)
    .def("setTimeEncodeAlgo", &SmurfBuilder::setTimeEncodeAlgo)
    .def("getEnableCompression", &SmurfBuilder::getEnableCompression)
    .def("setEnableCompression", &SmurfBuilder::setEnableCompression)
    .def("getBz2WorkFactor", &SmurfBuilder::getBz2WorkFactor)
    .def("setBz2WorkFactor", &SmurfBuilder::setBz2WorkFactor)
    .def("getFlacLevel", &SmurfBuilder::getFlacLevel)
    .def("setFlacLevel", &SmurfBuilder::setFlacLevel)
    .def("getDroppedFrames", &SmurfBuilder::getDroppedFrames)
    ;
    bp::def("build_g3super", test_cxx_interface);

    bp::implicitly_convertible<SmurfBuilderPtr, G3ModulePtr>();

}
