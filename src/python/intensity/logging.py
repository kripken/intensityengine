
# Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
# This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

"""
A simple logging system with variable levels of logging detail. This module is a thin
wrapper around our C++ Logging module.
"""

import c_module
CModule = c_module.CModule.holder

## Storage for logging constants.
class logging: # Must be synchronized with logging.h/cpp
    INFO    = 0
    DEBUG   = 1
    WARNING = 2
    ERROR   = 3
    OFF     = 4
    strings = ['INFO', 'DEBUG', 'WARNING', 'ERROR', 'OFF']

    @staticmethod
    def should_show(level):
        return CModule.should_show(level)

#curr_level = logging.DEBUG # TODO : load from config

## Write a message to the log.
## @param level The level of this message. If the current logging level allows this message level to be shown, it will, otherwise not.
## For example, if this is a WARNING, and we are running at a current level of INFO, then we would show INFO and all more serious
## messages, and in particular this WARNING one.
## @param message The content of the message, in string form.
def log(level, message):
    CModule.log(level, message)

