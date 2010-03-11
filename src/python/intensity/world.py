"""
Manages loading maps etc.
"""


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

import os, tarfile, re, httplib
import uuid

from _dispatch import Signal

from intensity.base import *
from intensity.logging import *
from intensity.utility import *


# Signals

map_load_start = Signal(providing_args=['activity_id', 'map_asset_id'])
map_load_finish = Signal() # Only sent if map loads successfully


# Globals

curr_activity_id = None ##< The activity ID of the current activity
curr_map_asset_id = None ##< The asset id of this map, whose location gives us the prefix, etc.
curr_map_prefix = None

def get_curr_activity_id():
    return curr_activity_id

def set_curr_activity_id(activity_id):
    global curr_activity_id
    curr_activity_id = activity_id

def get_curr_map_asset_id():
    return curr_map_asset_id

def set_curr_map_asset_id(map_asset_id):
    global curr_map_asset_id
    curr_map_asset_id = map_asset_id

def get_curr_map_prefix():
    return curr_map_prefix

def set_curr_map_prefix(prefix):
    global curr_map_prefix
    curr_map_prefix = prefix


class WorldClass:
    scenario_code = None

    def start_scenario(self):
        old_scenario_code = self.scenario_code
        while old_scenario_code == self.scenario_code:
            self.scenario_code = str(uuid.uuid4())

    def running_map(self):
        return self.scenario_code is not None


## Singleton with current world info
World = WorldClass()


# Parses a URL to an activity, finding the activity ID, and then contacting the master to
# find the map asset id as well, for that activity
def autodiscover_activity(activity_id):
    if get_config('Network', 'master_server', '') == '':
        return '', ''

    if '/' in activity_id:
        activity_id = re.search('/(\w+)/$', activity_id).group(1)

    # Get the map asset ID using a request to the master
    log(logging.DEBUG, 'Contacting master to find map asset ID for activity %s' % activity_id)
    conn = httplib.HTTPConnection(get_master_server())
    conn.request('GET', '/tracker/activity/view/%s/' % activity_id)
    response = conn.getresponse()
    assert(response.status == 200)
    data = response.read()
    conn.close()

    map_asset_id = re.search('asset/view/(\w+)/', data).group(1)

    return activity_id, map_asset_id


## Sets a map to be currently active, and starts a new scenario
## @param _map The asset id for the map (see curr_map_asset_id)
def set_map(activity_id, map_asset_id):
    log(logging.DEBUG, "Setting the map to %s / %s" % (activity_id, map_asset_id))

    # Determine map activity and asset and get asset info

    need_lookup = True
    if Global.SERVER:
        forced_location = get_config('Activity', 'force_location', '')
        if forced_location != '':
            need_lookup = False
            activity_id = '*FORCED*'
            map_asset_id = forced_location # Contains 'base/'
    else: # CLIENT
        parts = map_asset_id.split('/')
        if parts[0] == 'base':
            need_lookup = False
            set_config('Activity', 'force_location', map_asset_id)

    # If given a URL of an activity, or don't have the map asset id, autodiscover the activity and map asset ids
    if need_lookup and '/' in activity_id or map_asset_id == '':
        activity_id, map_asset_id = autodiscover_activity(activity_id)

    if need_lookup:
        try:
            asset_info = AssetManager.acquire(map_asset_id)
        except AssetRetrievalError, e:
            log(logging.ERROR, "Error in retrieving assets for map: %s" % str(e))
            if Global.CLIENT:
                CModule.show_message("Error", "Could not retrieve assets for the map: " + str(e))
                CModule.disconnect()
                CModule.logout()
            return False
    else:
        # Working entirely locally - use config location and run from there
        asset_info = AssetInfo('xyz', map_asset_id, '?', 'NONE', [], 'b')

    log(logging.DEBUG, "final setting values: %s / %s" % (activity_id, map_asset_id))

    map_load_start.send(None, activity_id=activity_id, map_asset_id=map_asset_id)

    World.start_scenario()

    # Server may take a while to load and set up the map, so tell clients
    if Global.SERVER:
        MessageSystem.send(ALL_CLIENTS, CModule.PrepareForNewScenario, World.scenario_code)
        CModule.force_network_flush() # Flush message immediately to clients

    # Set globals

    set_curr_activity_id(activity_id)
    set_curr_map_asset_id(map_asset_id)
    World.asset_info = asset_info

    curr_map_prefix = asset_info.get_zip_location() + os.sep # asset_info.location
    set_curr_map_prefix(curr_map_prefix)

    log(logging.DEBUG, "Map locations: %s -- %s ++ %s" % (asset_info.location, curr_map_prefix, AssetManager.get_full_location(asset_info)))

    # Load the geometry and map settings in the .ogz
    if not CModule.load_world(curr_map_prefix + "map"):
        log(logging.ERROR, "Could not load map %s" % curr_map_prefix)
        raise Exception("set_map failure")

    if Global.SERVER:
        # Create script entities for connected clients
        log(logging.DEBUG, "Creating scripting entities for map")
        CModule.create_scripting_entities()

        auth.InstanceStatus.map_loaded = True

        # Update master server - we are finished preparing
        auth.update_master({ 'finished_preparing': 1 })

        # Send map to all connected clients, if any
        send_curr_map(ALL_CLIENTS)

        # Initialize instance status for this new map
        auth.InstanceStatus.private_edit_mode = False

    map_load_finish.send(None)

    return True # TODO: Do something with this value


def restart_map():
    AssetManager.clear_cache() # Make sure we will load the latest assets
    set_map(get_curr_activity_id(), get_curr_map_asset_id())


## Returns the path to a file in the map script directory, i.e., a file is given in
## relative position to the current map, and we return the full path
def get_mapfile_path(relative_path):
    # Check first in the installation packages
    install_path = os.path.sep.join( os.path.join('packages', World.asset_info.get_zip_location(), relative_path).split('/') )
    if os.path.exists(install_path):
        return install_path
    return os.path.join(World.asset_info.get_zip_location(AssetManager.get_full_location(World.asset_info)), relative_path)


## Reads a file for Scripting. Must be done safely. The path is under /packages,
## and we ensure that no attempt is made to 'break out'
def read_file_safely(name):
    assert(".." not in name)
    assert("~" not in name)
    assert(name[0] != '/')
    # TODO: More checks

    # Use relative paths, if asked for, or just a path under the asset dir
    if len(name) >= 2 and name[0:2] == './':
        path = get_mapfile_path(name[2:])
    else:
        path = os.path.join( get_asset_dir(), name )

    try:
        f = open(path, 'r')
    except IOError:
        try:
            install_path = os.path.join('packages', name)
            f = open(install_path, 'r') # Look under install /packages
        except IOError:
            print "Could not load file %s (%s, %s)" % (name, path, install_path)
            assert(0)

    data = f.read()
    f.close()

    return data


## Returns the path to the map script. TODO: As an option, other map script names?
def get_map_script_filename():
    return get_mapfile_path('map.js')

## Runs the startup script for the current map. Called from worldio.loadworld
def run_map_script():
    script = open( get_map_script_filename(), "r").read()
    log(logging.DEBUG, "Running map script...")
    CModule.run_script(script, "Map script")
    log(logging.DEBUG, "Running map script complete..")


## Packages an asset for uploading, and handles some backups for internal files
## Recursively adds directories, but doesn't filter out BAK and ~ files in them, just in the root - FIXME
def upload_asset(asset_id, backup_postfix = None, num_backups = 0, num_backups_to_keep = 0):
    asset_info = AssetManager.get_info( asset_id )
    full_location = AssetManager.get_full_location(asset_info)

    if asset_info.is_zipfile():
        prefix = asset_info.get_zip_location(full_location)

        # Create

        zip_name = prefix + ".tar.gz"
        zipfile = tarfile.open(zip_name, 'w:gz')

        filenames = os.listdir(prefix)
        total = len(filenames)
        counter = 0
        for inner_filename in filenames:
            CModule.render_progress(float(counter)/total, 'packaging archive asset...')
            if Global.CLIENT: CModule.intercept_key(0)

            counter += 1

            # Don't add backup files
            if inner_filename[-4:] != '.BAK' and inner_filename[-1] != '~':
                zipfile.add(prefix + os.sep + inner_filename, arcname = inner_filename)

            if backup_postfix is not None:
                if inner_filename[-len(backup_postfix):] == backup_postfix:
                    shutil.copyfile(prefix + os.sep + inner_filename, prefix + os.sep + inner_filename + "." + str(time.time()) + ".BAK");
                    num_backups += 1

        zipfile.close()

        # Backups were created for the ogz and entities, do some cleaning up
        if num_backups_to_keep > 0:
            clean_up_backups(prefix, "BAK", num_backups * num_backups_to_keep)

    # Upload
    AssetManager.upload_asset(asset_info)


## @param location e.g. textures/mypack.tar.gz. No need for 'packages/'.
def upload_asset_by_location(location):
    try:
        upload_asset(AssetMetadata.get_by_path('packages/' +location).asset_id)
        print "Asset %s uploaded successfully" % location
    except Exception, e:
        CModule.show_message("Error", "Could not upload the asset to the asset server: " + str(e))


def upload_map():
    if get_config('Network', 'master_server', '') != '' and get_config('Activity', 'force_location', '') == '':
        try:
            upload_asset(
                get_curr_map_asset_id(),
                backup_postfix = '.js',
                num_backups = 2, # We already backed up the ogz and entities beforehand
                num_backups_to_keep = 3
            )
        except Exception, e:
            CModule.show_message("Error", "Could not upload the map to the asset server: " + str(e))
            return

        # Notify server
        MessageSystem.send(CModule.RestartMap)


def export_entities(filename):
    full_path = os.path.join(get_asset_dir(), get_curr_map_prefix(), filename)

    data = CModule.run_script_string("saveEntities()", "export_entities")

    # Save backup, if needed

    if os.path.exists(full_path):
        try:
            shutil.copyfile(full_path, full_path + "." + str(time.time())[-6:].replace('.', '') + '.BAK')
        except:
            pass # No worries mate

    # Save new data

    out = open(full_path, 'w')
    out.write(data)
    out.close()


# Prevent loops

from intensity.asset import *
from intensity.message_system import *
from intensity.master import get_master_server

if Global.SERVER:
    from intensity.server.persistence import *

