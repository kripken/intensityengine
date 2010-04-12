
# Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
# This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

import os, sys

import master_django.intensity.conf as intensity_conf

sys.path += [os.path.join(os.getcwd(), 'master_django')]

def show_usage():
    print "Usage: intensity_master.py HOME_DIR [prod | Django manage.py command+arguments]"
    print "    prod: Production mode (using CherryPy)"
    print "    Django manage.py command+arguments: Same as you might pass to Django's manage.py"
    print "        If no command given, default is 'runserver'"
    exit(1)

try:
    HOME_DIR = os.path.abspath(sys.argv[1])
except:
    show_usage()

os.environ[intensity_conf.INTENSITY_HOME_DIR_LABEL] = HOME_DIR
intensity_conf.set_home_dir()

PRODUCTION = False
if len(sys.argv) >= 3:
    if sys.argv[2] == 'prod':
        PRODUCTION = True

os.chdir(os.path.join(os.getcwd(), 'master_django')) # Django likes to run from its root

if PRODUCTION:
    print '[[PRODUCTION]]'
    command = 'prod_server.py'
    command_args = []
else:
    print '[[development]]'
    command = 'manage.py'
    command_args = sys.argv[2:]
    # If no command given, default is to run the server
    if len(command_args) == 0:
        command_args = ['runserver', '%s:%s' % (intensity_conf.get('Network', 'address'), intensity_conf.get('Network', 'port'))]

    from django.core import management

    #sys.path += os.path.split(os.getcwd())[:-1] # Add parent, so Django can import the current directory as if a module
    os.environ['DJANGO_SETTINGS_MODULE'] = intensity_conf.get('Django', 'settings')

    sys.path += ['.']
    from django.conf import settings as django_settings

    # Create DB if it doesn't exist
    if not os.path.exists(django_settings.DATABASE_NAME):
        management.call_command('syncdb')

        # Create user
        want_user = raw_input("Create default user ('test')? [Y/n] ")
        if want_user in ['', 'y', 'Y']:
            print "Creating default user..."
            from django.contrib.auth.models import User
            User.objects.create_user('test', 'test@test.com', 'secret')


args = [sys.executable, command] + command_args
if sys.platform == "win32":
    args = ['"%s"' % arg for arg in args]
os.spawnve(os.P_WAIT, sys.executable, args, os.environ)

