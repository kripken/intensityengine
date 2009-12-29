
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


import os

# Offer to interactively set up the settings.cfg

def run(config_filename, template_filename):
    print
    print '======================================'
    print 'Intensity Engine Server - Setup Wizard'
    print '======================================'
    print

    import ConfigParser, re

    if config_filename == template_filename:
        print 'Using default values from your existing settings.cfg. Delete it to use the engine defaults in this wizard.\n'

    # Load

    template = ConfigParser.ConfigParser()
    template.read(template_filename)

    # Ask questions

    def set_activity(value, default):
        if value == '':
            value = default

        if value == '':
            activity_id = ''
            map_asset_id = ''
        else:
            activity_id = value
            map_asset_id = '' # Will be discovered by the server on startup

        # Save them both
        section = 'Activity'
        if not template.has_section(section):
            template.add_section(section)
        template.set(section, 'force_activity_id', activity_id)
        template.set(section, 'force_map_asset_id', map_asset_id)

    QUESTIONS = [
        (
            'Enter the server IP address/hostname (or "localhost" to make it only accessible on the same machine)',
            ('Network', 'address'),
            'localhost',
            None,
        ),
        (
            'Enter the port to listen on',
            ('Network', 'port'),
            '28787',
            None,
        ),
        (
            'Enter the admin port to listen on (for controlling the server externally, like requisitioning)',
            ('Network', 'admin_port'),
            '28789',
            None,
        ),
#        (
#            'Enter the master server to interact with',
#            ('Network', 'master_server'),
#            'www.syntensity.com:8888',
#        ),
        (
            'Enter the activity to run (paste the activity ID, or the URL of the activity - something like "http://www.syntensity.com:8888/tracker/activity/view/.../")',
            ('Activity', 'force_activity_id'),
            '',
            set_activity,
        ),
    ]

    for question in QUESTIONS:
        text = question[0]
        section = question[1][0]
        option = question[1][1]
        default = question[2]
        post = question[3] if len(question) >= 4 else None
        try:
            default = template.get(section, option)
        except:
            pass
        value = raw_input('%s [%s]: ' % (text, default))
        if post is None:
            if value == '': value = default
            if not template.has_section(section):
                template.add_section(section)
            template.set(section, option, value)
        else:
            post(value, default)

    print '\nSetup wizard complete. Writing to: %s' % config_filename

    # Write

    config_file = open(config_filename, 'w')
    template.write(config_file)
    config_file.flush()
    os.fsync(config_file.fileno())
    config_file.close()


def ask(config_filename, template_filename, args):
    show = False
    force = False
    if False:#not os.path.exists(config_filename): # XXX For now, do not show by default - until tested on Windows
        show = True
    else:
        template_filename = config_filename # Start from existing config
    if '--wizard' in args:
        show = True
        force = True

    ran = False
    if show:
        try:
            if force or raw_input('\nRun setup wizard? [Y/n] ').lower() in ['y', '']:
                run(config_filename, template_filename)
                ran = True
        except EOFError:
            pass

    print
    print '======================================================================='
    if not ran:
        print 'Setup wizard not run'
    print 'You can run the setup wizard later, by running the server with --wizard'
    print '======================================================================='
    print

