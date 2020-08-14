#include "SmurfTransmitter.h"
#include "SmurfSample.h"
#include "SmurfBuilder.h"
#include <iostream>
#include <boost/algorithm/string.hpp>
#include <inttypes.h>

namespace sct = smurf::core::transmitters;

SmurfTransmitter::SmurfTransmitter(G3EventBuilderPtr builder) :
    sct::BaseTransmitter(), builder_(builder),
    debug_data_(false), debug_meta_(false){}

SmurfTransmitter::SmurfTransmitter(G3EventBuilderPtr builder, bool debug_data, bool debug_meta) :
    sct::BaseTransmitter(), builder_(builder),
    debug_data_(debug_data), debug_meta_(debug_meta){

    if (debug_data_)
        log_info("Starting SmurfTransmitter in debug mode...");
}

SmurfTransmitter::~SmurfTransmitter(){}

void SmurfTransmitter::metaTransmit(std::string cfg){
    if (debug_meta_){
        std::cout << "=====================================" << std::endl;
        std::cout << "Metadata received" << std::endl;
        std::cout << "=====================================" << std::endl;
        std::cout << cfg << std::endl;
        std::cout << "=====================================" << std::endl;
        std::cout.flush();
    }

    G3Time ts = G3Time::Now();
    StatusSamplePtr status_sample(new StatusSample(ts, cfg));
    builder_->AsyncDatum(ts.time, status_sample);
}

void SmurfTransmitter::dataTransmit(SmurfPacketROPtr sp){
    if (debug_data_)
        printSmurfPacket(sp);

    G3Time ts = G3Time::Now();
    SmurfSamplePtr smurf_sample(new SmurfSample(ts, sp));
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
    for (std::size_t i{0}; i < N_TES_BIAS; ++i)
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
