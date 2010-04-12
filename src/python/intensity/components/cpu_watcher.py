
# Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
# This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

'''
Watches that CPU usage doesn't exceed certain limits.

Useful when running untrusted code in sandboxed systems, to
prevent them from maxxing CPU to the detriment of other processes.
'''

import os, signal, time, threading

from intensity.base import *


def halt_on_excess():
    print '<<<! CPU usage is excessive, killing the process !>>>'
    main_actionqueue.add_action(lambda: show_client_message(ALL_CLIENTS, "Server CPU Monitor", "Shutting down due to excessive CPU use"))
    time.sleep(5.0) # Let print and message propagate
    os.kill(os.getpid(), signal.SIGKILL)

def watcher():
    resolution = float(get_config('CPUWatcher', 'resolution', '1.0'))
    threshold = float(get_config('CPUWatcher', 'threshold', '0.9')) # 90% CPU
    consecutives_to_act = int(get_config('CPUWatcher', 'consecutives_to_act', '5')) # How many consecutive threshold passings before acting
    action = get_config('CPUWatcher', 'action', 'halt_on_excess()')

    last_clock = time.clock()
    last_time = time.time()
    consecutives = 0
    while True:
        time.sleep(resolution)
        cpu_usage = (time.clock() - last_clock)/(time.time() - last_time)

        # Logic
        if cpu_usage >= threshold:
            consecutives += 1
        else:
            consecutives = 0
        if consecutives >= consecutives_to_act:
            exec(action)

        last_clock = time.clock()
        last_time = time.time()

thread = threading.Thread(target=watcher)
thread.setDaemon(True)
thread.start()

