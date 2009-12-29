
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


import time
import libmodule

from intensity.base import *
from intensity.logging import *


libmodule.init()

#libmodule.quit()


def pusher():
    while True:
        time.sleep(0.01)
        def action():
            pixels = libmodule.get_pixels()
            if pixels != 0:
                CModule.upload_texture_data("packages/models/videoscreen/512/blank.jpg", 0, 64, 512, 384, pixels)
        main_actionqueue.add_action(action)

thread = threading.Thread(target=pusher)
thread.setDaemon(True)
thread.start()

