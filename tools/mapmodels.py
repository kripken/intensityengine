#!/usr/bin/python

# Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
# This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

'''
Usage: mapmodels.py [raw-entities-file] [map.cfg]

raw-entities-file is the output when you load a map with
entities, it is a raw JSON dump of the entities. attr2 is the mapmodel
index, which we will convert.

map.cfg is the file that defines the mapmodels, using the mmodel command
'''

import sys

mapmodel_filename = sys.argv[2]
mapmodel_file = open(mapmodel_filename, 'r')
mapmodels = []
for line in mapmodel_file:
    line = line.strip()
    if 'mmodel' in line:
        line = line.replace('  ', ' ')
        mapmodels.append(line.split(' ')[1].replace('"', ''))
mapmodel_file.close()

def convert_mapmodel(index):
    return mapmodels[int(index)]

filename = sys.argv[1]
output = filename + ".fixed"
outfile = open(output, 'w')
outfile.write('[\n')
for line in open(filename, 'r'):
    line = line.strip()
    if len(line)>2:
        line = eval(line)[0]
        if 'Mapmodel' in line:
            line[2]['modelName'] = convert_mapmodel(line[2]['attr2'])
        outfile.write('  ' + str(line) + ',\n')
outfile.write(']\n')
outfile.close()

