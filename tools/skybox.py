#!/usr/bin/python

# Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
# This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

"""
Generates skybox images from a skydome. Requires the Python Imaging Library
---------------------------------------------------------------------------

Usage:
    python skybox.py [path-to-filename-containing-skydome-to-be-converted]
"""

import sys, os
from math import *

import Image

filename = sys.argv[1]
shortname = os.path.basename( filename.split(".")[-2] )

print "Processing", filename, shortname

try:
    original = Image.open(filename)
except IOError:
    print "File not found"
    sys.exit(1)

size = original.size[0]
assert(size == original.size[1]) # We only work on square images

new_size = 512 # 512x512 textures. Change if desired
half_new_size = new_size/2

# ft, bk, lf, rt, up, dn
areas = [ ["lf", lambda x,y,z: (-x,z,y)],
          ["rt", lambda x,y,z: (x,-z,y)],
          ["ft", lambda x,y,z: (z,x,y)],
          ["bk", lambda x,y,z: (-z,-x,y)],
          ["up", lambda x,y,z: (x,y,z)],
          ["dn", lambda x,y,z: (x,y,-z)],
        ]


for area in areas:
    area_suffix, view = area

    subimage = Image.new("RGB", (new_size,new_size))

    for px in range(new_size):
        for py in range(new_size):

            # Calc cartesian coordinates
            x = float(px - half_new_size + 0.5)/(half_new_size - 0.5)
            y = float(half_new_size - py - 0.5)/(half_new_size - 0.5)
            z = 1.0

            # Transform according to current view
            x,y,z = view(x,y,z)

            # Go to spherical coordinates
            r = sqrt(x**2 + y**2 + z**2)
            t = atan2( sqrt((x**2)+(y**2)), z) # Theta
            p = atan2( y, x )                  # Phi

            darkening = 1-(t/(pi/2)) # Make pitch black at horizon

            if not 0 <= t <= pi/2:
                fuzz = 1-((t-(pi/2))/(pi/2)) # 0 is all the way at the bottom; 1 is at the equator
#                fuzz = fuzz*fuzz
#                fuzz = 0
                t = pi/2 # Go all the way down, this is the closest pixel to us.
            else:
                fuzz = -1

            # Calculate relevant parameters
            dist_from_center = (size/2) * (2.*t/pi)
            angle = p

#            print x,y,z, "==>", r,t,p, "==>", dist_from_center, angle

            opx = (size/2) + (dist_from_center*sin(angle))
            opy = (size/2) + (dist_from_center*cos(angle))

            if opx == size:
                opx = size-1

            if opy == size:
                opy = size-1

#                print opx,opy

            value = original.getpixel((opx,opy))

            if fuzz != -1:
                value = (0,0,0)
#                if fuzz > 0.95:
#                    value = (0,0,0)
#                else:
#                    value = (20,30,50)
#                value = list(value)
#                for i in range(3):
#                    value[i] = fuzz*value[i]
#                value = tuple(value)

#            subimage.putpixel((px, py), value)
            subimage.putpixel((px, py), (value[0]*darkening, value[1]*darkening, value[2]*darkening))

#            else:
#                subimage.putpixel((px, py), (0,0,0))

    subimage.save( os.path.join(os.path.dirname(filename), shortname + "_" + area_suffix + ".jpg") )

