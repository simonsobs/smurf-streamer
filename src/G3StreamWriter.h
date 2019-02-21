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
#define NSAMPLES 1000


namespace ris = rogue::interfaces::stream;
namespace bp = boost::python;



class G3StreamWriter: public ris::Slave{
public:

    G3StreamWriter(std::string  filename);

    // Ends G3File with a EndProcessing Frame
    void endFile();

    // Writes cached samples to G3Frame
    void writeG3Frame(G3Time start_time, G3Time stop_time);

    // Called whenever frame is passed from master
    void acceptFrame ( ris::FramePtr frame );


    // Keeps track of bytes transmitted
    uint32_t rxCount, rxBytes, rxLast, cur_sample;
    uint32_t last_seq_rx;

    G3TimestreamPtr timestreams[NCHANS];
    G3TimestreamMapPtr ts_map;

    // Stores all detector phases as they come from rogue
    int32_t *phases;

    // Copy of phases that is refreshed once all chans and samples have been read.
    // Used in thread to write G3 output.
    int32_t *phases_cpy;

    // Stores phases after low_pass filtering
    int32_t *phases_filtered;

    G3WriterPtr writer;
    std::deque<G3FramePtr> junk;

    // Lock that needs to be used whenever file is written to.
    std::mutex write_mtx;

    // Keeps track of start and stop times for current frame
    G3Time start, stop;

    // Filter banks for low pass filter
    filtbank *bank1, *bank2;
    filtbank banks[2];
    uint16_t downsample_factor;

    uint32_t getCount() { return rxCount; } // Total frames
    uint32_t getBytes() { return rxBytes; } // Total Bytes
    uint32_t getLast()  { return rxLast;  } // Last frame size

    // Expose methods to python
    static void setup_python() {
        bp::class_<G3StreamWriter, boost::shared_ptr<G3StreamWriter>,
                    bp::bases<ris::Slave>, boost::noncopyable >("G3StreamWriter",
                    bp::init<std::string>())
        .def("getCount", &G3StreamWriter::getCount)
        .def("getBytes", &G3StreamWriter::getBytes)
        .def("getLast",  &G3StreamWriter::getLast)
        .def("endFile",  &G3StreamWriter::endFile)
        ;
        bp::implicitly_convertible<boost::shared_ptr<G3StreamWriter>, ris::SlavePtr>();
    };
};

#endif
