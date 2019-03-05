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

// Maps from 16 int to +/- pi
// TODO: What should happen if phase_int > 2^16 from filter gain?
double toPhase(int32_t phase_int){
    return phase_int * M_PI / (1 << 15);
}

//Calculates the G3Time from timestamps given by SMuRF.
G3Time toG3Time(uint32_t sec, uint32_t ns){
    // Number of seconds between 1970 (G3) and 1990 (Smurf)
    G3TimeStamp ts = 631152000 * G3Units::s;
    ts += sec * G3Units::s;
    ts += ns * G3Units::ns;
    return G3Time(ts);
}


G3StreamWriter::G3StreamWriter(std::string filename, int port) :
        rxCount(0), rxBytes(0), rxLast(0), cur_sample(0),
        ts_map(new G3TimestreamMap),
        writer(new G3NetworkSender("*", port, 10)),
        downsample_factor(1)
{
    phases = (int32_t *)calloc(NSAMPLES * NCHANS, sizeof(*phases));
    phases_cpy = (int32_t *)calloc(NSAMPLES * NCHANS, sizeof(*phases));
    phases_filtered = (int32_t *)calloc(NSAMPLES * NCHANS, sizeof(*phases_filtered));

    filtpar *pars = mce_filter();
    bank1 = create_filtbank(NCHANS, pars + 0);
    bank2 = create_filtbank(NCHANS, pars + 1);
    memcpy(&banks[0], bank1, sizeof(*bank1));
    memcpy(&banks[1], bank2, sizeof(*bank2));

    for (int i = 0; i < NCHANS; i++){
        timestreams[i] = G3TimestreamPtr(new G3Timestream(NSAMPLES / downsample_factor));
        ts_map->insert(std::make_pair(std::to_string(i), timestreams[i]));
    }
}

// void G3StreamWriter::endFile(){
//     G3FramePtr f = G3FramePtr(new G3Frame);
//     f->type = G3Frame::EndProcessing;
//
//     write_mtx.lock();
//     printf("Ending file...\n");
//     writer->Process(f, junk);
//     write_mtx.unlock();
// }

void G3StreamWriter::writeG3Frame(G3Time start_time, G3Time stop_time){

    /* Filter array */
    multi_filter_data(banks, 2, phases_cpy, phases_filtered, NSAMPLES);

    /* Write to G3TimestreamMap */
    for (int i = 0; i < NCHANS; i++){
        timestreams[i]->start = start_time;
        timestreams[i]->stop = stop_time;

        for (int j = 0; j < NSAMPLES / downsample_factor; j++){
            (*timestreams[i])[j] = toPhase(phases_filtered[i * NSAMPLES + j * downsample_factor]);
        }
    }

    /* Writes G3TimestreamMap to frame */
    G3FramePtr f = G3FramePtr(new G3Frame);
    f->Put("phases", ts_map);
    write_mtx.lock();
    printf("Writing G3Frame (%d samples @ %.0f Hz)...\n",
                NSAMPLES, timestreams[0]->GetSampleRate() * G3Units::s);
    writer->Process(f, junk);
    write_mtx.unlock();
    return;
}

void G3StreamWriter::acceptFrame ( ris::FramePtr frame ) {
    // if (rxCount > 0)
    // return  ;

    // Timestamping manually because currently the Smurf doesn't pass timestamps
    if (cur_sample == 0){
        start = G3Time::Now();
    }

    uint32_t nbytes = frame->getPayload();
    rxLast   = nbytes;
    rxBytes += nbytes;
    rxCount++;

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
    // printf("sec: %d\tns: %d\n", sec, ns);

    /* Reads Phases */
    uint8_t offset; // bit offset for phase
    int16_t phase_int;
    for (uint32_t i = 0; i < NCHANS; i++){
        offset = i & 3;

        phase_int = buff[header_len + i/4] >> (16 * offset) & 0xffff;

        phases[i * NSAMPLES + cur_sample] = (int32_t)(phase_int);
    }


    cur_sample++;
    if (cur_sample == NSAMPLES){

        stop = G3Time::Now();
        memcpy(phases_cpy, phases, NCHANS * NSAMPLES * sizeof(*phases));
        // writeG3Frame(start, stop);
        std::thread writer_thread(&G3StreamWriter::writeG3Frame, this, start, stop);
        // writer_thread.join();
        writer_thread.detach();
        cur_sample = 0;
    }
    free(buff);
    return;
}

BOOST_PYTHON_MODULE(G3StreamWriter) {
    PyEval_InitThreads();
    try {
        G3StreamWriter::setup_python();
    } catch (...) {
        printf("Failed to load module. import rogue first\n");
    }
    printf("Loaded my module\n");
};
