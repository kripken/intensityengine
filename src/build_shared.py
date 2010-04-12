
# Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
# This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

import os, sys

WINDOWS = sys.platform.find("win32") != -1 or sys.platform.find("win64") != -1 # ??? FIXME
LINUX = sys.platform.find("linux") != -1
OSX = sys.platform.find("osx") != -1 # ??? FIXME
assert(WINDOWS or LINUX or OSX)

def get_v8_lib():
    ret = os.path.join('src', 'thirdparty', 'v8')
    if LINUX:
        return os.path.join(ret, 'libv8.a')
    elif WINDOWS:
        return os.path.join(ret, 'v8.lib')
    else:
        assert(0) # OSX : TODO

