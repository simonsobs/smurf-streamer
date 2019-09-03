#include <rogue/interfaces/stream/Slave.h>
#include <rogue/interfaces/stream/Frame.h>
#include <rogue/interfaces/stream/FrameIterator.h>
#include <G3TimeStamp.h>
#include <G3Units.h>
#include <smurf_processor.h>

#include "SampleData.h"

#define MAX_BUFF_SIZE 100000

namespace ris = rogue::interfaces::stream;

int SampleBuffer::swap(){
    mutex.lock();
    write_buffer.swap(read_buffer);
    read_count = write_count;
    write_count = 0;
    mutex.unlock();
    return read_count;
}

void SampleBuffer::write(SmurfPacket_RO packet){
    int cur_size = write_buffer.size();

    if (write_count >= cur_size){
        if (cur_size > MAX_BUFF_SIZE){
            printf("Buffer full and @ max size. Dropping Frame.\n");
            return;
        }

        printf("Buffer full.... Doubling size to %d samples\n", 2*cur_size);
        write_buffer.resize(2*cur_size);
        read_buffer.resize(2*cur_size);
    }

    mutex.lock();
    write_buffer[write_count++] = packet;
    mutex.unlock();
}

// SampleData::SampleData(smurf_tx_data_t* buffer):
//     header(new SmurfHeader()),
//     data(smurfsamples)
// {
//     header->copy_header(buffer);
//
//     // Copies channel data from buffer
//     uint offset;
//     for (uint i = 0; i < smurfsamples; i++){
//         offset = i * sizeof(avgdata_t) + smurfheaderlength;
//         data[i] = pull_bit_field(header->header, offset, sizeof(avgdata_t));
//     }
//
//     // Sets G3TimeStamp
//     // Is this the right timestamp to use?
//     uint64_t t = pull_bit_field(header->header, 48, 8);;
//     if (t == 0){
//         timestamp = G3Time::Now();
//     } else{
//         timestamp = G3Time(t * G3Units::ns);
//     }
// }
