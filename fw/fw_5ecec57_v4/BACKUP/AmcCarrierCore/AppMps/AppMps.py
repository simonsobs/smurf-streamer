#!/usr/bin/env python
#-----------------------------------------------------------------------------
# Title      : PyRogue MPS Application Module
#-----------------------------------------------------------------------------
# File       : AppMps.py
# Created    : 2017-04-04
#-----------------------------------------------------------------------------
# Description:
# PyRogue MPS Application Module
#-----------------------------------------------------------------------------
# This file is part of the rogue software platform. It is subject to
# the license terms in the LICENSE.txt file found in the top-level directory
# of this distribution and at:
#    https://confluence.slac.stanford.edu/display/ppareg/LICENSE.html.
# No part of the rogue software platform, including this file, may be
# copied, modified, propagated, or distributed except according to the terms
# contained in the LICENSE.txt file.
#-----------------------------------------------------------------------------

import pyrogue as pr
import AmcCarrierCore
import AmcCarrierCore.AppMps

class AppMps(pr.Device):
    def __init__(   self, 
            name        = "AppMps", 
            description = "MPS Application", 
            **kwargs):
        super().__init__(name=name, description=description, **kwargs)

        ##############################
        # Variables
        ##############################

        self.add(AmcCarrierCore.AppMps.AppMpsSalt(
            offset       =  0x00000000,
        ))
        
        self.add(AmcCarrierCore.AppMps.AppMpsThr(
            offset       =  0x00010000,
        ))        
