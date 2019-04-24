#ifndef _G3_STREAMCONFIG_H
#define _G3_STREAMCONFIG_H

#include <string>
#include <fstream>
#include <boost/algorithm/string.hpp>

class StreamConfig {
public:
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

    int port;
    float frame_time;
    float max_queue_size;
    int flac_level;
};
#endif
