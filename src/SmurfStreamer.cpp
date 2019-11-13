#include <rogue/interfaces/stream/Slave.h>
#include <rogue/interfaces/stream/Frame.h>
#include <rogue/interfaces/stream/FrameIterator.h>
#include <rogue/GilRelease.h>


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

#include <utility>
#include <string>
#include <math.h>
#include <thread>

#include "SmurfStreamer.h"
#include "StreamConfig.h"

#include "smurf/core/transmitters/BaseTransmitter.h"

// Temporary measure until this builds
#define smurfsamples 528


namespace sct = smurf::core::transmitters;
// namespace ris = rogue::interfaces::stream;
namespace bp = boost::python;

//  Function for converting smurf timestamp to G3Time
G3Time smurf_to_G3Time(uint64_t time){

    // This might not be the correct conversion if the timestamps are wrt
    // different years... Need to check when we actually get data.
    return G3Time(time * G3Units::seconds);
}

SmurfStreamer::SmurfStreamer(std::string config_file):
        sct::BaseTransmitter(),
        config(config_file), frame_time(config.frame_time),
        ts_map(new G3TimestreamMap), chan_keys(new G3VectorString(smurfsamples)),
        frame_num(new G3Int(0)),
        writer(G3NetworkSender("*", config.port, config.max_queue_size))
{
    G3TimePtr session_start_time = G3TimePtr(new G3Time(G3Time::Now()));
    session_id = G3IntPtr(new G3Int(std::hash<G3Time*>()(session_start_time.get())));

    // Creates a timestream for each channel
    for (int i = 0; i < smurfsamples; i++){
        timestreams[i] = G3TimestreamPtr(new G3Timestream());
        timestreams[i]->units= G3Timestream::Counts;
        timestreams[i]->SetFLACCompression(config.flac_level);
        (*chan_keys)[i] = std::to_string(i);
        ts_map->insert(std::make_pair((*chan_keys)[i], timestreams[i]));
    }

    // Writes Session frame
    G3FramePtr f(new G3Frame(G3Frame::Observation));
    std::deque<G3FramePtr> junk;
    f->Put("session_id", session_id);
    f->Put("session_start_time", session_start_time);
    writer.Process(f, junk);

    run_thread = std::thread(&SmurfStreamer::run, this);
}

/*
    Loops and writes G3Frames every so often. Should be run in separate thread.
*/
void SmurfStreamer::run(){
    rogue::GilRelease noGil;

    printf("Starting run thread for G3Streamer\n");

    running = true;
    while (running){
        printf("HERE!\n");
        usleep(frame_time * 1000000);

        packet_queue.swap();
        // Puts packet queue's write buffer into the read_queue

        int nsamples = packet_queue.read_queue.size();
        if (nsamples == 0)
            continue;

        G3Time ts; // Packet timestamp
        SmurfPacketROPtr p; // Packet

        // Resizes timestreams to fit current queue
        // and sets start and end times
        for (int i = 0; i < smurfsamples; i++){
            timestreams[i]->resize(nsamples);

            ts = packet_queue.read_queue.front().first;
            p = packet_queue.read_queue.front().second;
            if (i==0)
                printf("Start time: %s\n", ts.isoformat().c_str());
            if (p->getHeader()->getUnixTime() != 0)
                timestreams[i]->start = smurf_to_G3Time(p->getHeader()->getUnixTime());
            else
                timestreams[i]->start = ts;

            ts = packet_queue.read_queue.back().first;
            p = packet_queue.read_queue.back().second;
            if (p->getHeader()->getUnixTime() != 0)
                timestreams[i]->stop = smurf_to_G3Time(p->getHeader()->getUnixTime());
            else
                timestreams[i]->stop = ts;
        }

        for (int i = 0; i < nsamples; i++){
            p = packet_queue.read_queue.front().second;

            for (int j = 0; j < smurfsamples; j++)
                (*timestreams[j])[i] = p->getData(j);

            packet_queue.read_queue.pop();
        }

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

void SmurfStreamer::stop(){
    printf("Stopping stream...\n");
    running = false;
    run_thread.join();
    printf("Stopped Stream.\n");
}

void SmurfStreamer::transmit(SmurfPacketROPtr packet){
    printf("Transmit...\n");
    fflush(stdout);
    return;
    if (!running)
        return;
    packet_queue.push(std::make_pair (G3Time::Now(), packet));
};

std::shared_ptr<SmurfStreamer> SmurfStreamerInit(std::string config_file="config.txt"){
    std::shared_ptr<SmurfStreamer> writer(new SmurfStreamer(config_file));
    return writer;
}

void SmurfStreamer::setup_python(){
    bp::class_<SmurfStreamer, std::shared_ptr<SmurfStreamer>,
                bp::bases<ris::Slave>, boost::noncopyable >
                ("SmurfStreamer", bp::no_init)
    .def("__init__", bp::make_constructor(
        &SmurfStreamerInit, bp::default_call_policies(), (bp::arg("config_file")="config.txt")
    ))
    .def("stop", &SmurfStreamer::stop)
    ;
    bp::implicitly_convertible<std::shared_ptr<SmurfStreamer>, ris::SlavePtr>();
}
