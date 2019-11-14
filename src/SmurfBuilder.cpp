#include "SmurfBuilder.h"

SmurfBuilder::SmurfBuilder() : G3EventBuilder(MAX_DATASOURCE_QUEUE_SIZE) {}
SmurfBuilder::~SmurfBuilder(){}

namespace bp = boost::python;

void SmurfBuilder::ProcessNewData(){

    SmurfSampleConstPtr pkt;
    {
        std::lock_guard<std::mutex> lock(queue_lock_);
        pkt = boost::dynamic_pointer_cast<const SmurfSample>(
            queue_.front().second);
        queue_.pop_front();
    }

    printf("Sample received in Builder!!\n");
    fflush(stdout);
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
