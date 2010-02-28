
#=============================================================================
# Copyright (C) 2010 Alon Zakai ('Kripken') kripkensteiner@gmail.com
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

'''
Runs a server in a side process in a convenient way, for local gameplay.
'''

import subprocess
import os, signal # Python 2.5 killing method, see below

from intensity.signals import shutdown


class Module:
    server_proc = None

def run_server():
    Module.server_proc = subprocess.Popen(
        "exec intensity_server.py -component:intensity.components.shutdown_if_idle",
        shell=True,
        stdout=subprocess.PIPE,
    )
    #process.communicate()

def terminate_server():
    if Module.server_proc is not None:
        os.kill(Module.server_proc.pid, signal.SIGKILL)
        process.wait()
        # Or, in Python 2.6:   process.terminate()
        Module.server_proc = None

shutdown.connect(terminate_server, weak=False)

