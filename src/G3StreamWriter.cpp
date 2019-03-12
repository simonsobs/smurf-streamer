#include <rogue/interfaces/stream/Slave.h>
#include <rogue/interfaces/stream/Frame.h>
#include <rogue/interfaces/stream/FrameIterator.h>

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
#include <random>

#include "lo_pass.h"
#include "G3StreamWriter.h"

namespace ris = rogue::interfaces::stream;
namespace bp = boost::python;

// Downsampling factor. We'll be receiving already filtered and downsampled data
// So this is just to make sure file sizes are right.
#define DSFactor 10

// Maps from 16 int to +/- pi
// TODO: What should happen if phase_int > 2^16 from filter gain?
double toPhase(int32_t phase_int){return phase_int * M_PI / (1 << 15);}

//Calculates the G3Time from timestamps given by SMuRF.
G3Time toG3Time(uint32_t sec, uint32_t ns){
    // Number of seconds between 1970 (G3) and 1990 (Smurf)
    G3TimeStamp ts = 631152000 * G3Units::s;
    ts += sec * G3Units::s;
    ts += ns * G3Units::ns;
    return G3Time(ts);
}

G3StreamWriter::G3StreamWriter(int port, int num_samples, int max_queue_size):
        nsamples(num_samples), cur_sample(0),
        ts_map(new G3TimestreamMap),
        chan_keys(new G3VectorString(NCHANS)),
        writer(new G3NetworkSender("*", port, max_queue_size)),
        frame_num(new G3Int(0)),
        session_id(new G3Int(0))
{

    std::hash<time_t> hash;
    session_id->value = hash(time(NULL));

    phases = (int32_t *)calloc(nsamples * NCHANS, sizeof(*phases));
    phases_cpy = (int32_t *)calloc(nsamples * NCHANS, sizeof(*phases));

    for (int i = 0; i < NCHANS; i++){
        timestreams[i] = G3TimestreamPtr(new G3Timestream(nsamples/DSFactor));
        (*chan_keys)[i] = std::to_string(i);
        ts_map->insert(std::make_pair((*chan_keys)[i], timestreams[i]));
    }
}

void G3StreamWriter::writeG3Frame(G3Time start_time, G3Time stop_time){

    /* Write to G3TimestreamMap */
    for (int i = 0; i < NCHANS; i++){
        timestreams[i]->start = start_time;
        timestreams[i]->stop = stop_time;

        for (int j = 0; j < nsamples / DSFactor; j++){
            (*timestreams[i])[j] = toPhase(phases_cpy[i * nsamples + j*DSFactor]);
        }
    }

    /* Writes G3TimestreamMap to frame */
    G3FramePtr f = G3FramePtr(new G3Frame);
    std::deque<G3FramePtr> junk;
    f->Put("keys", chan_keys);
    f->Put("data", ts_map);
    f->Put("frame_num", frame_num);
    frame_num->value += 1;
    f->Put("session_id", session_id);
    write_mtx.lock();
    printf("Writing G3Frame (%lu samples @ %.0f Hz)...\n",
                timestreams[0]->size(), timestreams[0]->GetSampleRate() * G3Units::s);
    writer->Process(f, junk);
    write_mtx.unlock();
    return;
}

void G3StreamWriter::acceptFrame ( ris::FramePtr frame ) {
    // Timestamping manually because currently the Smurf doesn't pass timestamps
    if (cur_sample == 0){
        start = G3Time::Now();
    }

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
    // uint32_t num_channels = buff[0] >> (32);
    uint32_t seq = buff[10] >> (32);
    uint32_t sec = buff[9] >> 32;
    uint32_t ns =  buff[9] & 0xFFFFFFFF;

    if (seq != (uint32_t)(last_seq_rx+1)){
        printf("Missed %u Frame(s)! last: %u, cur: %u\n", seq - last_seq_rx, last_seq_rx, seq);
    }
    last_seq_rx = seq;

    /* Reads Phases */
    uint8_t offset; // bit offset for phase
    int16_t phase_int;
    for (uint32_t i = 0; i < NCHANS; i++){
        offset = i & 3;

        phase_int = buff[header_len + i/4] >> (16 * offset) & 0xffff;
        phases[i * nsamples + cur_sample] = (int32_t)(phase_int);
    }
    cur_sample++;

    if (cur_sample == nsamples){
        stop = G3Time::Now();
        memcpy(phases_cpy, phases, NCHANS * nsamples * sizeof(*phases));

        std::thread writer_thread(&G3StreamWriter::writeG3Frame, this, start, stop);
        writer_thread.detach();
        cur_sample = 0;
    }
    free(buff);
    return;
}

boost::shared_ptr<G3StreamWriter> G3StreamWriterInit(
        int port=4536, int num_samples=1000, int max_queue_size=100
    ){
        boost::shared_ptr<G3StreamWriter> writer(new G3StreamWriter(port, num_samples, max_queue_size));
        return writer;
}

BOOST_PYTHON_MODULE(G3StreamWriter) {
    PyEval_InitThreads();
    try {
        bp::class_<G3StreamWriter, boost::shared_ptr<G3StreamWriter>,
                    bp::bases<ris::Slave>, boost::noncopyable >("G3StreamWriter",
                    bp::no_init)
        .def("__init__", bp::make_constructor(
            &G3StreamWriterInit, bp::default_call_policies(), (
                bp::arg("port")=4536,
                bp::arg("num_samples")=1000,
                bp::arg("max_queue_size")=100
            )
        ))
        // .def("endFile",  &G3StreamWriter::endFile)
        ;
        bp::implicitly_convertible<boost::shared_ptr<G3StreamWriter>, ris::SlavePtr>();
    } catch (...) {
        printf("Failed to load module. import rogue first\n");
    }
    printf("Loaded my module\n");
};
