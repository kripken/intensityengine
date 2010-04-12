
# Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
# This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

import os, logging, time

log_home_dir = None

def init(home_dir, level):
    global log_home_dir
    if log_home_dir == home_dir: return
    log_home_dir = home_dir

    LOG_FILENAME = os.path.join(home_dir, 'log.' + time.asctime(time.gmtime()).replace(' ', '-').replace(':', '_'))
    LOG_LEVEL = eval('logging.' + level)

    msg = "Initialized logging system to: %s - %s" % (LOG_FILENAME, level)
    print msg

    logger = logging.getLogger()
    logger.setLevel(LOG_LEVEL)

    ch = logging.FileHandler(LOG_FILENAME)
    ch.setLevel(LOG_LEVEL)
    formatter = logging.Formatter("%(asctime)s - %(levelname)s - %(message)s")
    ch.setFormatter(formatter)
    logger.addHandler(ch)


