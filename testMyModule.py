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
import DataWriter
import pyrogue
import time

from FpgaTopLevel import *


base = pyrogue.Root(name='AMCc', description='')

base.add(FpgaTopLevel(
    simGui = False,
    commType = "eth-rssi-interleaved",
    ipAddr = "192.168.2.20",
    pcieRssiLink = 0
    ))

rx = DataWriter.DataWriter()
pyrogue.streamConnect(base.FpgaTopLevel.stream.application(0xC1),rx)

base.start(pollEn=True)
try:
    while (True):
       print(" Rx: Count {}, Bytes {}, Last {}".format(rx.getCount(), rx.getBytes(), rx.getLast()))
       time.sleep(1)

except KeyboardInterrupt:
    # print ("Stopping rogue...")
    rx.endFile()
    pass
