
# Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
# This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

import os, sys, unittest


if __name__ == '__main__':
    print "Importing test cases..."
    from standalone import *

    print "Running tests..."
    unittest.main()

