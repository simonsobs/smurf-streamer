#include "SmurfTransmitter.h"
#include <iostream>


SmurfTransmitter::SmurfTransmitter() : debug_(false){}

SmurfTransmitter::SmurfTransmitter(bool debug) : debug_(debug){
    if (debug_)
        printf("Starting SmurfTransmitter in debug mode...");
}

void SmurfTransmitter::transmit(SmurfPacketROPtr packet){
    if (debug_){
        printSmurfPacket(packet);
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
    std::cout << " DATA (up to the first 20 points):" << std::endl;
    std::cout << "-----------------------" << std::endl;

    std::size_t n{20};
    if (numCh < n)
        n = numCh;

    for (std::size_t i(0); i < n; ++i)
            std::cout << "Data[" << i << "] = " << sp->getData(i) << std::endl;

        std::cout << "-----------------------" << std::endl;
        std::cout << std::endl;

        std::cout << "=====================================" << std::endl;
}
