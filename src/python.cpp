#include "SmurfStreamer.h"
#include "SmurfTransmitter.h"

namespace bp = boost::python;

BOOST_PYTHON_MODULE(sosmurfcore){
    // Makes sure rogue is imported...
    bp::import("rogue");

    PyEval_InitThreads();

    SmurfStreamer::setup_python();
    SmurfTransmitter::setup_python();

    printf("Loaded smurfcore\n");
}
