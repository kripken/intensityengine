
# Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
# This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

#
# Version
#

INTENSITY_VERSION_STRING = '1.1.5'

def comparable_version(version_string):
    if version_string == '': return (0,)
    return tuple(map(int, version_string.split('.')))

INTENSITY_VERSION = comparable_version(INTENSITY_VERSION_STRING)

def check_version(version_string, strict=False):
    version = comparable_version(version_string)
    if version == INTENSITY_VERSION: return True
    return (not strict) and version > INTENSITY_VERSION

