#include "SmurfStreamer.h"
#include "SmurfTransmitter.h"
#include "SmurfBuilder.h"

namespace bp = boost::python;

BOOST_PYTHON_MODULE(sosmurfcore){
    bp::import("rogue");
    bp::import("spt3g.core");

    PyEval_InitThreads();

    SmurfStreamer::setup_python();
    SmurfTransmitter::setup_python();
    SmurfBuilder::setup_python();

    printf("Loaded smurfcore\n");
}
