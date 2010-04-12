
# Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
# This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

'''
Uses the Linux 'cpulimit' command to limit CPU usage.

'''

import os, subprocess
import signal # Python 2.5 killing method, see below

from intensity.base import get_config
from intensity.signals import shutdown


max_cpu = int( get_config('CPULimiter', 'limit', '50') )

process = subprocess.Popen(
    "exec cpulimit -p %d -l %d" % (os.getpid(), max_cpu), # exec is useful so a second process is not spawned
    shell=True,
    stdout=subprocess.PIPE,
)
#process.communicate()

def terminate(sender, **kwargs):
    os.kill(process.pid, signal.SIGKILL)
    process.wait()
# Or, in Python 2.6:   process.terminate()

shutdown.connect(terminate, weak=False)

