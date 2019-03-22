#!/usr/bin/env python3
#-----------------------------------------------------------------------------
# Title      : G3StreamWriter
#-----------------------------------------------------------------------------
# File       : run.py
#-----------------------------------------------------------------------------
#
# This file is to test the G3StreamWriter. It starts communication with the smurf
# over rogue, and streams downsampled data with the G3NetworkSend. This file also
# runs a StreamListener, which reads data from the network and writes it to G3Files
# in a specified location.
#
#-----------------------------------------------------------------------------
import rogue.utilities
import G3StreamWriter
import pyrogue
import time
import argparse
from StreamListener import StreamListener
from FpgaTopLevel import *

parser = argparse.ArgumentParser()

parser.add_argument(
    "--commType",
    type     = str,
    required = False,
    default  = 'eth-rssi-interleaved',
    help     = "Sets the communication type",
)

parser.add_argument(
    "--ipAddr",
    type     = str,
    required = False,
    default  = '192.168.2.20',
    help     = "IP address",
)

parser.add_argument(
    "--port",
    type = int,
    required = False,
    default = 4536,
    help = "Port to write G3Frames",
)

args = parser.parse_args()

base = pyrogue.Root(name='AMCc', description='')

base.add(FpgaTopLevel(
    simGui = False,
    commType = args.commType,
    ipAddr = args.ipAddr,
    pcieRssiLink = 0
    ))


rx = G3StreamWriter.G3StreamWriter(num_samples=1000, max_queue_size=10)
pyrogue.streamConnect(base.FpgaTopLevel.stream.application(0xC1),rx)

base.FpgaTopLevel.AppTop.AppCore.StreamReg.StreamData[0].set(0)

base.start(pollEn=True)

# Listener to read from Stream and write to file. This doesn't have to be run in the
# same function as rogue.
listener = StreamListener(port=args.port, data_dir="data/", time_per_file=60*60)

print("Connected Smurf")
print("Writing G3Frames to port: *:{}".format(args.port))
try:
    while True:
        time.sleep(1)
    #listener.run()
except KeyboardInterrupt:
    print("Stopping due to keyboard interrupt...")
    listener.end_file()
    base.stop()
