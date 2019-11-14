#ifndef SMURF_BUILDER_H
#define SMURF_BUILDER_H
#include <unordered_map>
#include <deque>

#include <G3EventBuilder.h>
#include <G3Logging.h>

#include "SmurfSample.h"

#define MAX_DATASOURCE_QUEUE_SIZE 1000
//
class SmurfBuilder : public G3EventBuilder{
public:
    SmurfBuilder();
    ~SmurfBuilder();

    static void setup_python();

protected:
    void ProcessNewData();

private:
    struct oqueue_entry {
        G3FramePtr frame;
        SmurfSamplePtr sample;
        G3TimeStamp time;
    };

    std::deque<struct oqueue_entry> oqueue_;
};

G3_POINTERS(SmurfBuilder);

#endif
