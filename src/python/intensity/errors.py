
# Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
# This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

"""
Exception classes for our system.
"""

# Exceptions

## The basic expection class for Intensity Engine-specific errors
class IntensityError(Exception):
    def __init__(self, contents):
        self.contents = contents

    def __str__(self):
        return self.contents

class IntensityUserCancellation(IntensityError):
    pass

