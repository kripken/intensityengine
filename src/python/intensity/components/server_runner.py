
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

from intensity.base import *
from intensity.logging import *
from intensity.signals import shutdown, show_components


class Module:
    server_proc = None

def get_output_file():
    return os.path.join(get_home_subdir(), 'out_server.txt')

def run_server():
    Module.server_proc = subprocess.Popen(
        "%s -component:intensity.components.shutdown_if_idle" % ('exec ./intensity_server.sh' if UNIX else 'intensity_server.bat'),
        shell=True,
        stdout=open(get_output_file(), 'w'),
        stderr=subprocess.STDOUT,
    )
    #process.communicate()
    log(logging.WARNING, "Starting server process: %d" % Module.server_proc.pid)

def has_server():
    return Module.server_proc is not None

# If the server terminated, return its output (perhaps to show to the user as a
# crash log). Otherwise, return None
def check_server():
    if Module.server_proc is not None and Module.server_proc.poll():
        Module.server_proc = None
        return open(get_output_file(), 'r').read()
    else:
        return None

def stop_server(sender=None, **kwargs):
    if Module.server_proc is not None:
        log(logging.WARNING, "Stopping server process: %d" % Module.server_proc.pid)
        os.kill(Module.server_proc.pid, signal.SIGKILL)
        Module.server_proc.wait()
        # Or, in Python 2.6:   process.terminate()
        Module.server_proc = None

# Note strictly necessary, as the server will shut down if idle - but why not
# shut it down when the client shuts down.
shutdown.connect(stop_server, weak=False)

def show_gui(sender, **kwargs):
    if has_server():
        CModule.run_cubescript('guitext "Local server: Running"')
        CModule.run_cubescript('guibutton "  stop" [ (run_python "intensity.components.server_runner.stop_server()") ]')
    else:
        CModule.run_cubescript('guitext "Local server: (not active)"')
        CModule.run_cubescript('guibutton "  start" [ (run_python "intensity.components.server_runner.run_server()") ]')
    CModule.run_cubescript('guibar')

show_components.connect(show_gui, weak=False)

