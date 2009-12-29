#!/usr/bin/python

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


#
## Main SConstruct file
#

import os
import sys

from src.build_shared import *


# Options

AddOption('--windowsSDKs', dest='windowsSDKs', nargs=1, default='', help='Comma separated locations of Windows Platform SDK and Windows compiler (we will use Include and Lib under these locations). Order matters - place the Platform SDK first. Tested with with Windows Platform SDK and VC++ 2008 Express')

windowsSDKs = GetOption('windowsSDKs')
if windowsSDKs != '':
    print "Using Windows SDKs at", windowsSDKs
#    sys.path += [ os.path.join(windowsPlatformSDK, 'Include'), os.path.join(windowsPlatformSDK, 'Lib') ]
#    os.environ["PATH"] += ';"' + os.path.join(windowsPlatformSDK, 'Include') + '";"' + os.path.join(windowsPlatformSDK, 'Lib') + '"'

AddOption('--v8', dest='v8', nargs=1, default='no', help='Whether to try to build V8 (do "no" to speed up compilation once V8 is already built')
do_v8 = GetOption('v8') == 'yes'


####################

print "\n<<< Intensity Engine build procedure >>>\n"

help = GetOption('help')

if not help:

    ####################

    do_v8 = do_v8 or not os.path.exists( get_v8_lib() )

    if do_v8:
        print "--== Building Google V8 ==--"

        curr_dir = os.getcwd()
        os.chdir( os.path.join(curr_dir, 'src', 'thirdparty', 'v8') )
        add = ''
        prefix = ''
        if WINDOWS:
            print "WARNING: Using V8 parameters only tested with VS2008 thus far"
            SDKs = windowsSDKs.split(",")
            add += 'env="INCLUDE:'
            for SDK in SDKs:
                add += os.path.join(SDK.replace('"', ''), 'Include') + ";"
            add += ",LIB:"
            for SDK in SDKs:
                add += os.path.join(SDK.replace('"', ''), 'Lib') + ";"
            add += '"'
        elif LINUX:
            if 'GCC 4.4' in sys.version:
                prefix = '''GCC_VERSION="44" '''
        command = '%sscons mode=release %s' % (prefix, add) # FIXME Make parameter?
        print "V8 scons command:", command
        print "Running V8 build procedure, please wait..."
        output = os.popen(command)
        for line in output:
            print line
        output.close()
        os.chdir(curr_dir)

    ####################

    print "\n--== Building OpenJPEG ==--\n"

    SConscript('src/thirdparty/openjpeg/SConscript', build_dir='build/openjpeg', duplicate=0)

    ####################

    print "\n--== Building Intensity Engine itself ==--\n"

    SConscript('src/SConscript', build_dir='build', duplicate=0)

