
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


import os, ConfigParser


INTENSITY_HOME_DIR_LABEL = 'INTENSITY_HOME_DIR'

config = None
home_dir = os.path.dirname(__file__) # Default value, is the one used in testing

def get(category, key, default=''):
    try:
        return config.get(category, key)
    except:
        return default

def set_home_dir(_home_dir=None):
    # Use environment setting, if there is one
    if os.environ.get(INTENSITY_HOME_DIR_LABEL, '') != '':
        _home_dir = os.environ[INTENSITY_HOME_DIR_LABEL]
    # Use default location, if testing
    elif _home_dir is None:
        _home_dir = os.path.join(
            os.path.split(
                os.path.dirname(os.path.abspath(__file__))
            )[:-1]
        )[0] # The parent directory of this one. TODO: Always be in sync with DJANGO_SETTINGS_MODULE directory

    global config, home_dir
    home_dir = _home_dir
    if config is None:
        config_file = os.path.join(home_dir, 'settings.cfg')
        config = ConfigParser.ConfigParser()
        config.read( config_file )

        import intensity.logging_system as intensity_logging
        intensity_logging.init(home_dir, get('Logging', 'level', 'INFO'))


if INTENSITY_HOME_DIR_LABEL in os.environ:
    set_home_dir() # Load from environment

def get_home_dir():
    global home_dir
    return home_dir

def set(category, key, value):
    if not config.has_section(category):
        config.add_section(category)
    return config.set(category, key, value)

