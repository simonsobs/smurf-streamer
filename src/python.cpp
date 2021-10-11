#include "SmurfTransmitter.h"
#include "SmurfBuilder.h"

#define PY_ARRAY_UNIQUE_SYMBOL Py_Array_API_SO3G
#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
#include <numpy/arrayobject.h>
namespace bp = boost::python;

static void* _sosmurf_import_array() {
    import_array();
    return NULL;
}

BOOST_PYTHON_MODULE(sosmurfcore){
    // _so3g_import_array();
    
    bp::import("rogue");
    bp::import("spt3g.core");

    PyEval_InitThreads();
    _sosmurf_import_array();

    SmurfTransmitter::setup_python();
    SmurfBuilder::setup_python();

    printf("Loaded smurfcore\n");
}
