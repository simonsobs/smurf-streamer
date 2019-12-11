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

    std::deque<SmurfSampleConstPtr> stash_;
    G3TimeStamp stash_start_time_;
    G3TimeStamp agg_duration_; // Aggregation duration in G3Timestamp units (10 ns).

    std::vector<std::string> chan_names_;

    uint32_t out_num_;


};

G3_POINTERS(SmurfBuilder);

#endif
