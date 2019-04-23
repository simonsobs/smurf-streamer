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
#include <thread>
#include "SampleData.h"
#include "StreamConfig.h"

namespace ris = rogue::interfaces::stream;
namespace bp = boost::python;

class G3StreamWriter: public SmurfProcessor{
public:

    G3StreamWriter(std::string config_file);

    void read_config(std::string filename);
    // Called whenever frame is passed from master
    // void acceptFrame ( ris::FramePtr frame );
    void transmit(smurf_tx_data_t* data);

    // Streams data over G3Network
    void run();
    void stop();
    bool running;

    StreamConfig config;

    SampleBuffer sample_buffer;

    std::thread run_thread;

    float frame_time;

    G3IntPtr frame_num;
    G3IntPtr session_id;
    G3VectorStringPtr chan_keys;
    G3TimestreamPtr timestreams[smurfsamples];
    G3TimestreamMapPtr ts_map;

    G3NetworkSenderPtr writer;
};
#endif
