from spt3g import core

import rogue
import argparse
import sosmurf
import sys
import yaml
import os

import pysmurf.core.devices
import pysmurf.core.server_scripts.Common as pysmurf_common
import shlex
from pysmurf.core.roots.EmulationRoot import EmulationRoot

g3_dir = "/tmp"
stream_rate = 500.0  # Hz

print(f"PID: {os.getpid()}")

stream_root = sosmurf.StreamBase(
    "SOStream", debug_meta=True, debug_data=False, agg_time=2.0
)
file_writer = sosmurf.SOFileWriter("SOFileWriter", g3_dir, file_dur=10)
stream_root.add(file_writer)

pipe = core.G3Pipeline()
pipe.Add(stream_root.builder)
pipe.Add(sosmurf.SessionManager.SessionManager, stream_id="emulator")
pipe.Add(core.G3NetworkSender, hostname='*', port=4532, max_queue_size=1000)
pipe.Add(file_writer.rotator)


with EmulationRoot(server_port=9000, txDevice=stream_root) as root:
    print(f'  Streaming data at {stream_rate} Hz... ', end='')
    root.StreamDataSource.SourceEnable.set(True)
    root.StreamDataSource.Period.set(1/stream_rate)
    # Open the output data file
    print('  Opening the FileWriter output file... ', end='')
    root.SmurfProcessor.FileWriter.DataFile.set('/tmp/test_emu.dat')
    root.SmurfProcessor.FileWriter.Open()

    pipe.Run()

print("HERE")
