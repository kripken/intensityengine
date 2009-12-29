
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

import code, threading
try:
    import readline # Makes the interactive console much more pleasant to work with: history, left/right arrows, etc. etc.
except ImportError:
    print "WARNING: Cannot find 'readline' module (perhaps GNU readline is not installed?). Without this, the interactive console will lack history, arrow key movement, etc. On some platforms, this might not be a problem, though (Windows)."

from intensity.base import *


## Lock to prevent console and slicing from working at once, see comments in SafeConsole
slicing_console_lock = threading.Lock()

## A thread-safe version of the Python interactive console. We cannot run in parallel to the main server
## slice command, or we may encounter partially-initialized objects and any manner of other multithreaded
## issues. As the interactive console is just a convenience, not intended for performance work, we simply
## prevent working in parallel. Note that this means scenarios running will stall while a long console
## command is running!
class SafeConsole(code.InteractiveConsole):
    def __init__(self, *args):
        code.InteractiveConsole.__init__(self, *args)

    def runcode(self, compiled_code):
        with slicing_console_lock:
            code.InteractiveConsole.runcode(self, compiled_code)#source, filename, symbol)


## Console interpreter thread, for user iteractivity.
class ConsoleThread(threading.Thread):
    def run(self):
        print "Starting threaded interactive console in parallel"

        filler = "=" * len(INTENSITY_VERSION_STRING)

        self.safe_console.interact(banner="""

=================%s=======
Intensity Engine %s %s
=================%s=======

(for help, type 'help' and press Enter)
""" % (filler, INTENSITY_VERSION_STRING, 'Server' if Global.SERVER else 'Client', filler))

        # XXX FIXME: Windows returns immediately after interact(), oddly, not sure
        # why. So infinite loop to stop server from quitting. No interactive console
        # for now.        
        if WINDOWS or get_config('Startup', 'no_console', '0') == '1':
            try:
                print "Infinite looping"
                while not should_quit():
                    time.sleep(0.25)
                    pass
            except KeyboardInterrupt:
                pass

        print "Interactive console thread shutting down"
        if not should_quit(): quit()

