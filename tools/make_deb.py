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


# Create a simple .DEB.
#
# Typically you would first run package_for_release.py, into some directory.
# Then call this with that directory, and a deb will be created.
#

import os, sys, tempfile, tarfile, shutil, subprocess

# Params

deb_dir = sys.argv[1] # Where the deb config files are
files_dir = sys.argv[2] # Where the syntensity build is (output of package_for_release.py)

# Prepare

temp_dir = tempfile.mkdtemp()

os.makedirs(os.path.join(temp_dir, 'DEBIAN'))
os.makedirs(os.path.join(temp_dir, 'usr', 'bin'))
os.makedirs(os.path.join(temp_dir, 'usr', 'share', 'applications'))
os.makedirs(os.path.join(temp_dir, 'usr', 'share', 'pixmaps'))

# Create control

shutil.copyfile(os.path.join(deb_dir, 'control', 'control'), os.path.join(temp_dir, 'DEBIAN', 'control'))
shutil.copyfile(os.path.join(deb_dir, 'control', 'postinst'), os.path.join(temp_dir, 'DEBIAN', 'postinst'))
subprocess.call(['chmod 0555 %s' % os.path.join(temp_dir, 'DEBIAN', 'postinst')], shell=True)

# Create data

files = [ # general files
    ['usr', 'bin', 'syntensity'],
    ['usr', 'share', 'applications', 'syntensity.desktop'],
    ['usr', 'share', 'pixmaps', 'syntensity.png'],
]
for _file in files:
    joined = os.path.join(*_file)
    shutil.copyfile(os.path.join(deb_dir, 'data', joined), os.path.join(temp_dir, joined))

def add_files(location):
    full = os.path.join(files_dir, location)
    for _file in os.listdir(full): # build files
        if _file[-1] == '~': continue
        joined = os.path.join(location, _file)
        full_joined = os.path.join(full, _file)
        if os.path.isdir(full_joined):
            add_files(joined)
        else:
            dest = os.path.join(temp_dir, 'usr', 'share', 'games', 'syntensity', joined)
            if not os.path.exists(os.path.dirname(dest)):
                os.makedirs(os.path.dirname(dest))
            shutil.copyfile(full_joined, dest)
add_files('')

# Combine into deb

command = 'dpkg -b %s new_deb.deb' % temp_dir
subprocess.call([command], shell=True)

# Clean up

shutil.rmtree(temp_dir)

