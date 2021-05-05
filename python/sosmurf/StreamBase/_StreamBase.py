#!/usr/bin/env python3

import pyrogue
import smurf
import sosmurfcore

class StreamBase(pyrogue.Device):
    """
        Stream Base pyrogue object.
    """
    def __init__(self, name, debug_data=False, debug_meta=False, debug_builder=False, agg_time=1.0, **kwargs):
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

        # Add a variable for the Builder flag
        self.add(pyrogue.LocalVariable(
            name='DebugBuilder',
            description='Set the debug mode for the Smurfbuilder',
            mode='RW',
            value=debug_builder,
            localSet=lambda value: self.builder.setDebug(value),
            localGet=self.builder.getDebug))

        self.add(pyrogue.LocalVariable(
            name='AggTime',
            description='Sets the time [sec] for which the builder will listen '
                        'for samples before publishing to a G3Frame',
            mode='RW',
            value=float(agg_time),
            localSet=lambda value: self.builder.SetAggDuration(value),
            localGet=self.builder.GetAggDuration))

        # Add "Disable" variable
        self.add(pyrogue.LocalVariable(
            name='Disable',
            description="Disables the SmurfTransmitter, stopping the "
                        "SmurfStreamer from receiving data from upstream",
            mode='RW',
            value=False,
            localSet=lambda value: self._transmitter.setDisable(value),
            localGet=self._transmitter.getDisable))

        # Add the data dropped counter variable
        self.add(pyrogue.LocalVariable(
            name='dataDropCnt',
            description="Number of data frames dropped by the "
                        "SmurfTransmitter's data buffer",
            mode='RO',
            value=0,
            pollInterval=1,
            localGet=self._transmitter.getDataDropCnt))

        # Add the metaData dropped counter variable
        self.add(pyrogue.LocalVariable(
            name='metaDropCnt',
            description="Number of metadata frames dropped by the "
                        "SmurfTransmitter's metadata buffer",
            mode='RO',
            value=0,
            pollInterval=1,
            localGet=self._transmitter.getMetaDropCnt))

        # Command to clear all the counters
        self.add(pyrogue.LocalCommand(
            name='clearCnt',
            description='Clears dataDrop and metaDrop counters',
            function=self._transmitter.clearCnt))

        self.add(pyrogue.LocalVariable(
            name="pysmurf_action",
            description="Current pysmurf action",
            mode='RW',
            value='',
        ))

        self.add(pyrogue.LocalVariable(
            name="pysmurf_action_timestamp",
            description="Current pysmurf action timestamp",
            mode='RW',
            value=0,
        ))

        self.add(pyrogue.LocalVariable(
            name="stream_tag",
            description="Tag associated with data stream",
            mode='RW',
            value='',
        ))


    def getDataChannel(self):
        return self._transmitter.getDataChannel()

    def getMetaChannel(self):
        return self._transmitter.getMetaChannel()
