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

#include "smurf/core/transmitters/BaseTransmitter.h"


namespace bp = boost::python;
namespace sct = smurf::core::transmitters;

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

    SmurfTransmitter();

    SmurfTransmitter(bool debug);

    void setDebug(bool debug){debug_ = debug;}
    bool getDebug(){return debug_;}

    static void setup_python(){
        bp::class_< SmurfTransmitter,
                    std::shared_ptr<SmurfTransmitter>,
                    boost::noncopyable >
                    ("SmurfTransmitter", bp::init<>())
            .def(bp::init<bool>())
            .def("setDebug",     &SmurfTransmitter::setDebug)
            .def("getDebug",     &SmurfTransmitter::getDebug)
            .def("setDisable",    &BaseTransmitter::setDisable)
            .def("getDisable",    &BaseTransmitter::getDisable)
            .def("clearCnt",      &BaseTransmitter::clearCnt)
            .def("getPktDropCnt", &BaseTransmitter::getPktDropCnt)
        ;
        bp::implicitly_convertible<std::shared_ptr<SmurfTransmitter>, ris::SlavePtr>();
    }

private:

    bool debug_;

    void transmit(SmurfPacketROPtr packet);
};

typedef std::shared_ptr<SmurfTransmitter> SmurfTransmitterPtr;
typedef std::shared_ptr<const SmurfTransmitter> SmurfTransmitterConstPtr;

#endif
