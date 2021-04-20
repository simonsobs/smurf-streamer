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

        # Add "Disable" variable
        self.add(pyrogue.LocalVariable(
            name='Disable',
            description='Disable the processing block. Data will just pass thorough to the next slave.',
            mode='RW',
            value=False,
            localSet=lambda value: self._transmitter.setDisable(value),
            localGet=self._transmitter.getDisable))

        # Add the data dropped counter variable
        self.add(pyrogue.LocalVariable(
            name='dataDropCnt',
            description='Number of data frame dropped',
            mode='RO',
            value=0,
            pollInterval=1,
            localGet=self._transmitter.getDataDropCnt))

        # Add the metaData dropped counter variable
        self.add(pyrogue.LocalVariable(
            name='metaDropCnt',
            description='Number of metadata frame dropped',
            mode='RO',
            value=0,
            pollInterval=1,
            localGet=self._transmitter.getMetaDropCnt))

        # Command to clear all the counters
        self.add(pyrogue.LocalCommand(
            name='clearCnt',
            description='Clear all counters',
            function=self._transmitter.clearCnt))

    def getDataChannel(self):
        return self._transmitter.getDataChannel()

    def getMetaChannel(self):
        return self._transmitter.getMetaChannel()
