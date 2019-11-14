#include "SmurfBuilder.h"

#include <G3Frame.h>

SmurfBuilder::SmurfBuilder() : G3EventBuilder(MAX_DATASOURCE_QUEUE_SIZE) {}
SmurfBuilder::~SmurfBuilder(){}

namespace bp = boost::python;

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
    // This should be pretty simple since we only have one data source and we can
    // assume packets are coming in order (hopefully?)

    G3FramePtr frame = boost::make_shared<G3Frame>(G3Frame::Timepoint);
    frame->Put("Sample", pkt);

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
