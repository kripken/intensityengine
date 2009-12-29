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


# Using Python Imaging Library, or PythonMagick (Python ImageMagick; in ubuntu repos; http://www.graphicsmagick.org/Magick++/Image.html)
# or OpenJPEG codec (assumed binary is copied to here)

import os, sys
import fnmatch

# Tools: PIL, PM, OJ or NV

tasks = []


#===================================


# Phase 1: png ==> tif, with scaling and grayscaling (need tif for OJ)
#TOOL = 'PIL' ; INPUT_FORMATS = '_cc.png' ; SECONDARY_FORMAT = '_sc.png' ; OUTPUT_FORMAT = '_cc.tif' ; SCALE_DOWN = None ; QUALITY = None
#tasks += [{ 'TOOL': 'PIL', 'INPUT_FORMATS': '*_cc.png', 'OUTPUT_FORMAT': '_cc.tif' }]
#tasks += [{ 'TOOL': 'PIL', 'INPUT_FORMATS': '*_sc.png', 'OUTPUT_FORMAT': '_sc.tif', 'SCALE_DOWN': 4, 'FORCE_GRAYSCALE': True }]
#tasks += [{ 'TOOL': 'PIL', 'INPUT_FORMATS': '*_nm.png', 'OUTPUT_FORMAT': '_nm.tif', 'SCALE_DOWN': 2 }]
#tasks += [{ 'TOOL': 'PIL', 'INPUT_FORMATS': '*_hm.png', 'OUTPUT_FORMAT': '_hm.tif', 'SCALE_DOWN': 8, 'FORCE_GRAYSCALE': True }]

############## Phase 2: combine nm.png and hm.png ==> nm2_tif, at 50% size
##############TOOL = 'PIL' ; INPUT_FORMATS = '*_nm.png' ; SECONDARY_FORMAT = '_hm.png' ; OUTPUT_FORMAT = '_nm2.tif' ; SCALE_DOWN = 2 ; QUALITY = None

# Phase 3: tif ==> jp2
#tasks += [{ 'TOOL': 'OJ', 'INPUT_FORMATS': '*_cc.tif', 'OUTPUT_FORMAT': '_cc.jp2', 'COMPRESSION': 40, 'COMMAND': 'image_to_j2k' }]
#tasks += [{ 'TOOL': 'OJ', 'INPUT_FORMATS': '*_sc.tif', 'OUTPUT_FORMAT': '_sc.jp2', 'COMPRESSION': 40, 'COMMAND': 'image_to_j2k' }]
#tasks += [{ 'TOOL': 'OJ', 'INPUT_FORMATS': '*_nm.tif', 'OUTPUT_FORMAT': '_nm.jp2', 'COMPRESSION': 40, 'COMMAND': 'image_to_j2k' }]
#tasks += [{ 'TOOL': 'OJ', 'INPUT_FORMATS': '*_hm.tif', 'OUTPUT_FORMAT': '_hm.jp2', 'COMPRESSION': 40, 'COMMAND': 'image_to_j2k' }]

### Tests: jp2 ==> tif
#tasks += [{ 'TOOL': 'OJ', 'INPUT_FORMATS': '*_cc.jp2', 'OUTPUT_FORMAT': '_cc_test.tif', 'COMMAND': 'j2k_to_image' }]
#tasks += [{ 'TOOL': 'OJ', 'INPUT_FORMATS': '*_sc.jp2', 'OUTPUT_FORMAT': '_sc_test.tif', 'COMMAND': 'j2k_to_image' }]
#tasks += [{ 'TOOL': 'OJ', 'INPUT_FORMATS': '*_nm.jp2', 'OUTPUT_FORMAT': '_nm_test.tif', 'COMMAND': 'j2k_to_image' }]
#tasks += [{ 'TOOL': 'OJ', 'INPUT_FORMATS': '*_hm.jp2', 'OUTPUT_FORMAT': '_hm_test.tif', 'COMMAND': 'j2k_to_image' }]

### Tests: view output pairs of original (before any manipulation) and final output of the pipeline
#tasks += [{ 'TOOL': 'VIEW', 'INPUT_FORMATS': '*_cc.png', 'OUTPUT_FORMAT': '_cc_test.tif' }]
#tasks += [{ 'TOOL': 'VIEW', 'INPUT_FORMATS': '*_sc.png', 'OUTPUT_FORMAT': '_sc_test.tif', 'CHANCE': 0.2 }]
#tasks += [{ 'TOOL': 'VIEW', 'INPUT_FORMATS': '*_nm.png', 'OUTPUT_FORMAT': '_nm_test.tif', 'CHANCE': 0.2 }]
#tasks += [{ 'TOOL': 'VIEW', 'INPUT_FORMATS': '*_hm.png', 'OUTPUT_FORMAT': '_hm_test.tif', 'CHANCE': 0.2 }]


####TOOL = 'PM' ; INPUT_FORMATS = '*_cc.jp2' ; OUTPUT_FORMAT = '_cc3.png' ; SCALE_DOWN = None ; QUALITY = None # Testing
####TOOL = 'PIL'; INPUT_FORMATS = '*_cc.png' ; SECONDARY_FORMAT = None ; OUTPUT_FORMAT = '_cc2.png' ; SCALE_DOWN = None ; QUALITY = None#Tst

# Phase 1_TAKE_B: Original PNGs into 1/2 size jpgs (no combining)
#tasks += [{ 'TOOL': 'PIL', 'INPUT_FORMATS': '*_cc.png', 'OUTPUT_FORMAT': '_cc.jpg', 'SCALE_DOWN': 1, 'QUALITY': 85 }]
#tasks += [{ 'TOOL': 'PIL', 'INPUT_FORMATS': '*_sc.png', 'OUTPUT_FORMAT': '_sc.jpg', 'SCALE_DOWN': 1, 'FORCE_GRAYSCALE': True, 'QUALITY': 85 }]
#tasks += [{ 'TOOL': 'PIL', 'INPUT_FORMATS': '*_nm.png', 'OUTPUT_FORMAT': '_nm.jpg', 'SCALE_DOWN': 1, 'QUALITY': 85 }]
#tasks += [{ 'TOOL': 'PIL', 'INPUT_FORMATS': '*_hm.png', 'OUTPUT_FORMAT': '_hm.jpg', 'SCALE_DOWN': 1, 'FORCE_GRAYSCALE': True, 'QUALITY': 85 }]
#tasks += [{ 'TOOL': 'PIL', 'INPUT_FORMATS': '*_si.png', 'OUTPUT_FORMAT': '_si.jpg', 'SCALE_DOWN': 1, 'QUALITY': 85 }]

# Phase 4A: For DDS creation, make combined PNGs - half size for now
#tasks += [{ 'TOOL': 'PIL', 'INPUT_FORMATS': '*_cc.png', 'SECONDARY_FORMAT': '_sc.png', 'OUTPUT_FORMAT': '_ccsc.png', 'SCALE_DOWN': 2 }]
#tasks += [{ 'TOOL': 'PIL', 'INPUT_FORMATS': '*_nm.png', 'SECONDARY_FORMAT': '_hm.png', 'OUTPUT_FORMAT': '_nmhm.png', 'SCALE_DOWN': 4 }]

# Phase 4B: DDS creation. Note: cc, not ccsc in output, so can fallback to cc.jpg
#tasks += [{ 'TOOL': 'NV', 'INPUT_FORMATS': '*_ccsc.png', 'OUTPUT_FORMAT': '_cc.dds' }]
#tasks += [{ 'TOOL': 'NV', 'INPUT_FORMATS': '*_nmhm.png', 'OUTPUT_FORMAT': '_nm.dds' }]

# Packaging Phase
SUFF = ['*.jpg', '*.dds']
tasks += [{ 'TOOL': 'PACK', 'INPUT_FORMATS': SUFF, 'GLOB': '*', 'ASSET': 'textures/gk/swarm.tar.gz', 'COPY': 0, 'CFG': 0, 'BUNDLE': 1 }]
'''
tasks += [{ 'TOOL': 'PACK', 'INPUT_FORMATS': SUFF, 'GLOB': 'gk_max_brick*', 'ASSET': 'textures/gk/brick3.tar.gz' }]
tasks += [{ 'TOOL': 'PACK', 'INPUT_FORMATS': SUFF, 'GLOB': 'env_GK_CON*,gk_max_concrete*', 'ASSET': 'textures/gk/concrete3.tar.gz' }]
tasks += [{ 'TOOL': 'PACK', 'INPUT_FORMATS': SUFF, 'GLOB': 'gk_max_deco*', 'ASSET': 'textures/gk/deco3.tar.gz' }]
tasks += [{ 'TOOL': 'PACK', 'INPUT_FORMATS': SUFF, 'GLOB': 'env_GK_GRU*,gk_max_gound*,gk_max_ground*', 'ASSET': 'textures/gk/ground3.tar.gz' }]
#tasks += [{ 'TOOL': 'PACK', 'INPUT_FORMATS': SUFF, 'GLOB': 'gk_max_maya*', 'ASSET': 'textures/gk/maya3.tar.gz' }]
tasks += [{ 'TOOL': 'PACK', 'INPUT_FORMATS': SUFF, 'GLOB': 'gk_max_morter*', 'ASSET': 'textures/gk/morter3.tar.gz' }]
tasks += [{ 'TOOL': 'PACK', 'INPUT_FORMATS': SUFF, 'GLOB': 'env_GK_MET*,gk_max_metal*', 'ASSET': 'textures/gk/metal3.tar.gz' }]
tasks += [{ 'TOOL': 'PACK', 'INPUT_FORMATS': SUFF, 'GLOB': 'env_GK_ROK*', 'ASSET': 'textures/gk/rock3.tar.gz' }]
#tasks += [{ 'TOOL': 'PACK', 'INPUT_FORMATS': SUFF, 'GLOB': 'gk_max_roof*', 'ASSET': 'textures/gk/roof3.tar.gz' }]
#tasks += [{ 'TOOL': 'PACK', 'INPUT_FORMATS': SUFF, 'GLOB': 'env_GK_STO*', 'ASSET': 'textures/gk/stone3.tar.gz' }]
#tasks += [{ 'TOOL': 'PACK', 'INPUT_FORMATS': SUFF, 'GLOB': 'env_GK_WAL*', 'ASSET': 'textures/gk/wall3.tar.gz' }]
tasks += [{ 'TOOL': 'PACK', 'INPUT_FORMATS': SUFF, 'GLOB': 'gk_max_walls*', 'ASSET': 'textures/gk/walls3.tar.gz' }]
tasks += [{ 'TOOL': 'PACK', 'INPUT_FORMATS': SUFF, 'GLOB': 'gk_max_wood*', 'ASSET': 'textures/gk/wood3.tar.gz' }]
'''

#===================================


path = sys.argv[1]
current_dir = os.path.dirname(os.path.abspath(__file__))

for task in tasks:
    TOOL = task['TOOL']
    INPUT_FORMATS = task['INPUT_FORMATS']
    SECONDARY_FORMAT = task.get('SECONDARY_FORMAT')
    OUTPUT_FORMAT = task.get('OUTPUT_FORMAT', '')
    SCALE_DOWN = task.get('SCALE_DOWN')
    QUALITY = task.get('QUALITY')
    COMPRESSION = task.get('COMPRESSION')
    COMMAND = task.get('COMMAND')
    FORCE_GRAYSCALE = task.get('FORCE_GRAYSCALE', False) 

    if TOOL == 'PIL':
        import Image
    elif TOOL == 'PM':
        from PythonMagick import Image
    elif TOOL == 'OJ':
        COMPRESSION = (' -r %d ' % COMPRESSION) if COMPRESSION is not None else ' '
        COMMAND = os.path.join(current_dir, COMMAND) + COMPRESSION
        import subprocess
    elif TOOL == 'VIEW':
        import subprocess, random
        CHANCE = task.get('CHANCE', 1.0)
    elif TOOL == 'NV':
        import subprocess
        COMMAND = os.path.join(current_dir, 'nvcompress') + ' -bc3 ' # bc3 == DXT5 # XXX Disable '-fast'
    elif TOOL == 'PACK':
        import shutil, subprocess, tarfile
        GLOBS = task['GLOB'].split(',')
        ASSET = task['ASSET']
        COPY = task.get('COPY', False)
        CFG = task.get('CFG', False)
        BUNDLE = task.get('BUNDLE', False)
#        SHADER_PARAMS = task.get('SHADER_PARAMS', '')
        asset_dir = os.path.join(path, 'packages', ASSET.replace('.tar.gz', ''))
        if not os.path.exists(asset_dir):
            os.makedirs(asset_dir)
        assert(asset_dir[-1] != '/') # so asset_dir + '.tar.gz' is the asset archive
        archive_files = []
    else:
        assert(0) # "eh?"

    if path[-1] == '/':
        files = os.listdir(path)
        files.sort() # key=lambda name: -os.stat(os.path.join(path, name)).st_size)
        if len(sys.argv) == 3:
            start_from = sys.argv[2]
        else:
            start_from = None
    else:
        files = [path]
        path = ''
        start_from = None

    total = 0

    if type(INPUT_FORMATS) == str:
        INPUT_FORMATS = [INPUT_FORMATS]

    def fit_input_format(name):
        for format in INPUT_FORMATS:
            if fnmatch.fnmatch(name, format): # Allow globs
#            if name[-len(format):] == format:
                return format
        return None

    for name in files:
        if fit_input_format(name) is not None:
            total += 1

    count = 0

    for name in files:
        if start_from is not None and start_from != name:
            continue # If we must start from somewhere, start when we get there
        start_from = None

        INPUT_FORMAT = fit_input_format(name)
        if INPUT_FORMAT is not None:
            print "Converting", name, ' -- ', (100*count)/total, '%', 'Size: %.2f MB' % (os.stat(os.path.join(path, name)).st_size/(1024*1024.))
            count += 1

            # Replace _XX??.YYY with the output format
            start = name.rfind('_')
            output = name[:start] + OUTPUT_FORMAT

            input_filename = os.path.join(path, name)
            output_filename = os.path.join(path, output)

            if TOOL == 'PIL':
                im = Image.open(os.path.join(path, name))

                def is_power_of_2(x):
                    while x > 1:
                        assert(x % 2 == 0)
                        x /= 2
                is_power_of_2(im.size[0])
                is_power_of_2(im.size[1])

                if not FORCE_GRAYSCALE:
                    r, g, b = im.split()[0:3]

                    if SECONDARY_FORMAT is not None:
                        try:
                            # Add in alpha channel
                            im2 = Image.open(os.path.join(path, name.replace(INPUT_FORMAT, SECONDARY_FORMAT)))
                            # Convert im2 to grayscale, if needed:
                            im2 = im2.convert("L")
                            a = im2.split()[0]
                            assert( r.size == g.size == b.size == a.size )
                            im = Image.merge("RGBA", (r, g, b, a))
                        except IOError:
                            print "Skipping secondary for", name
                            im = Image.merge("RGB", (r, g, b)) # If no secondary, at least we removed the alpha channel
                            #assert(0)
                            #continue
                    else:
                        im = Image.merge("RGB", (r, g, b)) # If no secondary, at least we removed the alpha channel
                else:
                    # Force grayscale
                    im = im.convert('L')

                if SCALE_DOWN is not None:
                    size = im.size
                    im = im.resize((size[0]/SCALE_DOWN, size[1]/SCALE_DOWN), Image.BICUBIC)
                if QUALITY is not None:
                    kwargs = {'quality': QUALITY}
                else:
                    kwargs = {}
                im.save(os.path.join(path, output), **kwargs) # 'JPEG' as second param to force JPEG
            elif TOOL == 'PM':
                im = Image()
                im.read(os.path.join(path, name))
                if QUALITY is not None:
                    im.quality(QUALITY)
                try:
                    im.write(os.path.join(path, output))
                except Exception, e:
                    print "ERROR:", str(e)
            elif TOOL == 'OJ': # OpenJPEG
                curr_command = COMMAND + '-i %s -o %s' % (input_filename, output_filename)
                print '    ' + curr_command
                process = subprocess.Popen(
                    curr_command,
                    shell=True,
                    stdout=subprocess.PIPE,
                ).communicate()
            elif TOOL == 'VIEW':
                if random.random() > CHANCE: continue
                # Let the user view the two existing files
                print "Original:", name
                process1 = subprocess.Popen(
                    'eog %s' % input_filename,
                    shell=True,
                    stdout=subprocess.PIPE,
                ).communicate()
                print "Final product"
                process2 = subprocess.Popen(
                    'eog %s' % output_filename,
                    shell=True,
                    stdout=subprocess.PIPE,
                ).communicate()
#                raw_input() # Pause for viewer to form an opinion
            elif TOOL == 'NV': # NVidia
                curr_command = COMMAND + ' ' + input_filename + ' ' + output_filename
                print '    ' + curr_command
                print subprocess.Popen(
                    curr_command,
                    shell=True,
                    stdout=subprocess.PIPE,
                ).communicate()[0]
            elif TOOL == 'PACK':
                # Add to archive asset, if fit at least one glob
                for glob in GLOBS:
                    glob = os.path.join(path, glob)
                    if fnmatch.fnmatch(input_filename, glob):
                        # This file is relevant to our interests
                        in_asset_dir = os.path.join(asset_dir, os.path.basename(input_filename))
                        if COPY:
                            shutil.copyfile(input_filename, in_asset_dir) # TODO: Smart move of some sort, or symlink?
                        archive_files += [in_asset_dir]
                        break

    if TOOL == 'PACK':
        # The files are in the directory. Create a cfg, and then an archive

        if CFG:
            print subprocess.Popen(
                'python tools/create_tex_config.py %s' % (asset_dir,),# SHADER_PARAMS),
                shell=True,
                stdout=subprocess.PIPE,
            ).communicate()[0]

        if BUNDLE:
            # Add js files
            for name in os.listdir(asset_dir):
                if name[-3:] == '.js':
                    archive_files += [os.path.join(asset_dir, name)]

            # Make archive
            zipfile = tarfile.open(asset_dir + '.tar.gz', 'w:gz')
            for archive_file in archive_files:
                zipfile.add(archive_file, arcname = os.path.basename(archive_file))
            zipfile.close()

