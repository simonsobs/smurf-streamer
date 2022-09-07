from spt3g import core

import rogue
import pyrogue.gui
import argparse
import sosmurf
import sys
import yaml
import os

import pysmurf.core.devices
import pysmurf.core.server_scripts.Common as pysmurf_common
import shlex
from pysmurf.core.roots.EmulationRoot import EmulationRoot


print(f"PID: {os.getpid()}")

stream_root = sosmurf.StreamBase(
    "SOStream", debug_meta=False, debug_data=False, agg_time=2.0
)
file_writer = sosmurf.SOFileWriter("SOFileWriter", g3_dir, file_dur=10*60)
stream_root.add(file_writer)

pipe = core.G3Pipeline()
pipe.Add(stream_root.builder)
pipe.Add(sosmurf.SessionManager.SessionManager, stream_id="emulator")
pipe.Add(core.G3NetworkSender, hostname='*', port=4532, max_queue_size=1000)
pipe.Add(file_writer.rotator)


with EmulationRoot():
    pipe.Run()
    
print("HERE")