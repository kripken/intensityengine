"""
A simple logging system with variable levels of logging detail. This module is a thin
wrapper around our C++ Logging module.
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

