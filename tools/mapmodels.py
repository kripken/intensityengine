#!/usr/bin/python


#=============================================================================
# Copyright 2008 Alon Zakai ('Kripken') kripkensteiner@gmail.com
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

