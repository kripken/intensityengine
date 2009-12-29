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

import sys

filename = sys.argv[1]
output = filename + ".js"

outfile = open(output, 'w')

for line in open(filename, 'r'):
    line = line.strip()
    if len(line)>2:
        line = line.replace('"', '')
        diff = 1
        while diff != 0:
            diff = len(line)
            line = line.replace('  ', ' ')
            diff -= len(line)

        line = line.replace('attack ', 'attack') # hack

        line = line.split(" ")
        command = line[0]
        num_quoted_params = 2
        if command == 'setshader': command = 'Map.setShader'
        if command == 'texture': command = 'Map.texture'
        if command == 'setshaderparam':
            command = 'Map.setShaderParam'
            num_quoted_params = 1
        elif command == 'texlayer':
            command = 'Map.texLayer'
            num_quoted_params = 0
        elif command == 'mdlscale':
            command = 'Model.scale'
            num_quoted_params = 0
        elif command == 'mdlspec':
            command = 'Model.spec'
            num_quoted_params = 0
        elif command == 'mdlglow':
            command = 'Model.glow'
            num_quoted_params = 0
        elif command == 'mdlellipsecollide':
            command = 'Model.ellipseCollide'
            num_quoted_params = 0
        elif command == 'mdlglare':
            command = 'Model.glare'
            num_quoted_params = 0
        elif command == 'mdlshader':
            command = 'Model.shader'
        elif command == 'mdlambient':
            command = 'Model.ambient'
            num_quoted_params = 0
        elif command == 'mdlbb':
            command = 'Model.bb'
        elif command == 'md5load':
            command = 'Model.md5Load'
        elif command == 'md5tag':
            command = 'Model.md5Tag'
        elif command == 'md5skin':
            command = 'Model.md5Skin'
            num_quoted_params = 3
        elif command == 'md5bumpmap':
            command = 'Model.md5Bumpmap'
        elif command == 'md5envmap':
            command = 'Model.md5Envmap'
        elif command == 'md5anim':
            command = 'Model.md5Anim'
        elif command == 'md5animpart':
            command = 'Model.md5Animpart'
        elif command == 'md5pitch':
            command = 'Model.md5Pitch'
        elif command == 'rdvert':
            command = 'Model.rdVert'
            num_quoted_params = 0
        elif command == 'rdtri':
            command = 'Model.rdTri'
            num_quoted_params = 0
        elif command == 'rdjoint':
            command = 'Model.rdJoint'
            num_quoted_params = 0
        elif command == 'rdlimitdist':
            command = 'Model.rdLimitDist'
            num_quoted_params = 0
        elif command == 'rdlimitrot':
            command = 'Model.rdLimitRot'
            num_quoted_params = 0
        elif command == 'objload':
            command = 'Model.objLoad'
        elif command == 'objskin':
            command = 'Model.objSkin'
            num_quoted_params = 3
        elif command == 'objbumpmap':
            command = 'Model.objBumpmap'
        elif command == 'objenvmap':
            command = 'Model.objEnvmap'
        elif command == 'exec':
            command = 'Library.include'
            line = map(lambda item: item.replace('.cfg', '.js'), line)

        quoted_params = line[1:num_quoted_params+1]
        params = line[num_quoted_params+1:]
        params = map(lambda param: ('0' + param) if len(param) > 0 and param[0] == '.' else param, params)

        out = command + '(' + ', '.join(['"' + p + '"' for p in quoted_params] + params) + ');'

#        out = out.replace('attack', 'attack ') # reverse hack

        print out
        outfile.write(out + '\n')
    else:
        print
        outfile.write('\n')

outfile.close()

