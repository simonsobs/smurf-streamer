#ifndef _SMURFTRANSMITTER_H
#define _SMURFTRANSMITTER_H

#include <G3Frame.h>
#include <G3Writer.h>
#include <G3Data.h>
#include <G3Vector.h>
#include <G3Timestream.h>
#include <G3TimeStamp.h>
#include <G3Units.h>
#include <G3NetworkSender.h>
#include <G3EventBuilder.h>
#include <G3Logging.h>

#include "smurf/core/transmitters/BaseTransmitter.h"

namespace bp = boost::python;
namespace sct = smurf::core::transmitters;

using SmurfPacketRO = SmurfPacketManagerRO<ZeroCopyCreator>;
using SmurfPacketROPtr = SmurfPacketManagerROPtr<ZeroCopyCreator>;

/*
 *  Sample object passed to SmurfBuilder object
 */

class SmurfSampleFrameObject : public G3FrameObject{
public:
    uint32_t seq;
};

void printSmurfPacket(SmurfPacketROPtr sp);

class SmurfTransmitter : public sct::BaseTransmitter{
public:

    SmurfTransmitter(G3EventBuilderPtr builder);

    SmurfTransmitter(G3EventBuilderPtr builder, bool debug_data, bool debug_meta);

    ~SmurfTransmitter();

    // Set/Get the debug flags
    void       setDebugData(bool d) { debug_data_ = d;    };
    void       setDebugMeta(bool d) { debug_meta_ = d;    };
    const bool getDebugData()       { return debug_data_; };
    const bool getDebugMeta()       { return debug_meta_; };

    static void setup_python(){
        bp::class_< SmurfTransmitter,
                    std::shared_ptr<SmurfTransmitter>,
                    boost::noncopyable >
                    ("SmurfTransmitter", bp::init<G3EventBuilderPtr>())
            .def(bp::init<G3EventBuilderPtr, bool, bool>())

            .def("setDisable",     &SmurfTransmitter::setDisable)
            .def("getDisable",     &SmurfTransmitter::getDisable)
            .def("setDebugData",   &SmurfTransmitter::setDebugData)
            .def("getDebugData",   &SmurfTransmitter::getDebugData)
            .def("setDebugMeta",   &SmurfTransmitter::setDebugMeta)
            .def("getDebugMeta",   &SmurfTransmitter::getDebugMeta)
            .def("clearCnt",       &SmurfTransmitter::clearCnt)
            .def("getDataDropCnt", &SmurfTransmitter::getDataDropCnt)
            .def("getMetaDropCnt", &SmurfTransmitter::getMetaDropCnt)
            .def("getDataChannel", &SmurfTransmitter::getDataChannel)
            .def("getMetaChannel", &SmurfTransmitter::getMetaChannel)
        ;
        // bp::implicitly_convertible<std::shared_ptr<SmurfTransmitter>, ris::SlavePtr>();
    }

private:

    bool debug_data_; // Debug flag, for data
    bool debug_meta_; // Debug flag, for metadata

    G3EventBuilderPtr builder_;

    void dataTransmit(SmurfPacketROPtr packet);
    void metaTransmit(std::string cfg);

    SET_LOGGER("SmurfTransmitter")
};

typedef std::shared_ptr<SmurfTransmitter> SmurfTransmitterPtr;
typedef std::shared_ptr<const SmurfTransmitter> SmurfTransmitterConstPtr;

#endif
