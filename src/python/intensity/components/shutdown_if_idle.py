
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
Shuts down the server if no clients are in it, for a while.

Unlike shutdown_if_empty, we do not run until someone enters - if someone
does not enter soon after we load, we will shut down. Another difference is
that we do not immediately shut down after someone leaves, we wait a short
while.
'''

import os, signal, time, threading

from intensity.base import *
from intensity.server.persistence import Clients


def halt_on_excess():
    print '<<< Idle, shutting down >>>'
    time.sleep(1.0) # Let print and message propagate
    os.kill(os.getpid(), signal.SIGKILL)

def watcher():
    consecutives = 0
    while True:
        time.sleep(60.0)
        if Clients.count() > 0:
            consecutives = 0
        else:
            print '<<< Warning: Idling (%d) >>>' % consecutives
            consecutives += 1
            if consecutives == 3:
                halt_on_excess()

thread = threading.Thread(target=watcher)
thread.setDaemon(True)
thread.start()

