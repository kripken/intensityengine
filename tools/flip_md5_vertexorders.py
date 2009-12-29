#!/usr/bin/python

import sys, os

filename = sys.argv[1]

in_data = open(filename, 'r')
out_data = open(filename + '.flipped', 'w')

for line in in_data:
    if "tri " in line.strip():
        front, index, one, two, three = line.strip().split(" ")
        out_data.write("\t%s %s %s %s %s\n" % (front, index, three, two, one))
    else:
        out_data.write(line)

out_data.close()
in_data.close()

