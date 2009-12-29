
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

