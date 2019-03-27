#ifndef _SampleData_H
#define _SampleData_H

#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>

#include <rogue/interfaces/stream/Slave.h>
#include <rogue/interfaces/stream/Frame.h>
#include <rogue/interfaces/stream/FrameIterator.h>

#include <G3TimeStamp.h>
#include <mutex>

namespace ris = rogue::interfaces::stream;



class SampleData{
public:
    uint32_t seq;
    G3Time timestamp;
    std::vector<int16_t> data;
    SampleData(ris::FramePtr frame);
};
typedef boost::shared_ptr<SampleData> SampleDataPtr;

class SampleBuffer{
private:
    std::vector<SampleDataPtr> write_buffer;
    int write_count;
    std::mutex mutex;
public:
    int buff_max;
    int read_count;
    std::vector<SampleDataPtr> read_buffer;

    SampleBuffer(int buff_max=1000):
        buff_max(buff_max),
        read_buffer(buff_max), write_buffer(buff_max),
        read_count(0), write_count(0){}


    int swap();
    void write(SampleDataPtr sample);

};

#endif
