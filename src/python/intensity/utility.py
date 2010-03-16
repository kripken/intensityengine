"""
Various utilities.
"""


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

import os
from math import atan2, pi, sqrt, asin
import hashlib

from intensity.base import *
from intensity.logging import *


## Lets 'print' be used from boost::python as a function. For debugging. Will not be needed in Python 3.0.
def print_func(param):
    print param

## A simple timer that counts down time.
class Timer:
    def __init__(this, seconds):
        this.seconds = seconds

    def tick(this, seconds):
        this.seconds -= seconds
        return this.seconds <= 0


## A timer that resets itthis: after the allotted time to be counted down is done, it starts counting again.
class ResettingTimer:
    """doctest unit testing:

    >>> rt = ResettingTimer(10)
    >>> import time

    >>> rt.tick(4)
    False

    You should not do the following, but it shows what is happening internally:
    >>> rt.seconds
    6

    >>> rt.tick(4)
    False

    >>> rt.tick(4)
    True

    Again, this should not be done, but shows what happened internally:
    >>> rt.seconds
    10

    As you can see, the timer reset itthis.
    """
    ## @param seconds The amount of seconds to count down each time.
    def __init__(this, seconds):
        this.max_seconds = seconds
        this.seconds     = seconds

    ## Increment the time by a certain amount.
    ## @param seconds The amount of seconds to increment the timer by.
    ## @return Whether the counting down has finished. If so, the timer also resets itthis to a new count down.
    def tick(this, seconds):
        this.seconds -= seconds
        if this.seconds <= 0:
            this.seconds = this.max_seconds
            return True
        else:
            return False


## Removes old backup files from a directory, based on their modification times
## @param path The directory to do the cleaning in
## @param suffix The suffix that defines a backup file (other files are ignored) - without the '.' character
## @param to_leave The number of backup files to leave - all others are removed
def clean_up_backups(path, suffix, to_leave):
    names = filter(lambda name: name[-(len(suffix)+1):] == '.' + suffix, os.listdir(path))
    infos = [ (name, os.stat( os.path.join(path, name) ).st_mtime) for name in names ]
    infos.sort(key = lambda pair: pair[1])
    for i in range(len(infos)-to_leave):
#        print os.path.join(path, infos[i][0])
        os.remove( os.path.join(path, infos[i][0]) )


if Global.CLIENT:
    ## CEGUI interface
    def run_lua_function(name):
        CAPI.run_lua_function(name)


def validate_relative_path(path):
    '''
    >>> validate_relative_path('packages/models/cannon/barrel/../skin.jpg')
    True
    >>> validate_relative_path('skin.jpg')
    False
    >>> validate_relative_path('../skin.jpg')
    False
    >>> validate_relative_path('packages/../skin.jpg')
    False
    >>> validate_relative_path('packages/skin.jpg')
    True
    >>> validate_relative_path('packages//../skin.jpg')
    False
    >>> validate_relative_path('packages//skin.jpg')
    True
    >>> validate_relative_path('packages/models/../../skin.jpg')
    False
    >>> validate_relative_path('packages/models/../skin.jpg')
    True
    '''
    path = path.replace('\\', '/') # Use entirely UNIX-style seps to check
    level = -1 # So first addition puts us in 0, a valid level
    for seg in path.split('/'):
        if seg == '..':
            level -= 1
        elif seg != '.' and seg != '':
            level += 1
        if level < 0:
            return False
    level -= 1 # Last segment, the file itself, doesn't count
    return level >= 0


__shown_process_script_warning = False
def process_script(script):
    '''
    Optimize a JavaScript script before we run it.
    We comment out lines that look like log(XXX, ...);
    if XXX would not be shown anyhow. That is, if
    we have
        log(DEBUG, serializeJSON(something));
    then the expensive function call will be done
    even if the output is not shown. So by commenting
    it out, we prevent that.
    '''
    if get_config('Logging', 'optimize_scripts', '1') != '1': return script

    global __shown_process_script_warning
    if not __shown_process_script_warning:
        log(logging.WARNING, 'Optimizing scripts by commenting out unneeded loggings')
        __shown_process_script_warning = True

    ret = []
    for line in script.split('\n'):
        line = line.strip()
        for i in range(len(logging.strings)):
            prefix = 'log(' + logging.strings[i]
            if line[0:len(prefix)] == prefix:
                if line[-2:] == ');':
                    if not logging.should_show(i):
                        line = ''
                else:
                    log(logging.WARNING, 'script optimization not sure about:' + line)
                break
        ret.append(line)

    return '\n'.join(ret)

def check_newer_than(principal, *others):
    '''
    Checks if the file 'principal' is newer than
    various other files. If the principal doesn't
    exist, that is also consider not being newer than.
    '''

    if not os.path.exists(principal):
        return False

    mtime = os.stat(principal).st_mtime
    for other in others:
        if os.stat(other).st_mtime > mtime: # == means nothing to worry about
            return False

    return True

def prepare_texture_replace(filename, curr_textures):
    '''
    Given a file of old textures actually used in a map (from listtex), and a list of current texture names, makes a lookup table.
    The lookup table, given an index in the old textures, returns the new correct index for it using the current textures.

    Method:
        1. Run the old map with the old map.js script
        2. Do /listtex and save the output printed to the console to a file.
            (You may need to remove some unneeded textures at the end.)
        2. Fix the map.js to set up textures the way you want.
        3. Run with that new map.js.
        4. Do /massreplacetex FILENAME with the name of the file you created before
    '''
#    print "CURR:", curr_textures
    old_data = open(filename, 'r').read().replace('\r', '').split('\n')
#    print "OLD:", old_data
    lookup = {}
    for item in old_data:
        if item == '': continue
        index, name = item.split(' : ')
        index = int(index)
#        print "    ", index, name,
        # find new index - the index of that same name in the current textures
        lookup[index] = curr_textures.index(name)
        print lookup[index]
        curr_textures[lookup[index]] = 'zzz' # Do not find this again. This makes it ok to have the same texture twice,
                                                # with different rotations - we will process them ok

#    print "LOOKUP:", lookup
    return lookup


if __name__ == '__main__':
    import doctest
    doctest.testmod()

