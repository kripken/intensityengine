"""
An interface for the main C(++) module comprising the C++ portions of our system.

This is loaded before anything else in the Python system. It sets up the
exposing of C++ functions to Python and of simulating commandline arguments
passing to Python as well.
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


import sys

class Holder:
    pass

class CModule:
    ## Users of this module might want to create a shortcut to point to
    ## c_module.CModule.holder to shorten notation, i.e., to do "import c_module;
    ## CModule = c_module.CModule.holder" (see examples in other files for use of this)
    holder = Holder()

# Expose a C function so Python can call it.
def expose_function(python_name, c_func):
    print "Exposing C Function to Python:", python_name
    CModule.holder.__dict__[python_name] = c_func

# Set the python arguments
def set_python_args(args):
    print "Setting Python arguments:", args
    sys.argv = args

