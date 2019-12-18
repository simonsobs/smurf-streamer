#!/usr/bin/env python3

# Python wrapper for smurf-streammer module

import pyrogue
import smurf
import sosmurfcore

class SmurfStreamer(pyrogue.Device):
    """
    Smurf Streamer python wrapper
    """
    def __init__(self, name, config_file="config.txt", **kwargs):
        pyrogue.Device.__init__(self, name=name, description='SMuRF G3 Streamer', **kwargs)
        self._streamer = sosmurfcore.SmurfStreamer(config_file=config_file)

        # Add pyrogue variables here!!

    def _getStreamSlave(self):
        return self._streamer
