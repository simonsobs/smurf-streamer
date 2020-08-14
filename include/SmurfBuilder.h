#ifndef SMURF_BUILDER_H
#define SMURF_BUILDER_H
#include <unordered_map>
#include <deque>
#include <thread>

#include <G3EventBuilder.h>
#include <G3Logging.h>

#include "SmurfSample.h"

#define MAX_DATASOURCE_QUEUE_SIZE 3000

// Flow control constants
#define FC_ALIVE   0
#define FC_START   1
#define FC_STOP    2
#define FC_CLEANSE 3

class SmurfBuilder : public G3EventBuilder{
public:
    SmurfBuilder();
    ~SmurfBuilder();

    static void setup_python();

    float agg_duration_; // Aggregation duration in seconds
    void SetAggDuration(float dur){ agg_duration_ = dur;};
    const float GetAggDuration(){ return agg_duration_; };

    void       setDebug(bool d) { debug_ = d;    };
    const bool getDebug()       { return debug_; };

protected:
    void ProcessNewData();

private:
    // Deques containing data sample pointers.
    std::deque<SmurfSampleConstPtr> write_stash_, read_stash_;
    std::mutex write_stash_lock_, read_stash_lock_;

    uint16_t num_channels_;

    // Puts all stashed data in G3Frame and sends it out.
    void FlushStash();

    // safely swaps read and write stashes
    void SwapStash();

    // Calls FlushStash every agg_duration_ seconds
    static void ProcessStashThread(SmurfBuilder *);

    bool running_;
    bool debug_;

    std::thread process_stash_thread_;

    std::vector<std::string> chan_names_;

    // Keys for the TES bias timestream map
    std::vector<std::string> bias_keys_;

    // Stores current output frame number
    uint32_t out_num_;
};

G3_POINTERS(SmurfBuilder);

#endif
