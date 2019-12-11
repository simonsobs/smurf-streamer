#include "SmurfBuilder.h"

#include <G3Frame.h>
#include <G3Data.h>
#include <G3Timestream.h>

#include <chrono>

namespace bp = boost::python;

SmurfBuilder::SmurfBuilder() :
    G3EventBuilder(MAX_DATASOURCE_QUEUE_SIZE), out_num_(0),
    agg_duration_(5*G3Units::sec)
     {
         char name[10];
         for (int i = 0; i < 2000; i++){
             sprintf(name, "r%04d", i);
             chan_names_.push_back(name);
         }
     }


SmurfBuilder::~SmurfBuilder(){}


void SmurfBuilder::ProcessNewData(){
    SmurfSampleConstPtr pkt;
    G3TimeStamp ts;
    {
        std::lock_guard<std::mutex> lock(queue_lock_);
        ts = queue_.front().first;
        pkt = boost::dynamic_pointer_cast<const SmurfSample>(
            queue_.front().second);
        queue_.pop_front();
    }

    // Aggregates new packet
    if (stash_.empty())
        stash_start_time_ = pkt->Timestamp;

    stash_.push_back(pkt);
    if ((pkt->Timestamp.time - stash_start_time_) < agg_duration_)
        return;

    // This code should run once every agg_duration_
    int nchans = stash_.front()->NChannels();

    G3Timestream ts_base(stash_.size(), NAN);
    ts_base.start = stash_.front()->Timestamp;
    ts_base.stop = stash_.back()->Timestamp;

    // Initialize timestream map
    G3TimestreamMapPtr data_map = G3TimestreamMapPtr(new G3TimestreamMap);
    for (int i = 0; i < nchans; i++){
        data_map->insert(std::make_pair(
            chan_names_[i], G3TimestreamPtr(new G3Timestream(ts_base))
        ));
    }

    // Insert sample data into timestreams
    int sample = 0;
    for (auto x = stash_.begin(); x != stash_.end(); x++, sample++){
        auto chan_data = (*x)->Channels();
        for (int i = 0; i < nchans; i++){
            (*((*data_map)[chan_names_[i]]))[sample] = chan_data[i];
        }
    }

    G3FramePtr frame = boost::make_shared<G3Frame>(G3Frame::Scan);
    frame->Put("frame_num", boost::make_shared<G3Int>(out_num_++));
    frame->Put("data", data_map);
    frame->Put("num_samples", boost::make_shared<G3Int>(sample));

    stash_.clear();

    // // This should be pretty simple since we only have one data source and we can
    // // assume packets are coming in order (hopefully?)
    // G3FramePtr frame = boost::make_shared<G3Frame>(G3Frame::Timepoint);
    // frame->Put("Sample", pkt);
    // frame->Put("out_num", boost::make_shared<G3Int>(out_num_++));
    FrameOut(frame);
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
