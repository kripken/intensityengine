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
Given a directory with textures in standard naming convention,
autogenerates .js files: A separate file for each texture
(loading all parts of that texture - normal, specmap, etc.),
and a package.js which runs them all for those that want all
the textures in the directory (aka 'texture pack').

Notes:
    * The textures should be in their position in packages/
        so that the tool can find their paths. So, you
        should be running the tool with something like

            python tools/create_tex_config.py packages/textures/USERNAME/TEXPACKAGENAME/


Naming conventions:

_cc = clour
_sc = spec
_nm =normal map
_hm = high map
_si = glow self illumination
-oc = ambient occlusion

as DXT 5 DDS combined

_cc
RGB = _cc
A = _sc

_nm

RGB = _nm
A = _hm

as DXT 1 DDS

_sc
RGB = _sc

as DXT 1 DDS

_si
RGB = _si

as DXT 1 DDS

_oc
RGB = _oc
'''

import os, sys

def relativize(path):
    '''
    Relativizes relative to 'packages'
    So /some/path/packages/textures/nicepack/5.jpg ==> textures/nicepack/5.jpg
    '''
    parts = path.split(os.path.sep)
    package_index = parts.index('packages')
    return os.path.sep.join(parts[package_index+1:])

# syntensity/client/packages/textures/gk/brick2,syntensity/client/packages/textures/gk/deco2,syntensity/client/packages/textures/gk/walls2,syntensity/client/packages/textures/gk/maya2,syntensity/client/packages/textures/gk/morter2,syntensity/client/packages/textures/gk/roof2,syntensity/client/packages/textures/gk/concrete2,syntensity/client/packages/textures/gk/wall2,syntensity/client/packages/textures/gk/wood2,syntensity/client/packages/textures/gk/ground2,syntensity/client/packages/textures/gk/metal2,syntensity/client/packages/textures/gk/rock2,syntensity/client/packages/textures/gk/stone2

directories = sys.argv[1]
try:
    shader_params = sys.argv[2]
except:
    shader_params = '''
Map.setShaderParam("specscale", 3.6, 3.6, 3.6);
Map.setShaderParam("parallaxscale", 0.02, -0.03);
Map.setShaderParam("specmap", 4, 4, 4);
'''

if ',' in directories:
    directories = directories.split(',')
else:
    directories = [directories]

extensions = ['png', 'jpg', 'jp2']

for directory in directories:
    created_cfgs = set()

    for name in os.listdir(directory):
        parts = name.split('.')
        corename = '.'.join(parts[:-1])
        extension = parts[-1]
        if extension in extensions and corename[-3:] == '_cc':
            if extension == 'jp2':
                # jpeg2000 files will be converted into PNG and DDS files after download
                # Tell cfg <dds>, and write a cfg file for a PNG which will exist at that time
                jp2 = True
                dds_text = '<dds>'
                extension = 'png'
            else:
                jp2 = False
                dds_text = '<dds>' # Doesn't hurt to try?
                # If there is also a jp2, create a config for that file, not this png/dds
                # (that cfg will say <dds>, refer to the png, etc.)
                if corename + '.jp2' in os.listdir(directory):
                    continue

            corename = corename[:-3] # Remove '_cc'

            # This is a valid file, which deserves a cfg
            cfgname = os.path.join(directory, corename + '.js')

            def check_exists(suffix):
                return reduce(
                    lambda x, y: x or y,
                    map(lambda extension: os.path.exists(os.path.join(directory, corename + '_' + suffix + '.' + extension)), extensions)
                )

            # Decide shader
            has_spec = True
            has_env = True
            has_specmap, has_normal, has_parallax, has_glow, has_aoc = map(check_exists, ['sc', 'nm', 'hm', 'si', 'oc'])

            has_alpha_specmap = has_alpha_parallax = False # These may be in the alpha channels - assume they are NOT there XXX
            has_specmap = has_specmap or has_alpha_specmap
            has_parallax = has_parallax or has_alpha_parallax

            if not has_normal:
                if not has_glow:
                    shader = 'stdworld'
                else:
                    shader = 'glowworld'
            else:
                if not has_spec:
                    if not has_parallax:
                        if not has_glow:
                            shader = 'bumpworld'
                        else:
                            shader = 'bumpglowworld'
                    else:
                        if not has_glow:
                            shader = 'bumpparallaxworld'
                        else:
                            shader = 'bumpparallaxglowworld'
                else:
                    if not has_specmap:
                        if not has_parallax:
                            if not has_glow:
                                shader = 'bumpspecworld'
                            else:
                                shader = 'bumpspecglowworld'
                        else:
                            if not has_glow:
                                shader = 'bumpspecparallaxworld'
                            else:
                                shader = 'bumpspecparallaxglowworld'
                    else:
                        if not has_parallax:
                            if not has_glow:
                                shader = 'bumpspecmapworld'
                            else:
                                shader = 'bumpspecmapglowworld'
                        else:
                            if not has_glow:
                                shader = 'bumpspecmapparallaxworld'
                            else:
                                shader = 'bumpspecmapparallaxglowworld'

            if has_env:
                shader = shader.replace('bump', 'bumpenv')

            def relativize_with_suffix(suffix, force_extension=None, not_exist_extension=extension):
                _dds_text = dds_text if suffix != 'si' else ''

                if force_extension is not None:
                    test = os.path.join(directory, corename + '_' + suffix + '.' + force_extension)
                    return (_dds_text, relativize(test))

                for extension in extensions:
                    test = os.path.join(directory, corename + '_' + suffix + '.' + extension)
                    if os.path.exists(test):
    #                if extension == 'jp2':
    #                    test = os.path.join(directory, corename + '_' + suffix + '.png')
                        return (_dds_text, relativize(test))

                test = os.path.join(directory, corename + '_' + suffix + '.' + not_exist_extension)
                return (_dds_text, relativize(test))

            outfile = open(cfgname, 'w')

            outfile.write('''// Automatically generated by tools/create_tex_config.py\n\n''')

            outfile.write('''Map.setShader('%s');\n''' % shader)

            outfile.write(shader_params)

            outfile.write('''\n''')

            ROTS = [0, 1]
            scale = 0.5

            for rot in ROTS:
                scaling_rotation = (', ' + str(rot) + ', 0, 0, ' + str(scale),)
                def do_combination(primary, secondary, texcode, sec_texcode, secondary_exists):
                    if jp2:
                        outfile.write('''Map.convertJP2toPNG('%s');\n''' % relativize_with_suffix(primary, 'jp2')[1])

                    if secondary_exists: # and not has_alpha_secondary XXX
                        if jp2:
                            outfile.write('''Map.convertJP2toPNG('%s');\n''' % relativize_with_suffix(secondary, 'jp2')[1])

        #                outfile.write('''\nMap.combineImages('%s', '%s', '%s');\n''' % (
        #                    relativize_with_suffix(primary, extension)[1],
        #                    relativize_with_suffix(secondary, extension)[1],
        #                    relativize_with_suffix(primary + secondary, extension)[1],
        #                ))

        #                outfile.write('''Map.convertPNGtoDDS('%s');\n''' % relativize_with_suffix(primary + secondary)[1])
        #                outfile.write('''Map.texture('%s', '%s%s');\n\n''' % ((texcode,) + relativize_with_suffix(primary + secondary, extension)))
                        outfile.write('''Map.texture('%s', '%s%s'%s);\n''' % ((texcode,) + relativize_with_suffix(primary, extension) + scaling_rotation))
                        outfile.write('''Map.texture('%s', '%s'%s);\n''' % ((sec_texcode,) + relativize_with_suffix(secondary, extension)[1:] + scaling_rotation))
                    else:
        #                outfile.write('''Map.convertPNGtoDDS('%s');\n''' % relativize_with_suffix(primary, extension)[1])
        #                outfile.write('''Map.texture('%s', '%s%s');\n\n''' % ((texcode,) + relativize_with_suffix(primary, extension)))
                        outfile.write('''Map.texture('%s', '%s%s'%s);\n\n''' % ((texcode,) + relativize_with_suffix(primary, extension) + scaling_rotation))


                do_combination('cc', 'sc', '0', 's', has_specmap)

                if has_normal:
                    do_combination('nm', 'hm', 'n', 'z', has_parallax)

                if has_glow:
                    do_combination('si', '', 'g', '', False)

                outfile.write('''\n''')

            outfile.close()

            created_cfgs.add(relativize(cfgname))

    #

    # Create big cfg to run all the small ones 
    outfile = open(os.path.join(directory, 'Package.js'), 'w')
    for created_cfg in created_cfgs:
        outfile.write('''Library.include('%s');\n''' % created_cfg)
    outfile.close()

    print 'Completed %s successfully' % directory

