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

#define NCHANS 4096

namespace ris = rogue::interfaces::stream;
namespace bp = boost::python;
class G3StreamWriter: public ris::Slave{
public:

    G3StreamWriter(int port, int num_samples, int max_queue_size);

    // Writes cached samples to G3Frame
    void writeG3Frame(G3Time start_time, G3Time stop_time);
    // Writes remaining unsaved data to G3Frame
    // void flush();

    // Called whenever frame is passed from master
    void acceptFrame ( ris::FramePtr frame );


    // Keeps track of bytes transmitted
    // Number of samples per G3Frame
    const int nsamples;
    uint32_t cur_sample;
    uint32_t last_seq_rx;

    G3IntPtr frame_num;
    G3IntPtr session_id;
    G3VectorStringPtr chan_keys;
    G3TimestreamPtr timestreams[NCHANS];
    G3TimestreamMapPtr ts_map;



    // Stores all detector phases as they come from rogue
    int32_t *phases;

    // Copy of phases that is refreshed once all chans and samples have been read.
    // Used in thread to write G3 output.
    int32_t *phases_cpy;

    // Stores phases after low_pass filtering
    int32_t *phases_filtered;

    G3NetworkSenderPtr writer;

    // Lock that needs to be used whenever file is written to.
    std::mutex write_mtx;

    // Keeps track of start and stop times for current frame
    G3Time start, stop;
};
#endif
