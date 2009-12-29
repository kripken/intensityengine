
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


