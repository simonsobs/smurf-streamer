#ifndef _G3_STREAMCONFIG_H
#define _G3_STREAMCONFIG_H

#include <string>
#include <fstream>
#include <boost/algorithm/string.hpp>

class StreamConfig {
public:
    // Port to stream frames to.
    int port;

    // Amount of aggregation time before sending G3Frame
    float frame_time;

    // Max number of frames to hold in network queue
    float max_queue_size;

    // FLAC compression level for G3Timestreams
    int flac_level;

    StreamConfig(std::string filename){
        std::ifstream cFile (filename);
        if (cFile.is_open()){
            std::string line;

            while(getline(cFile, line)){
                auto delimiterPos = line.find("=");
                auto name = line.substr(0, delimiterPos);
                boost::trim(name);
                auto value = line.substr(delimiterPos + 1);
                boost::trim(value);

                if (name == "port")
                    port =  std::stoi(value);
                else if (name == "frame_time")
                    frame_time = std::stof(value);
                else if (name == "max_queue_size")
                    max_queue_size = std::stoi(value);
                else if (name == "flac_level")
                    flac_level = std::stoi(value);
            }
        }
    };

};
#endif
