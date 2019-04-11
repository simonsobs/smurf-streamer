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
#include <smurf_processor.h>

#include "SampleData.h"

namespace ris = rogue::interfaces::stream;
namespace bp = boost::python;

class G3StreamWriter: public SmurfProcessor{
public:

    G3StreamWriter(int port, float frame_time, int max_queue_size);

    // Called whenever frame is passed from master
    // void acceptFrame ( ris::FramePtr frame );
    void transmit(smurf_tx_data_t* data);

    // Streams data over G3Network
    void run();
    void stop();
    bool running;

    SampleBuffer sample_buffer;

    float frame_time;
    int count;

    G3IntPtr frame_num;
    G3IntPtr session_id;
    G3VectorStringPtr chan_keys;
    G3TimestreamPtr timestreams[smurfsamples];
    G3TimestreamMapPtr ts_map;

    G3NetworkSenderPtr writer;
};
#endif
