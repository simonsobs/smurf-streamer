#!/usr/bin/env python3

# Python wrapper for smurf-streammer module

import pyrogue
import smurf
import sosmurfcore

class SmurfTransmitter(pyrogue.Device):
    """
    Smurf Transmitter python wrapper
    """
    def __init__(self, name, debug=False, **kwargs):
        pyrogue.Device.__init__(self, name=name, description='SMuRF G3 Streamer', **kwargs)

        self._transmitter = sosmurfcore.SmurfTransmitter(debug)

        # Add pyrogue variables here!!
        self.add(pyrogue.LocalVariable(
            name='Debug',
            description='Set the debug mode',
            mode='RW',
            value=debug,
            localSet=lambda value: self._transmitter.setDebug(value),
            localGet=self._transmitter.getDebug
        ))

    def _getStreamSlave(self):
        return self._transmitter
