#include "SmurfTransmitter.h"
#include "SmurfBuilder.h"
#include "so3g_numpy.h"

namespace bp = boost::python;

static void* _sosmurf_import_array() {
    import_array();
    return NULL;
}

BOOST_PYTHON_MODULE(sosmurfcore){
    bp::import("rogue");
    bp::import("spt3g.core");

    PyEval_InitThreads();
    _sosmurf_import_array();

    SmurfTransmitter::setup_python();
    SmurfBuilder::setup_python();

    printf("Loaded smurfcore\n");
}
