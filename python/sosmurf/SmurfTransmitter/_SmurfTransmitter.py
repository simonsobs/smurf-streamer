#!/usr/bin/env python3

# Python wrapper for smurf-streammer module

import pyrogue
import smurf
import SmurfStreamer as StreamModule

class SmurfTransmitter(pyrogue.Device):
    """
    Smurf Streamer python wrapper
    """
    def __init__(self, name, config_file="config.txt", **kwargs):
        pyrogue.Device.__init__(self, name=name, description='SMuRF G3 Streamer', **kwargs)
        self._streamer = StreamModule.SmurfStreamer(config_file=config_file)

        # Add pyrogue variables here!!

    def _getStreamSlave(self):
        return self._streamer
