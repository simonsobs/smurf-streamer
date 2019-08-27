#include <rogue/interfaces/stream/Slave.h>
#include <rogue/interfaces/stream/Frame.h>
#include <rogue/interfaces/stream/FrameIterator.h>
#include <rogue/GilRelease.h>

#include <smurf_processor.h>

#include <boost/python.hpp>
#include <boost/python/module.hpp>

#include <G3Frame.h>
#include <G3Writer.h>
#include <G3Data.h>
#include <G3Vector.h>
#include <G3Timestream.h>
#include <G3TimeStamp.h>
#include <G3Units.h>
#include <G3NetworkSender.h>

#include <string>
#include <math.h>
#include <thread>

#include "SampleData.h"
#include "G3StreamWriter.h"
#include "StreamConfig.h"

namespace ris = rogue::interfaces::stream;
namespace bp = boost::python;


/*
    Params:
        int port: port that G3Frames will be written. Defalts 4536
        int frame_time: Time per G3Frame (seconds)
        int max_queue_size: Max queue size for G3NetworkStreamer
        int sample_buff_size: Size of Frame buffer that stores data. Defaults to
                                10000, but can be decreasod once we start getting
                                downsampled dat.
*/
G3StreamWriter::G3StreamWriter(std::string config_file):
        SmurfProcessor(),
        config(config_file),
        frame_time(config.frame_time),
        ts_map(new G3TimestreamMap),
        chan_keys(new G3VectorString(smurfsamples)),
        writer(G3NetworkSender("*", config.port, config.max_queue_size)),
        sample_buffer(),
        run_thread(&G3StreamWriter::run, this),
        frame_num(new G3Int(0)), running(true)
{
    G3TimePtr session_start_time = G3TimePtr(new G3Time(G3Time::Now()));
    session_id = G3IntPtr(new G3Int(std::hash<G3Time*>()(session_start_time.get())));

    // Writes Session frame
    G3FramePtr f(new G3Frame(G3Frame::Observation));
    std::deque<G3FramePtr> junk;
    f->Put("session_id", session_id);
    f->Put("session_start_time", session_start_time);
    writer.Process(f, junk);



    for (int i = 0; i < smurfsamples; i++){
        timestreams[i] = G3TimestreamPtr(new G3Timestream());
        timestreams[i]->units= G3Timestream::Counts;
        timestreams[i]->SetFLACCompression(config.flac_level);
        (*chan_keys)[i] = std::to_string(i);
        ts_map->insert(std::make_pair((*chan_keys)[i], timestreams[i]));
    }
}

/*
    Loops and writes G3Frames every so often. Should be run in separate thread.
*/
void G3StreamWriter::run(){
    rogue::GilRelease noGil;

    printf("Starting run thread for G3Streamer\n");
    running = true;

    while (running){
        printf("HERE\n");
        usleep(frame_time * 1000000);

        // Swaps buffers so that we can continue accepting new frames.
        int nsamples = sample_buffer.swap();

        if (nsamples == 0)
            continue;

        // Reads sample data into TimestreamMap
        for (int i = 0; i < smurfsamples; i++)
            timestreams[i]->resize(nsamples);

        SmurfPacket_RO p;
        for (int i = 0; i < nsamples; i ++){
            p = sample_buffer.read_buffer[i];

            for (int j = 0; j < smurfsamples; j++){
                // if (i == 0)
                //     timestreams[j]->start = x->timestamp;
                (*timestreams[j])[i] = p->getValue(j);
            }
        }
        // for (int i = 0; i < smurfsamples; i++)
        //     timestreams[i]->stop = x->timestamp;

        G3FramePtr f(new G3Frame(G3Frame::Scan));
        std::deque<G3FramePtr> junk;
        f->Put("keys", chan_keys);
        f->Put("data", ts_map);
        f->Put("frame_num", frame_num);
        f->Put("session_id", session_id);

        printf("Writing %lu samples @ %.2f Hz\n", ts_map->NSamples(), ts_map->GetSampleRate() / G3Units::Hz);
        writer.Process(f, junk);
        frame_num->value+=1;
    }
}

void G3StreamWriter::stop(){
    printf("Stopping stream...\n");
    running = false;
    run_thread.join();
    printf("Stopped Stream.\n");
}


void G3StreamWriter::transmit(SmurfPacket_RO packet){
    printf("HERE\n");
    if (!running)
        return;

    // SampleDataPtr sample(new SampleData(data));
    sample_buffer.write(packet);
};

boost::shared_ptr<G3StreamWriter> G3StreamWriterInit(std::string config_file="config.txt"){
        boost::shared_ptr<G3StreamWriter> writer(new G3StreamWriter(config_file));
        return writer;
}

BOOST_PYTHON_MODULE(G3StreamWriter) {
    PyEval_InitThreads();
    try {
        bp::class_<G3StreamWriter, boost::shared_ptr<G3StreamWriter>,
                    bp::bases<ris::Slave>, boost::noncopyable >("G3StreamWriter",
                    bp::no_init)
        .def("__init__", bp::make_constructor(
            &G3StreamWriterInit, bp::default_call_policies(), (bp::arg("config_file")="config.txt")
        ))
        .def("stop", &G3StreamWriter::stop)
        .def("printTransmitStatistic", &G3StreamWriter::printTransmitStatistic)
        .def("setDebug",  &G3StreamWriter::setDebug)
        ;
        bp::implicitly_convertible<boost::shared_ptr<G3StreamWriter>, ris::SlavePtr>();
    } catch (...) {
        printf("Failed to load module. import rogue first\n");
    }
    printf("Loaded my module\n");
};
