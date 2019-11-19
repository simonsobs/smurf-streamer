#ifndef SMURFSAMPLE_H
#define SMURFSAMPLE_H

#include <G3Frame.h>
#include <G3TimeStamp.h>
#include <vector>

#include <pybindings.h>
#include <serialization.h>


#include "smurf/core/common/SmurfPacket.h"

class SmurfAggregatedSample : public G3FrameObject {
public:
    SmurfAggregatedSample() : G3FrameObject(), Timestamp(0) {}

private:
}


class SmurfSample : public G3FrameObject{
public:
    SmurfSample() : G3FrameObject(), Timestamp(0) {}
    SmurfSample(G3Time time, size_t nsamples);

    SmurfPacketRO::data_t *Samples() const;

    void setTESBias(size_t n, uint32_t value);

    const int NSamples() const;

    G3Time Timestamp;

    template <class A> void serialize(A &ar, unsigned v);

    static void setup_python();

private:
    std::vector<SmurfPacketRO::data_t> samples;
    std::vector<uint32_t> tes_biases;
};

G3_POINTERS(SmurfSample);
// MAKE SURE TO BUMP THIS IF ANYTHING CHANGES IN THE DATA INTERFACE
G3_SERIALIZABLE(SmurfSample, 1);

#endif
