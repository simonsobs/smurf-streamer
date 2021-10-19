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

    G3FramePtr FrameFromSamples(
            std::deque<SmurfSampleConstPtr>::iterator start,
            std::deque<SmurfSampleConstPtr>::iterator stop);

    float agg_duration_; // Aggregation duration in seconds
    void SetAggDuration(float dur){ agg_duration_ = dur;};
    const float GetAggDuration(){ return agg_duration_; };

    void       setDebug(bool d) { debug_ = d;    };
    const bool getDebug()       { return debug_; };

    void setEncode(bool b) { encode_timestreams_ = b; };
    const bool getEncode() {return encode_timestreams_;};

    void setDataEncodeAlgo(int algo);
    int getDataEncodeAlgo() const;

    void setPrimaryEncodeAlgo(int algo);
    int getPrimaryEncodeAlgo() const;

    void setTesBiasEncodeAlgo(int algo);
    int getTesBiasEncodeAlgo() const;

    void setTimeEncodeAlgo(int algo);
    int getTimeEncodeAlgo() const;

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

    // Sets SuperTimestream encoding algorithms:
    //   - 0: No compression
    //   - 1: FLAC only
    //   - 2: bzip only
    //   - 3: FLAC + bzip
    // See So3G docs for more info.
    int data_encode_algo_;
    int primary_encode_algo_;
    int tes_bias_encode_algo_;
    int time_encode_algo_;

    bool running_;
    bool debug_;
    bool encode_timestreams_;

    std::thread process_stash_thread_;

    std::vector<std::string> chan_names_;

    // Keys for the TES bias timestream map
    std::vector<std::string> bias_keys_;

    // Stores current output frame number
    uint32_t out_num_;
};

G3_POINTERS(SmurfBuilder);

#endif
