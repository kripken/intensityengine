#!/usr/bin/python

# Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
# This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.


'''
This script can automate a lot of the texture preparation steps that can
take a lot of time, if you have a great many images.

It has been tested on Linux, but should work anywhere, perhaps with
minor modifications to the external/commandline tasks that are run.

The script doesn't accept parameters - you need to edit the settings
inside the file. See comments below, and the example settings. The
only commandline parameter is the directory in which to work, i.e. that
contains the input files (and will contain the output files).

The script has the following tools it can apply:

    PIL: Convert file formats, using the Python Imaging Library (PIL). Can
        scale images and convert to grayscale. Keeps suffixes, so if
        _cc.X means a primary diffuse texture, can convert *_cc.png to
        *_cc.jpg, etc. Can also combine images into single images with
        multiple layers, e.g., normal map + heightmap into an image with
        the heightmap in the alpha channel.

    NV: NVidia texture tools. Can convert PNG to DDS.

    OJ: OpenJpeg. Can convert TIFF to JP2 or vice versa.

    PM: PythonMagick (ImageMagick). Can convert file formats as well.
        http://www.graphicsmagick.org/Magick++/Image.html

    PACK: Packaging into assets. Gets a glob (wildcard) of which files
        go into which asset. Subparameters:
            COPY: Copy files into a directory structure suitable for
                    the asset
            CFG: Run tools/create_tex_config.py to create .js config
                    files for the asset. Note that that tool assumes
                    standard suffixes etc. for autodetection.
            BUNDLE: Create an archive asset suitable for the Intensity
                    engine asset system.

    VIEW: Shows the files, for debugging purposes

The appropriate tools used (NVidia texture tools, PIL, etc.) must be
installed.

After the line

    tasks = []

below, you can add as many tasks as you want. They will be done in
order. This lets you prepare a lot of things to do in batch. To add
tasks, use lines like

    tasks += ...

for the details, see the example parameters below.

The example parameters below are the ones that were actually used to
generate the syntensity assets, so they should be useful. The procedure
was basically to pop them into and out of comments, to decide what
to run at any particular time. So, to get started, first comment out
whatever tasks additions are currently not in comments.

Note that there are two sets of example tasks: The first ones, and
then some more after 'Phase 1_TAKE_B'. The former are the various
phases for an earlier packaging method, the latter is the current
approach, which is recommended.
'''

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

