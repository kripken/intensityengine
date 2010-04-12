
# Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
# This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

log(logging.DEBUG, "Python server system initializing")

import time, threading


#
# Database/persistence functionalities
#

from intensity.server.persistence import *


#
# Interactive interpreter command console, threaded
#

## Show some help. TODO: improve.
##
class Help:
    def __repr__(self):
        if LINUX:
            hotkey = "Ctrl-D"
        elif WINDOWS:
            hotkey = "Ctrl-Z, then Enter"
        else:
            hotkey = "?"
            
        print """
Basic Commands:

    stats()          - Show number of remote clients, bandwidth usage, etc.
    autostats(delay) - Continuously show statistics, every 'delay' seconds
                       (if not given, delay is equal to 2.0)
                       If already running, another call to autostats() will stop autostat

    CModule.run_script(script) - Run a JavaScript command in the scripting engine

    quit() - Shut down the server (hotkey: %s)""" % (hotkey)
        return ""

help = Help()

from intensity.console import *

console_thread = ConsoleThread()
console_thread.safe_console = SafeConsole(__main__.__dict__)
console_thread.setDaemon(True) # Do not stop quitting when the main thread quits


# Statistics

stats = CModule.show_server_stats

class AutostatsThread(threading.Thread):
    instance = None

    def run(self):
        stats()
        while not self.stop:
            time.sleep(self.delay)
            if not self.stop:
                stats()
        AutostatsThread.instance = None

def autostats(delay=2.0):
    assert(delay > 0.04)
    if AutostatsThread.instance is None:
        thread = AutostatsThread()
        AutostatsThread.instance = thread

        thread.delay = delay
        thread.stop = False
        thread.setDaemon(True)
        thread.start()
    else:
        AutostatsThread.instance.stop = True
        while AutostatsThread.instance is not None:
            time.sleep(0.1)

