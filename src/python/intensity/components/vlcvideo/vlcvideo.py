
# Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
# This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

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

