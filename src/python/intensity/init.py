"""
Initialization of the system, in Python.
The most basic stuff, that needs to be done first thing, on both client and server.
"""


#=============================================================================
# Copyright (C) 2008 Alon Zakai ('Kripken') kripkensteiner@gmail.com
#
# This file is part of the Intensity Engine project,
#    http://www.intensityengine.com
#
# The Intensity Engine is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as published by
# the Free Software Foundation, version 3.
#
# The Intensity Engine is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with the Intensity Engine.  If not, see
#     http://www.gnu.org/licenses/
#     http://www.gnu.org/licenses/agpl-3.0.html
#=============================================================================


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

