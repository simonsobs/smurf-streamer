#ifndef SMURF_BUILDER_H
#define SMURF_BUILDER_H
#include <unordered_map>
#include <deque>
#include <thread>

#include <G3EventBuilder.h>
#include <G3Logging.h>

#include "SmurfSample.h"

#define MAX_DATASOURCE_QUEUE_SIZE 3000

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
    std::deque<SmurfSampleConstPtr> write_stash_, read_stash_;
    std::mutex write_stash_lock_, read_stash_lock_;

    uint16_t num_channels_;


    // Puts all stashed data in G3Frame and sends it out.
    void FlushReadStash();
    void SwapStash();

    // Calls FlushStash every agg_duration_ seconds
    static void ProcessStashThread(SmurfBuilder *);

    bool running_;

    std::thread process_stash_thread_;

    std::vector<std::string> chan_names_;

    // Stores current output frame number
    uint32_t out_num_;
};

G3_POINTERS(SmurfBuilder);

#endif
