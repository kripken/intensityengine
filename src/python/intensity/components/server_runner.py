
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

import subprocess, time
import os, signal # Python 2.5 killing method, see below

from intensity.base import *
from intensity.logging import *
from intensity.signals import shutdown, show_components
from intensity.asset import AssetMetadata


class Module:
    server_proc = None

def get_output_file():
    return os.path.join(get_home_subdir(), 'out_server.txt')

def run_server(location=None):
    CModule.run_cubescript('echo "Starting server, please wait..."')

    if location is not None:
        try:
            location = AssetMetadata.get_by_path('packages/base/'+location+'.tar.gz').asset_id
        except Exception, e:
            log(logging.ERROR, "Error in getting asset info for map: %s" % location)
            return

    Module.server_proc = subprocess.Popen(
        "%s %s %s -component:intensity.components.shutdown_if_idle" % (
            'exec ./intensity_server.sh' if UNIX else 'intensity_server.bat',
            '-config:Activity:force_activity_id:' if location is not None else '',
            ('-config:Activity:force_map_asset_id:%s' % location) if location is not None else '',
        ),
        shell=True,
        stdout=open(get_output_file(), 'w'),
        stderr=subprocess.STDOUT,
    )
    #process.communicate()
    Module.server_proc.connected_to = False
    log(logging.WARNING, "Starting server process: %d" % Module.server_proc.pid)

    def prepare_to_connect():
        while True:
            time.sleep(1.0)
            if check_server_ready():
                def do_connect():
                    assert(not Module.server_proc.connected_to)
                    Module.server_proc.connected_to = True
                    CModule.run_cubescript('connect 127.0.0.1 28787') # XXX: hard-coded
                main_actionqueue.add_action(do_connect)
                break
    side_actionqueue.add_action(prepare_to_connect)

def has_server():
    return Module.server_proc is not None

# Check if the server is ready to be connected to
def check_server_ready():
    INDICATOR = '[[MAP LOADING]] - Success'
    return INDICATOR in open(get_output_file(), 'r').read()

def check_server_terminated():
    return Module.server_proc.poll()

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
        if check_server_ready():
            CModule.run_cubescript('guitext "Local server: Running"')
            CModule.run_cubescript('guibutton "  stop" [ (run_python "intensity.components.server_runner.stop_server()") ]')
        elif check_server_terminated():
            Module.server_proc = None
            log(logging.ERROR, "Server output: %s" % open(get_output_file(), 'r').read()) # XXX Show in GUI? last few lines at least?
        else:
            CModule.run_cubescript('guitext "Local server: ...preparing..."')
    else:
        CModule.run_cubescript('''
            if ( = $logged_into_master 1 ) [
                guitext "Local server: (not active)"
                guilist [
                    guitext "Map location to run: base/"
                    guifield local_server_location 30 []
                    guitext ".tar.gz"
                ]
                guibutton "  start" [ (run_python (format "intensity.components.server_runner.run_server('%1')" $local_server_location)) ]
            ] [
                guitext "Local server: (need master login)"
            ]
        ''')
    CModule.run_cubescript('guibar')

show_components.connect(show_gui, weak=False)

