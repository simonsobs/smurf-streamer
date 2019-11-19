#!/usr/bin/env python3

# Python wrapper for smurf-streammer module

import pyrogue
import smurf
import sosmurfcore

class SmurfTransmitter(pyrogue.Device):
    """
    Smurf Transmitter python wrapper
    """
    def __init__(self, builder, name, debug_data=False, debug_meta=False, **kwargs):
        pyrogue.Device.__init__(self, name=name, description='SMuRF Data CustomTransmitter', **kwargs)

        self._transmitter = sosmurfcore.SmurfTransmitter(builder, debug_data, debug_meta)

        # Add pyrogue variables here!!
        self.add(pyrogue.LocalVariable(
            name='DebugData',
            description='Set the debug mode, for the data',
            mode='RW',
            value=debug_data,
            localSet=lambda value: self._transmitter.setDebugData(value),
            localGet=self._transmitter.getDebugData))

        # Add a variable for the debugMeta flag
        self.add(pyrogue.LocalVariable(
            name='DebugMeta',
            description='Set the debug mode, for the metadata',
            mode='RW',
            value=debug_meta,
            localSet=lambda value: self._transmitter.setDebugMeta(value),
            localGet=self._transmitter.getDebugMeta))

    def getDataChannel(self):
        return self._transmitter.getDataChannel()

    def getMetaChannel(self):
        return self._transmitter.getMetaChannel()
