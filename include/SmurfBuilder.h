#ifndef SMURF_BUILDER_H
#define SMURF_BUILDER_H
#include <unordered_map>
#include <deque>
#include <thread>

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

    float agg_duration_; // Aggregation duration in G3Timestamp units (10 ns).

protected:
    void ProcessNewData();

private:
    // Deques containing data sample pointers.
    std::deque<SmurfSampleConstPtr> stash_, read_stash_;

    std::mutex stash_lock_, read_stash_lock_;

    G3TimeStamp stash_start_time_;

    // Puts all stashed data in G3Frame and sends it out.
    void FlushStash();

    // Calls FlushStash every agg_duration_ seconds
    static void ProcessStashThread(SmurfBuilder *);

    G3FramePtr CreateStatusFrame(StatusSampleConstPtr, G3TimeStamp);

    bool running_;

    std::thread process_stash_thread_;

    std::vector<std::string> chan_names_;

    // Stores current output frame number
    uint32_t out_num_;
};

G3_POINTERS(SmurfBuilder);

#endif
