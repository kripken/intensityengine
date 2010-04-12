
# Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
# This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

'''
'''

import os, signal, time, threading, subprocess

from intensity.base import *


def get_memory_usage():
    '''Returns memory usage of current process, in MB'''
    process = subprocess.Popen(
        "ps -o rss %d" % os.getpid(),
        shell=True,
        stdout=subprocess.PIPE,
    )
    return float(process.communicate()[0].split('\n')[1])/1024.

def halt_on_excess():
    print '<<<! Memory usage is excessive (%f MB), killing the process !>>>' % get_memory_usage()
    main_actionqueue.add_action(lambda: show_client_message(ALL_CLIENTS, "Server Memory Monitor", "Shutting down due to excessive memory use"))
    time.sleep(5.0) # Let print and message propagate
    os.kill(os.getpid(), signal.SIGKILL)

def watcher():
    resolution = float(get_config('MemoryWatcher', 'resolution', '1.0'))
    threshold = float(get_config('MemoryWatcher', 'threshold', '40')) # 40MB
    consecutives_to_act = int(get_config('MemoryWatcher', 'consecutives_to_act', '1')) # How many consecutive threshold passings before acting
    action = get_config('MemoryWatcher', 'action', 'halt_on_excess()')

    consecutives = 0
    while True:
        time.sleep(resolution)
        if get_memory_usage() >= threshold:
            consecutives += 1
        else:
            consecutives = 0
        if consecutives >= consecutives_to_act:
            exec(action)


thread = threading.Thread(target=watcher)
thread.setDaemon(True)
thread.start()

