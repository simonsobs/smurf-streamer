#ifndef _G3_STREAMWRITER_H
#define _G3_STREAMWRITER_H

#include <rogue/interfaces/stream/Slave.h>
#include <rogue/interfaces/stream/Frame.h>
#include <rogue/interfaces/stream/FrameIterator.h>

#include <G3Frame.h>
#include <G3Timestream.h>
#include <G3TimeStamp.h>
#include <string>
#include <mutex>
#include <random>

#include "SampleData.h"

#define NCHANS 4096

namespace ris = rogue::interfaces::stream;
namespace bp = boost::python;

class G3StreamWriter: public ris::Slave{
public:

    G3StreamWriter(int port, float frame_time, int max_queue_size, int sample_buff_size);

    // Called whenever frame is passed from master
    void acceptFrame ( ris::FramePtr frame );

    // Streams data over G3Network
    void run();
    void stop();
    bool running;

    SampleBuffer sample_buffer;

    float frame_time;

    // Keeps track of bytes transmitted
    // Number of samples per G3Frame
    uint32_t last_seq_rx;

    G3IntPtr frame_num;
    G3IntPtr session_id;
    G3VectorStringPtr chan_keys;
    G3TimestreamPtr timestreams[NCHANS];
    G3TimestreamMapPtr ts_map;

    G3NetworkSenderPtr writer;
};
#endif
