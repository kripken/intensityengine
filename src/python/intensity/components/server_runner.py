
# Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
# This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

'''
Runs a server in a side process in a convenient way, for local gameplay.

Works both when logged into the master, or when not.
'''

import subprocess, time
import os, signal, sys

from intensity.base import *
from intensity.logging import *
from intensity.signals import shutdown, show_components
from intensity.asset import AssetMetadata
from intensity.world import map_load_finish
from intensity.message_system import *


class Module:
    server_proc = None

def get_output_file():
    return os.path.join(get_home_subdir(), 'out_server.txt')

def run_server(location=None, use_master=True):
    CModule.run_cubescript('echo "Starting server, please wait..."')

    if location is not None:
        location = 'base/' + location + '.tar.gz'

    if location is not None and use_master:
        try:
            location = AssetMetadata.get_by_path('packages/' + location).asset_id
        except Exception, e:
            log(logging.ERROR, "Error in getting asset info for map %s: %s" % (location, str(e)))
#            raise
            return

    if use_master:
        activity = '-config:Activity:force_activity_id:' if location is not None else ''
        map_asset = ('-config:Activity:force_map_asset_id:%s' % location) if location is not None else ''
    else:
        activity = ''
        map_asset = '-config:Activity:force_location:%s' % location

    Module.server_proc = subprocess.Popen(
        "%s %s %s %s -component:intensity.components.shutdown_if_idle -components:intensity.components.shutdown_if_empty -config:Startup:no_console:1" % (
            'exec ./intensity_server.sh' if UNIX else 'intensity_server.bat',
            os.path.join(sys.argv[1], 'settings_server.cfg') if sys.argv[1][0] != '-' else '', # Home dir, if given for client - use also in server
            activity,
            map_asset,
        ),
        shell=True,
        stdout=open(get_output_file(), 'w'),
        stderr=subprocess.STDOUT,
    )
    #process.communicate()
    Module.server_proc.connected_to = False
    log(logging.WARNING, "Starting server process: %d" % Module.server_proc.pid)

    def prepare_to_connect():
        success = False
        for i in range(20):
            time.sleep(1.0)
            if not has_server():
                break
            elif check_server_ready():
                success = True
                def do_connect():
                    assert(not Module.server_proc.connected_to)
                    Module.server_proc.connected_to = True
                    CModule.run_cubescript('connect 127.0.0.1 28787') # XXX: hard-coded
                main_actionqueue.add_action(do_connect)
                break
            else:
                CModule.run_cubescript('echo "Waiting for server to finish starting up... (%d)"' % i)
        if not success:
            log(logging.ERROR, "Failed to start server. See out_server.txt")
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
        try:
            if sys.version >= '2.6':
                Module.server_proc.terminate()
            else:
                os.kill(Module.server_proc.pid, signal.SIGKILL) # Will fail on Windows, so must have 2.6 there!
            Module.server_proc.wait()
        except OSError:
            log(logging.ERROR, "Stopping server process failed.");
        # Or, in Python 2.6:   process.terminate()
        Module.server_proc = None

        def do_disconnect():
            CModule.disconnect()
        main_actionqueue.add_action(do_disconnect)

# Note strictly necessary, as the server will shut down if idle - but why not
# shut it down when the client shuts down.
shutdown.connect(stop_server, weak=False)

def show_gui(sender, **kwargs):
    if has_server():
        if check_server_ready():
            CModule.run_cubescript('''
                guitext "Local server: Running"
                guistayopen [
                    guibutton "  stop" [ (run_python "intensity.components.server_runner.stop_server()") ]
                ]
                guibutton "  show output" [ showgui local_server_output ]
                guistayopen [
                    guibutton "  save map" [ do_upload ]
                ]
                guibutton "  restart map" "showgui restart_map"
                guibutton "  editing commands" "showgui editing"
            ''')
        elif check_server_terminated():
            Module.server_proc = None
            log(logging.ERROR, "Local server terminated due to an error")
        else:
            CModule.run_cubescript('''
                guitext "Local server: ...preparing..."
                guistayopen [
                    guibutton "  stop" [ (run_python "intensity.components.server_runner.stop_server()") ]
                ]
            ''')
    else:
        CModule.run_cubescript('''
            guitext "Local server: (not active)"
            if ( = $logged_into_master 0 ) [
                guitext "   << not logged into master >>"
            ]

            guilist [
                guitext "Map location to run: base/"
                guifield local_server_location 30 []
                guitext ".tar.gz"
            ]
            guistayopen [
                guibutton "  start" [
                    if ( = $logged_into_master 1 ) [
                        (run_python (format "intensity.components.server_runner.run_server('%1')" $local_server_location))
                    ] [
                        (run_python (format "intensity.components.server_runner.run_server('%1', False)" $local_server_location))
                    ]
                ]
            ]
            guibutton "  show output" [ showgui local_server_output ]
        ''')
    CModule.run_cubescript('guibar')

show_components.connect(show_gui, weak=False)

# Always enter private edit mode if masterless
def request_private_edit(sender, **kwargs):
    if not CModule.run_cubescript("$logged_into_master"):
        MessageSystem.send(CModule.RequestPrivateEditMode)
map_load_finish.connect(request_private_edit, weak=False)

CModule.run_cubescript('''
    newgui local_server_output [
        guinoautotab [
            guibar
            guieditor "%(name)s" -80 20
            guibar
            guistayopen [
                guibutton "refresh" [textfocus "%(name)s"; textload "%(name)s"; showgui -1]
            ]
        ]
    ]
''' % { 'name': get_output_file() })

