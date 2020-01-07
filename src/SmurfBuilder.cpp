#include "SmurfBuilder.h"

#include <G3Frame.h>
#include <G3Data.h>
#include <G3Timestream.h>
#include <G3Map.h>

#include <chrono>
#include <string>
#include <thread>

namespace bp = boost::python;

SmurfBuilder::SmurfBuilder() :
    G3EventBuilder(MAX_DATASOURCE_QUEUE_SIZE),
    out_num_(0), num_channels_(0),
    agg_duration_(3)
{
    process_stash_thread_ = std::thread(ProcessStashThread, this);
}

SmurfBuilder::~SmurfBuilder(){
    running_ = false;
    process_stash_thread_.join();
}

void SmurfBuilder::ProcessStashThread(SmurfBuilder *builder){
    builder->running_ = true;
    while (builder->running_) {
        std::this_thread::sleep_for(
            std::chrono::milliseconds(int(1000 * builder->agg_duration_))
        );
        builder->SwapStash();
        builder->FlushReadStash();
    }
}

void SmurfBuilder::SwapStash(){
    std::lock_guard<std::mutex> read_lock(read_stash_lock_);
    std::lock_guard<std::mutex> write_lock(write_stash_lock_);
    write_stash_.swap(read_stash_);
}

void SmurfBuilder::FlushReadStash(){
    std::lock_guard<std::mutex> read_lock(read_stash_lock_);

    if (read_stash_.empty())
        return;

    int nchans = read_stash_.front()->NChannels();

    // Creates channel names
    if (nchans > chan_names_.size()){
        char name[10];
         for (int i = chan_names_.size(); i < nchans; i++){
            sprintf(name, "r%04d", i);
            chan_names_.push_back(name);
        }
    }

    G3Timestream ts_base(read_stash_.size(), NAN);
    ts_base.start = read_stash_.front()->Timestamp;
    ts_base.stop = read_stash_.back()->Timestamp;

    // Initialize timestream map
    G3TimestreamMapPtr data_map = G3TimestreamMapPtr(new G3TimestreamMap);
    for (int i = 0; i < nchans; i++){
        data_map->insert(std::make_pair(
            chan_names_[i].c_str(), G3TimestreamPtr(new G3Timestream(ts_base))
        ));
    }
    G3TimestreamMapPtr tes_bias_map = G3TimestreamMapPtr(new G3TimestreamMap);
    for (int i = 0; i < 16; i++){
        tes_bias_map->insert(std::make_pair(
            std::to_string(i).c_str(), G3TimestreamPtr(new G3Timestream(ts_base))
        ));
    }

    // Insert sample data into timestreams
    int sample = 0;
    for (auto x = read_stash_.begin(); x != read_stash_.end(); x++, sample++){
        auto chan_data = (*x)->Channels();
        for (int i = 0; i < nchans; i++){
            (*((*data_map)[chan_names_[i]]))[sample] = chan_data[i];
        }

        for (int i = 0; i < 16; i++){
            std::string tes_name = std::to_string(i);
            (*((*tes_bias_map)[tes_name]))[sample] = (*x)->getTESBias(i);
        }
    }

    G3FramePtr frame = boost::make_shared<G3Frame>(G3Frame::Scan);
    frame->Put("frame_num", boost::make_shared<G3Int>(out_num_++));
    frame->Put("data", data_map);
    frame->Put("tes_biases", tes_bias_map);
    frame->Put("num_samples", boost::make_shared<G3Int>(sample));

    read_stash_.clear();

    FrameOut(frame);
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

        G3FramePtr frame(boost::make_shared<G3Frame>(G3Frame::Observation));
        frame->Put("frame_num", boost::make_shared<G3Int>(out_num_++));
        frame->Put("status", boost::make_shared<G3String>(status_pkt->status_));

        FrameOut(frame);
    }
    else if (data_pkt = boost::dynamic_pointer_cast<const SmurfSample>(pkt)){

        if (num_channels_ == 0){
            num_channels_ = data_pkt->NChannels();
        }
        else if (data_pkt->NChannels() != num_channels_){
            SwapStash();
            FlushReadStash();
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
    ;

    bp::implicitly_convertible<SmurfBuilderPtr, G3ModulePtr>();
}
