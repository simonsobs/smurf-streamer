#ifndef _SampleData_H
#define _SampleData_H

#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>

#include <rogue/interfaces/stream/Slave.h>
#include <rogue/interfaces/stream/Frame.h>
#include <rogue/interfaces/stream/FrameIterator.h>

#include <smurf_processor.h>

#include <G3TimeStamp.h>
#include <mutex>

namespace ris = rogue::interfaces::stream;

// class SampleData{
// public:
//     boost::shared_ptr<SmurfHeader> header;
//     std::vector<avgdata_t> data;
//     G3Time timestamp;
//     SampleData(smurf_tx_data_t* data);
// };
// typedef boost::shared_ptr<SampleData> SampleDataPtr;

class SampleBuffer{
/*
    Double buffer containing frame data.
*/
private:
    std::vector<SmurfPacket_RO> write_buffer;
    int write_count;
    std::mutex mutex;
public:
    int read_count;
    std::vector<SmurfPacket_RO> read_buffer;

    SampleBuffer(long size=1024):
        read_buffer(size), write_buffer(size),
        read_count(0), write_count(0){}

    int swap();
    void write(SmurfPacket_RO sample);
};

#endif
