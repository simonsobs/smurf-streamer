#!/usr/bin/env python3

import pyrogue
import smurf
import sosmurfcore

class StreamBase(pyrogue.Device):
    """
        Stream Base pyrogue object.
    """
    def __init__(self, name, debug_data=False, debug_meta=False, agg_time=1.0, **kwargs):
        pyrogue.Device.__init__(self, name=name, description='SMuRF Data CustomTransmitter', **kwargs)

        self.builder = sosmurfcore.SmurfBuilder()
        self._transmitter = sosmurfcore.SmurfTransmitter(self.builder, debug_data, debug_meta)

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

        self.add(pyrogue.LocalVariable(
            name='AggTime',
            description='Sets the time [sec] for which the builder will listen '
                        'for samples before publishing to a G3Frame',
            mode='RW',
            value=float(agg_time),
            localSet=lambda value: self.builder.SetAggDuration(value),
            localGet=self.builder.GetAggDuration))


    def getDataChannel(self):
        return self._transmitter.getDataChannel()

    def getMetaChannel(self):
        return self._transmitter.getMetaChannel()
