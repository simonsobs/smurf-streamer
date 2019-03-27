#include <rogue/interfaces/stream/Slave.h>
#include <rogue/interfaces/stream/Frame.h>
#include <rogue/interfaces/stream/FrameIterator.h>
#include <G3TimeStamp.h>

#include "SampleData.h"

namespace ris = rogue::interfaces::stream;


int SampleBuffer::swap(){
    mutex.lock();
    write_buffer.swap(read_buffer);
    read_count = write_count;
    write_count = 0;
    mutex.unlock();
    return read_count;
}

void SampleBuffer::write(SampleDataPtr sample){
    if (write_count >= buff_max){
        printf("Buffer full......\n");
        return;
    }
    mutex.lock();
    write_buffer[write_count++] = sample;
    mutex.unlock();
}

SampleData::SampleData(ris::FramePtr frame){
    uint32_t nbytes = frame->getPayload();

    // Iterators to start and end of frame
    ris::Frame::iterator iter = frame->beginRead();
    ris::Frame::iterator  end = frame->endRead();

    // Copies frame data to buffer
    uint64_t *buff = (uint64_t *)malloc (nbytes);
    memset(buff, 0, nbytes);
    uint64_t  *dst = buff;

    while (iter != end){
       auto size = iter.remBuffer ();
       auto *src = iter.ptr();
       memcpy(dst, src, size);
       dst += size/sizeof(dst[0]);
       iter += size;
    }

    /* Reads header */
    uint8_t header_len = 16;
    uint32_t num_channels = buff[0] >> (32);
    seq = buff[10] >> (32);
    uint32_t sec = buff[9] >> 32;
    uint32_t ns =  buff[9] & 0xFFFFFFFF;

    if (sec == 0){
        timestamp = G3Time::Now();
    }

    data = std::vector<int16_t>(num_channels);
    uint8_t offset;
    for (uint32_t i = 0; i < num_channels; i++){
        offset = i & 3;
        data[i] = buff[header_len + i/4] >> (16 * offset) & 0xffff;
    }

    free(buff);
}
