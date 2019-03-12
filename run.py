#!/usr/bin/env python3
#-----------------------------------------------------------------------------
# Title      : Test file for MyModule
#-----------------------------------------------------------------------------
# File       : exoTest.py
# Created    : 2018-02-28
#-----------------------------------------------------------------------------
# This file is part of the rogue_example software. It is subject to
# the license terms in the LICENSE.txt file found in the top-level directory
# of this distribution and at:
#    https://confluence.slac.stanford.edu/display/ppareg/LICENSE.html.
# No part of the rogue_example software, including this file, may be
# copied, modified, propagated, or distributed except according to the terms
# contained in the LICENSE.txt file.
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
    "--out",
    type     = str,
    required = False,
    default  = 'test.g3',
    help     = "File where data is written",
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
    listener.run()
except KeyboardInterrupt:
    print("Stopping due to keyboard interrupt...")
    listener.end_file()
    base.stop()
