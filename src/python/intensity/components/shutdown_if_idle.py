
# Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
# This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

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

