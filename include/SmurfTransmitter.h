#ifndef _SMURFTRANSMITTER_H
#define _SMURFTRANSMITTER_H

/*
 *  Sample object passed to SmurfBuilder object
 */
class SmurfSampleFrameObject : public G3FrameObject{
public:
    uint32_t seq;
};


class SmurfTransmitter : public sct::BaseTransmitter{
public:
    SmurfTransmitter(G3EventBuilder builder);

    // Called by SmurfProcessor with downsampled data packet
    void transmit(SmurfPacketROPtr packet);

    // Sets python bindings
    void setup_python();

private:
    G3EventBuilderPtr builder_;
}


#endif
