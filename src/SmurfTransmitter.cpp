#include "SmurfTransmitter.h"
#include "SmurfSample.h"
#include <iostream>

namespace sct = smurf::core::transmitters;

//

SmurfTransmitter::SmurfTransmitter(G3EventBuilderPtr builder) :
    sct::BaseTransmitter(), builder_(builder), debug_(false){}

SmurfTransmitter::SmurfTransmitter(G3EventBuilderPtr builder, bool debug) :
    sct::BaseTransmitter(), builder_(builder), debug_(debug){

    if (debug_)
        log_info("Starting SmurfTransmitter in debug mode...");
}

void SmurfTransmitter::transmit(SmurfPacketROPtr sp){
    if (debug_)
        printSmurfPacket(sp);

    // TODO: Get G3Time from smurf header...
    G3Time ts = G3Time::Now();
    size_t nchans = sp->getHeader()->getNumberChannels();
    SmurfSamplePtr smurf_sample(new SmurfSample(ts, nchans));

    auto samples = smurf_sample->Samples();
    for (int i = 0; i < nchans; i++){
        samples[i] = sp->getData(i); // I think this returns a reference. Do we want to copy?
    }

    // Sets TES Biases for SmurfSample
    for (int i = 0; i < 16; i++){ // Is this always going to be 16?
        smurf_sample->setTESBias(i, sp->getHeader()->getTESBias(i));
    }

    builder_->AsyncDatum(ts.time, smurf_sample);
}

void printSmurfPacket(SmurfPacketROPtr sp){
    std::size_t numCh {sp->getHeader()->getNumberChannels()};

    std::cout << "=====================================" << std::endl;
    std::cout << "Packet received" << std::endl;
    std::cout << "=====================================" << std::endl;
    std::cout << std::endl;

    std::cout << "-----------------------" << std::endl;
    std::cout << " HEADER:" << std::endl;
    std::cout << "-----------------------" << std::endl;
    std::cout << "Version            = " << unsigned(sp->getHeader()->getVersion()) << std::endl;
    std::cout << "Crate ID           = " << unsigned(sp->getHeader()->getCrateID()) << std::endl;
    std::cout << "Slot number        = " << unsigned(sp->getHeader()->getSlotNumber()) << std::endl;
    std::cout << "Number of channels = " << unsigned(numCh) << std::endl;
    std::cout << "Unix time          = " << unsigned(sp->getHeader()->getUnixTime()) << std::endl;
    std::cout << "Frame counter      = " << unsigned(sp->getHeader()->getFrameCounter()) << std::endl;
    std::cout << "TES Bias values:" << std::endl;
    for (std::size_t i{0}; i < 16; ++i)
        std::cout << sp->getHeader()->getTESBias(i) << ", ";
    std::cout << std::endl;

    std::cout << std::endl;

    std::cout << "-----------------------" << std::endl;
    std::cout << " DATA (up to the first 5 points):" << std::endl;
    std::cout << "-----------------------" << std::endl;

    std::size_t n{5};
    if (numCh < n)
        n = numCh;

    for (std::size_t i(0); i < n; ++i)
            std::cout << "Data[" << i << "] = " << sp->getData(i) << std::endl;

        std::cout << "-----------------------" << std::endl;
        std::cout << std::endl;

        std::cout << "=====================================" << std::endl;
}
