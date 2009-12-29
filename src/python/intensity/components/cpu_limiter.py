
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

