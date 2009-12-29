
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


import glob, os, shutil, stat, sys


target_dir = sys.argv[1]
tarball = sys.argv[2] == 'tarball' if len(sys.argv) >= 3 else False

print "Creating in:", target_dir
print "tarball?", tarball
print

SYNTENSITY = True

EXEC = '<exec>'
RECURSE = '<recurse>'

needed_globs = [
    'intensity*.py',
    'README.txt', 'TROUBLESHOOTING.txt', 'LICENSE.txt', 'mybrushes.cfg', 'README-standalone.txt', 'COMPILE.txt',
    'data/*',
    'CMakeLists.txt',
    'src/CMakeLists.txt',
    'src/enet/CMakeLists.txt',
    'src/client/CMakeLists.txt',
    'src/server/CMakeLists.txt',
    'local/client/settings.cfg',
    'local/server/settings.cfg',
    'local/master_server/settings.cfg',
    'local/master_server/templates/*.html',
    'master_django/*.py',
    'master_django/intensity/*.py',
    'master_django/intensity/register/*.py',
    'master_django/intensity/components/*.py',
    'master_django/intensity/components/templates/*.html',
    'master_django/intensity/middleware/*.py',
    'master_django/intensity/static/*',
    'master_django/intensity/templates/*.html',
    'master_django/intensity/templates/registration/*.html',
    'master_django/intensity/templatetags/*.py',
    'master_django/intensity/tracker/*.py',
    'master_django/intensity/tracker/fixtures/*.py',
    'master_django/intensity/tracker/fixtures/*.json',
    'master_django/intensity/tracker/fixtures/*.tar.gz',
    'master_django/intensity/tracker/templates/tracker/*.html',
    'packages/*',
    'packages/brushes/*',
    'packages/caustics/*',
    'packages/cloward/*',
    'packages/freeseamless/*',
    'packages/golgotha/*',
    'packages/gor/*',
    'packages/hud/*',
    'packages/icons/*',
    'packages/library/*',
    'packages/library/modes/*',
    'packages/library/guns/*',
    'packages/materials/*',
    'packages/models/areatrigger/*',
    'packages/models/cannon/*',
    'packages/models/cannon/barrel/*',
    'packages/models/cannon/base/*',
    'packages/models/debris/*',
    RECURSE + 'packages/models/flag/*',
    'packages/models/frankie/*',
    'packages/models/frankie/alt/*',
    'packages/models/gibc/*',
    'packages/models/gibh/*',
    'packages/models/invisiblegeneric/*',
    'packages/models/invisiblegeneric/ellipse/*',
    'packages/models/nut/*',
    'packages/models/platform/*',
    RECURSE + 'packages/models/stromar/*',
    'packages/models/tree/*',
    'packages/music/*',
    'packages/particles/*',
    'packages/skyboxes/mayhem/*',
    'packages/skyboxes/philo/*',
    'packages/sounds/0ad/*',
    'packages/sounds/gk/*',
    'packages/sounds/olpc/*',
    'packages/sounds/olpc/AdamKeshen/*',
    'packages/sounds/olpc/Berklee44BoulangerFX/*',
    'packages/sounds/olpc/FlavioGaete/*',
    'packages/sounds/olpc/MichaelBierylo/*',
    'packages/sounds/olpc/NilsVanOttorloo/*',
    'packages/sounds/yo_frankie/*',
    'packages/tomek/*',
    'packages/yo_frankie/*',
    'src/*',
    'src/enet/*',
    'src/enet/include/*',
    'src/enet/include/enet/*',
    'src/engine/*',
    'src/fpsgame/*',
    'src/intensity/*',
    'src/javascript/*',
    'src/javascript/intensity/*',
    'src/python/_dispatch/*',
    'src/python/intensity/*',
    'src/python/intensity/client/*',
    'src/python/intensity/server/*',
    RECURSE + 'src/python/intensity/components/*',
    'src/shared/*',
#    'src/thirdparty/openjpeg/license.txt',
    'src/thirdparty/v8/LICENSE',
    'src/thirdparty/SDL.txt',
    'syntensity/client/settings.cfg',
    'syntensity/server/settings.cfg',
]        

# Syntensity-specific stuff
if not tarball and SYNTENSITY:
    needed_globs += [
        RECURSE + 'data/initial_packages/*'
    ]


if 'linux' in sys.platform:
    print "  on Linux"
    LINUX = True
    WINDOWS = False
elif 'win' in sys.platform:
    print "  on Windows"
    WINDOWS = True
    LINUX = False
else:
    print "Unknown platform %s" % sys.platform

if not tarball:
    if LINUX:
        needed_globs += [
            EXEC + 'cbuild/src/client/Intensity*',
            EXEC + 'cbuild/src/server/Intensity*',
        ]
    else:
        needed_globs += [
            EXEC + 'cbuild/src/client/Release/Intensity*',
#            EXEC + 'cbuild/src/server/Release/Intensity*',
        ]

if LINUX:
    needed_globs += [
        EXEC + 'intensity*.sh',
    ]

    if not tarball:
        needed_globs += [
#            EXEC + 'build/openjpeg/*.so',
        ]
elif WINDOWS:
    needed_globs += [
        EXEC + 'intensity*.bat',
#        EXEC + 'build/openjpeg/*.dll',
#        EXEC + 'build/openjpeg/*.manifest',
        EXEC + 'windows/dll/*',
        RECURSE + 'Python25/*',
    ]


while len(needed_globs) > 0:
    temp_needed_globs = needed_globs
    needed_globs = []
    for needed_glob in temp_needed_globs:
        needed_glob = needed_glob.replace('/', os.path.sep)
        print ' + ' + needed_glob

        if needed_glob[:len(EXEC):] == EXEC:
            needed_glob = needed_glob.replace(EXEC, '')
            make_exec = True
        else:
            make_exec = False

        if needed_glob[:len(RECURSE):] == RECURSE:
            print '  (recursing)'
            needed_glob = needed_glob.replace(RECURSE, '')
            recurse = True
        else:
            recurse = False

        files = glob.glob(needed_glob)
        assert(len(files) > 0) # No missing globs, which might be a typo here or missing file on filesystem
        actual_files = 0
        for filename in files:
            if os.path.isdir(filename):
                if recurse:
                    needed_globs += [RECURSE + filename + '/*']
                    actual_files += 1
                continue

            actual_files += 1
            if filename[-1] == '~' or filename[-4:] == '.BAK': continue
            print ' |-- ' + filename
            dirname = os.path.join(target_dir, os.path.dirname(filename))
            if not os.path.exists(dirname):
                os.makedirs(os.path.join(target_dir, os.path.dirname(filename)))
            full_filename = os.path.join(target_dir, filename)
            shutil.copyfile(filename, full_filename)
            if make_exec:
                os.chmod(full_filename, os.stat(full_filename).st_mode | stat.S_IXUSR)
        assert(actual_files > 0)

print
print "Finished"

