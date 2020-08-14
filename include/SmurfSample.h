#ifndef SMURFSAMPLE_H
#define SMURFSAMPLE_H

#include <G3Frame.h>
#include <G3TimeStamp.h>
#include <vector>

#include <pybindings.h>
#include <serialization.h>

#include "smurf/core/common/SmurfPacket.h"

#define N_TES_BIAS 16

class StatusSample : public G3FrameObject{
public:
    StatusSample(): G3FrameObject(), time_(0) {}
    StatusSample(G3Time time, std::string status) :
        G3FrameObject(), time_(time), status_(status){}

    std::string status_;

    G3Time time_;

    static void setup_python() {};
};

G3_POINTERS(StatusSample);

enum TimestampType {Timing_LowPrecision, Timing_HighPrecision};
static const char * TimestampTypeStrings[] = {"Low Precision", "High Precision"};

class SmurfSample : public G3FrameObject{
public:
    SmurfSample(G3Time time, SmurfPacketROPtr sp) :
        G3FrameObject(), time_(time), sp(sp) {}

    SmurfPacketROPtr sp;

    G3Time time_;

    static const TimestampType timing_type_ = Timing_LowPrecision;

    static void setup_python() {};
};

G3_POINTERS(SmurfSample);

#endif
