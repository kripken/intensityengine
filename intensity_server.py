#!/usr/bin/python

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

from __future__ import with_statement

import os
import sys

from intensity.base import *
Global.init_as_server()

import intensity.c_module
CModule = intensity.c_module.CModule.holder


###
### Start
###

# Begin loading

print "Intensity Engine Server parameters:", sys.argv

execfile( os.path.join(PYTHON_SCRIPT_DIR, "init.py") )

print "Initializing C Server's Python connection system"

import __main__

print "Setting home dir"

home_dir = None
try:
    home_dir = sys.argv[1] if sys.argv[1][0] != '-' else None
except IndexError:
    print "Note: No home directory specified, so using default (which is tied to this operating-system level user)"
if home_dir is not None:
    set_home_dir(home_dir)

print "Initializing config"

config_filename = os.path.join( get_home_subdir(), 'settings.cfg' )
# Allow the server to run in the client home dir, to share the assets. This is done
# by using settings_server.cfg instead of settings.cfg (which the client uses).
# This option is enabled if you do NOT provide a home dir, i.e., if you use the
# default. In other words, the default is to share the home dir with the client
# (but specifying that same home dir will not work, as both will use settings.cfg).
if home_dir is None:
    config_filename = os.path.join( get_home_subdir(), 'settings_server.cfg' )

template_filename = os.path.join( os.getcwd(), 'data', 'server_settings_template.cfg' )

# Offer an interactive setup wizard, if relevant
import intensity.server.wizard as wizard
wizard.ask(config_filename, template_filename, sys.argv)

print 'Config filename:', config_filename

init_config(config_filename, template_filename)

load_components()

print "Initializing logging"

CModule.init_logging()

print "Testing for local mode"

from intensity.server.auth import check_local_mode
if check_local_mode():
    print "<<< Server is running in local mode - only a single client from this machine can connect >>>"

print "Generating client/server specific code"

print "Initializing server"

execfile( os.path.join(PYTHON_SCRIPT_DIR, "server", "server_init.py") )

print "Initializing CModule"

CModule.init()
CModule.set_home_dir( get_home_subdir() )

# Utilities

from intensity.utility import *

# Run application-specific startup script.

run_startup_script()

# Start admin listener - AFTER checking last shutdown (so out first master update will be in standby, if necessary)
from intensity.server.admin_listener import *

# Start server slicing and main loop

print "Preparing timing and running first slice"

NETWORK_RATE = float(get_config("Network", "rate", 33))/1000.0
MIN_DELAY    = 0.01 # If this is too big - 0.1 is too big - then we get jerky motion with many NPCs. Probably
                    # because 0.1 is a long wait between updates (10Hz instead of 30Hz)
                    # But if this is too small - 0.0 is too small - then the interactive console is not responsive

CModule.slice()  # Do a single time slice

print "Network rate:", NETWORK_RATE, "MIN_DELAY:", MIN_DELAY

print "Running main server with parallel interactive console"

console_thread.start()

PROFILE = False

if PROFILE:
    import hotshot, hotshot.stats
    prof = hotshot.Profile("server_profile")

# Updates to master

MASTER_UPDATE_INTERVAL = float(get_config("Network", "master_update_rate", 300))

last_master_update = 0

# Main loop

def main_loop():
    try:
        last_time = time.time()
        while not should_quit():

            # Sleep just long enough for the network rate to be ok, with a minimum so that the interactive console is responsive.
            while time.time() - last_time < NETWORK_RATE: # For some reason Python 'stutters' in timekeeping, so need a loop, not a single oprt n.
                delay = max( NETWORK_RATE - (time.time() - last_time) , MIN_DELAY )
    #            print "Sleeping:", delay
                time.sleep(delay)
            assert(time.time() - last_time >= NETWORK_RATE - 0.0001) # 0.0001 for potential rounding errors

            # TODO: In the future, might just run CModule.slice() in a separate thread in order to get responsiveness for interactive console
            # Would need to be a *real* thread, not a CPython one
            last_time = time.time()

            if not should_quit(): # If during the sleep a quit was requested, do this immediately, do not slice any more

                # We do this safely, so as to never work in parallel with the interactive console; see comments in SafeConsole (ser ver_init.py)
                with slicing_console_lock:
                    CModule.slice()  # Do a single time slice

            # Update master, if necessary TODO: If we add more such things, create a modular plugin system

            global last_master_update

            if time.time() - last_master_update >= MASTER_UPDATE_INTERVAL:
                last_master_update = time.time()

                def do_update_master():
                    auth.update_master()

                # Update master in the side thread, but with it's results - set_map, etc. - in the main queue
                side_actionqueue.add_action(do_update_master)

            # Run queued actions

            main_actionqueue.execute_all()
    except KeyboardInterrupt:
        pass # Just exit gracefully


# end main loop

if PROFILE:
    prof.runcall(main_loop)
    prof.close()
else:
    main_loop()

print "Stopping main server"

execfile( os.path.join(PYTHON_SCRIPT_DIR, "quit.py") )

if PROFILE:
    stats = hotshot.stats.load("server_profile")
    stats.strip_dirs()
    stats.sort_stats('time', 'calls')
    stats.print_stats(20)

