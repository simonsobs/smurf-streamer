#include "SmurfTransmitter.h"
#include "SmurfBuilder.h"

namespace bp = boost::python;

using SmurfPacketRO = SmurfPacketManagerRO<ZeroCopyCreator>;
using SmurfPacketROPtr = SmurfPacketManagerROPtr<ZeroCopyCreator>;

BOOST_PYTHON_MODULE(sosmurfcore){
    bp::import("rogue");
    bp::import("spt3g.core");

    PyEval_InitThreads();

    SmurfTransmitter::setup_python();
    SmurfBuilder::setup_python();

    printf("Loaded smurfcore\n");
}
