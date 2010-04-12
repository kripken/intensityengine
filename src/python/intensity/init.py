
# Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
# This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

"""
Initialization of the system, in Python.
The most basic stuff, that needs to be done first thing, on both client and server.
"""

# Logging

from intensity.logging import *


log(logging.DEBUG, "Python system initializing")

import os, shutil


## Runs the startup script defined in the config file. This is done on loading, and
## might be done later if the application file is changed.
def run_startup_script():
    script_name = get_config('Startup', 'script', '')
    if len(script_name) > 0:
        execfile(script_name, __main__.__dict__, __main__.__dict__ )

#*#
#*# Shared stuff for client and server
#*#

from intensity.errors import *
from intensity.message_system import *
from intensity.asset import *
from intensity.safe_actionqueue import *
from intensity.world import *
from intensity.master import *
from intensity.signals import *

