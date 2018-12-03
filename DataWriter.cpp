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


#define NCHANS 4096
#define NSAMPLES 100

namespace bp = boost::python;
namespace ris = rogue::interfaces::stream;

// Seconds between 1970 to 1990
int sec_1970_to_1990 = 631152000;

double int_to_phase = M_PI / (1<<15);

class DataWriter : public rogue::interfaces::stream::Slave {
    uint32_t rxCount, rxBytes, rxLast, c;
    G3TimestreamPtr timestreams[NCHANS];
    G3TimestreamMapPtr ts_map;

    int32_t *phases, *phases_f;

    // int32_t phases[NCHANS][NSAMPLES];
    // int32_t phases_f[NCHANS][NSAMPLES];
    G3WriterPtr writer;
    G3Time start, stop;

    filtbank *bank1, *bank2;
    filtbank banks[2];

public:

    DataWriter() : ts_map(new G3TimestreamMap), writer(new G3Writer("phases.g3")) {
        rxCount = 0; rxBytes = 0; rxLast = 0; c = 0;

        phases = (int32_t *)calloc(NSAMPLES * NCHANS, sizeof(*phases));
        phases_f = (int32_t *)calloc(NSAMPLES * NCHANS, sizeof(*phases_f));

        filtpar *pars = mce_filter();
        bank1 = create_filtbank(NCHANS, pars + 0);
        bank2 = create_filtbank(NCHANS, pars + 1);
        memcpy(&banks[0], bank1, sizeof(*bank1));
        memcpy(&banks[1], bank2, sizeof(*bank2));

        for (int i = 0; i < NCHANS; i++){
            timestreams[i] = G3TimestreamPtr(new G3Timestream(NSAMPLES));
            ts_map->insert(std::make_pair(std::to_string(i),
                                            G3TimestreamPtr(new G3Timestream(NSAMPLES))));
        }
    }

    uint32_t getCount() { return rxCount; } // Total frames
    uint32_t getBytes() { return rxBytes; } // Total Bytes
    uint32_t getLast()  { return rxLast;  } // Last frame size

    double toPhase(int16_t phase_int){
        return phase_int * M_PI / (1 << 15);
    }

    G3Time toG3Time(uint32_t sec, uint32_t ns){
        // Number of seconds between 1970 (G3) and 1990 (Smurf)
        G3TimeStamp ts = 631152000 * G3Units::s;
        ts += sec * G3Units::s;
        ts += ns * G3Units::ns;
        return G3Time(ts);
    }

    void endFile(){
        G3FramePtr f = G3FramePtr(new G3Frame);
        f->type = G3Frame::EndProcessing;

        std::deque<G3FramePtr> junk;
        printf("Ending file...\n");
        writer->Process(f, junk);
    }

    void writeG3Frame(){
        /* Downsample array */
        multi_filter_data(banks, 2, phases, phases_f, NSAMPLES);

        /* Write to G3TimestreamMap */
        for (int i = 0; i < NCHANS; i++){
            for (int j = 0; j < NSAMPLES; j++){
                (*timestreams[i])[j] = toPhase(phases_f[i * NSAMPLES + j]);
            }
        }

        /* Writes G3TimestreamMap to frame */
        G3FramePtr f = G3FramePtr(new G3Frame);
        f->Put("Phases", ts_map);

        std::deque<G3FramePtr> junk;
        writer->Process(f, junk);
    }

    void acceptFrame ( ris::FramePtr frame ) {
        // if (rxCount > 0)
        // return  ;

        if (start.time == 0)
            start = G3Time::Now();

        uint32_t nbytes = frame->getPayload();
        rxLast   = nbytes;
        rxBytes += nbytes;
        rxCount++;

        // printf("Number of bytes: %d\n", nbytes);

        // Iterators to start and end of frame
        rogue::interfaces::stream::Frame::iterator iter = frame->beginRead();
        rogue::interfaces::stream::Frame::iterator  end = frame->endRead();

        // Example destination for data copy
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
        // printf("sec: %d\tns: %d\n", sec, ns);

        /* Reads Phases */
        int16_t phase_int;
        uint8_t offset; // bit offset for phase
        for (uint32_t i = 0; i < NCHANS; i++){
            offset = i & 3;
            phases[i * NSAMPLES + c] = buff[header_len + i/4] >> (16 * offset) & 0xffff;
            // phases[i][c] = buff[header_len + i/4] >> (16 * offset) & 0xffff;
            // (*phases[i])[c] = phase_int * M_PI / (1<<15);
            // phase_map->find(std::to_string(i))->second->push_back(phase_int * M_PI / (1<<15));
            // printf("Phase %d: %d\n", i, phase_int);
        }
        c++;

        if (c == NSAMPLES){
            stop = G3Time::Now();

            writeG3Frame();
            // stop = G3Time(toG3TimeStamp(sec, ns));
            // for (int i = 0; i < NCHANS; i++){
            //     phases[i]->start = start;
            //     phase_array[i]->stop = stop;
            // }
            // printf("Start time: %s\n", start.isoformat().c_str());
            // printf("Stop time: %s\n", stop.isoformat().c_str());
            // writeG3Frame();
            // start = stop;
            start = G3Time::Now();
            c = 0;
        }
        return;
    }
    // Expose methods to python
    static void setup_python() {
        bp::class_<DataWriter, boost::shared_ptr<DataWriter>, bp::bases<ris::Slave>, boost::noncopyable >("DataWriter",bp::init<>())
        .def("getCount", &DataWriter::getCount)
        .def("getBytes", &DataWriter::getBytes)
        .def("getLast",  &DataWriter::getLast)
        .def("endFile",  &DataWriter::endFile)
        ;
        bp::implicitly_convertible<boost::shared_ptr<DataWriter>, ris::SlavePtr>();
    };
};

BOOST_PYTHON_MODULE(DataWriter) {
    PyEval_InitThreads();
    try {
        DataWriter::setup_python();
    } catch (...) {
        printf("Failed to load module. import rogue first\n");
    }
    printf("Loaded my module\n");
};
