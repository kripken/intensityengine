
# Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
# This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

"""
An interface for the main C(++) module comprising the C++ portions of our system.

This is loaded before anything else in the Python system. It sets up the
exposing of C++ functions to Python and of simulating commandline arguments
passing to Python as well.
"""

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
#    print "Exposing C Function to Python:", python_name
    CModule.holder.__dict__[python_name] = c_func

# Set the python arguments
def set_python_args(args):
    print "Setting Python arguments:", args
    sys.argv = args

