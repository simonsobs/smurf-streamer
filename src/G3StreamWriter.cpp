/*
*-----------------------------------------------------------------------------
* Title      : Source code for MyModule
*-----------------------------------------------------------------------------
* File       : exoTest.py
* Created    : 2018-02-28
*-----------------------------------------------------------------------------
* This file is part of the rogue_example software. It is subject to
* the license terms in the LICENSE.txt file found in the top-level directory
* of this distribution and at:
*    https://confluence.slac.stanford.edu/display/ppareg/LICENSE.html.
* No part of the rogue_example software, including this file, may be
* copied, modified, propagated, or distributed except according to the terms
* contained in the LICENSE.txt file.
*-----------------------------------------------------------------------------
*/


#include <rogue/interfaces/stream/Slave.h>
#include <rogue/interfaces/stream/Frame.h>
#include <rogue/interfaces/stream/FrameIterator.h>
#include <boost/python.hpp>
#include <boost/python/module.hpp>
#include <string>
#include <G3Frame.h>
#include <G3Writer.h>
#include <G3Data.h>
#include <G3Vector.h>
#include <G3Timestream.h>
#include <G3TimeStamp.h>
#include <G3Units.h>
#include <math.h>
#include <chrono>
#include "lo_pass.h"
#include "G3StreamWriter.h"

#include <random>

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

G3StreamWriter::G3StreamWriter(std::string filename) :
        rxCount(0), rxBytes(0), rxLast(0), cur_sample(0),
        ts_map(new G3TimestreamMap),
        ts_map_f(new G3TimestreamMap),
        writer( new G3Writer(filename)),
        downsample_factor(10)
{
    // Array of phases
    phases = (int32_t *)calloc(NSAMPLES * NCHANS, sizeof(*phases));
    // Array of phases after low_pass_filtering
    phases_f = (int32_t *)calloc(NSAMPLES * NCHANS, sizeof(*phases_f));

    filtpar *pars = mce_filter();
    bank1 = create_filtbank(NCHANS, pars + 0);
    bank2 = create_filtbank(NCHANS, pars + 1);
    memcpy(&banks[0], bank1, sizeof(*bank1));
    memcpy(&banks[1], bank2, sizeof(*bank2));

    for (int i = 0; i < NCHANS; i++){
        timestreams[i] = G3TimestreamPtr(new G3Timestream(NSAMPLES));
        ts_map->insert(std::make_pair(std::to_string(i), timestreams[i]));

        timestreams_f[i] = G3TimestreamPtr(new G3Timestream((int)(NSAMPLES / downsample_factor)));
        ts_map_f->insert(std::make_pair(std::to_string(i), timestreams_f[i]));
    }
}

void G3StreamWriter::endFile(){
    G3FramePtr f = G3FramePtr(new G3Frame);
    f->type = G3Frame::EndProcessing;

    std::deque<G3FramePtr> junk;
    printf("Ending file...\n");
    writer->Process(f, junk);
}

void G3StreamWriter::writeG3Frame(){
    /* Downsample array */
    // multi_filter_data(banks, 2, phases, phases_f, NSAMPLES);

    /* Write to G3TimestreamMap */
    for (int i = 0; i < NCHANS; i++){
        // timestreams[i]->start = start;
        timestreams_f[i]->start = start;

        // timestreams[i]->stop = stop;
        timestreams_f[i]->stop = stop;

        for (int j = 0; j < NSAMPLES / downsample_factor; j++){
            (*timestreams_f[i])[j] = toPhase(phases_f[i * NSAMPLES + j * downsample_factor]);
        }

        // Writes
        // for (int j = 0; j < NSAMPLES; j++){
        //     (*timestreams[i])[j] = toPhase(phases[i * NSAMPLES + j]);
        // }
    }

    // printf("Writing G3Frame (%d samples @ %.0f Hz)...\n",
    //             NSAMPLES, timestreams_f[0]->GetSampleRate() * G3Units::s);

    /* Writes G3TimestreamMap to frame */
    G3FramePtr f = G3FramePtr(new G3Frame);

    f->Put("filtered", ts_map_f);
    // f->Put("unfiltered", ts_map);
    writer->Process(f, junk);
}

void G3StreamWriter::acceptFrame ( ris::FramePtr frame ) {
    // if (rxCount > 0)
    // return  ;

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

        phases[i * NSAMPLES + cur_sample] = //(int32_t)(distribution(generator) * 100);
            (int32_t)(phase_int);
    }

    cur_sample++;
    if (cur_sample == NSAMPLES){
        stop = G3Time::Now();
        // G3Time x = G3Time::Now();
        writeG3Frame();
        cur_sample = 0;
    }
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
