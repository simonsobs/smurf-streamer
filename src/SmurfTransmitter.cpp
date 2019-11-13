#include "SmurfTransmitter.h"
#include "SmurfSample.h"
#include <iostream>


SmurfTransmitter::SmurfTransmitter() : debug_(false){}

SmurfTransmitter::SmurfTransmitter(bool debug) : debug_(debug){
    if (debug_)
        printf("Starting SmurfTransmitter in debug mode...");
}

void SmurfTransmitter::transmit(SmurfPacketROPtr sp){
    if (debug_){
        printSmurfPacket(sp);
    }

    // TODO: Get G3Time from smurf header...
    G3Time ts = G3Time::Now();
    size_t nchans = sp->getHeader()->getNumberChannels();
    SmurfSamplePtr smurf_sample(new SmurfSample(ts, nchans));

    auto samples = smurf_sample->Samples();
    for (int i = 0; i < nchans; i++){
        samples[i] = sp->getData(i); // I think this returns a reference. Do we want to copy?
    }

    // Sets TES Biases is SmurfSample
    for (int i = 0; i < 16; i++){
        smurf_sample->setTESBias(i, sp->getHeader()->getTESBias(i));
    }
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
