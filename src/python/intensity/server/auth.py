
"""
Handles authentication, both of the server instance to the master server,
and of clients to this server.
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


from intensity.logging import *


class InstanceStatus:
    in_standby = False ##!< Standby mode means we need to be manually repurposed - we don't
                       ##!< automatically start a new map if the master update tells us that
                       ##!< This is important after a server crash - don't want to immediately
                       ##!< restart the same map, it might be a crash on load, leading to a loop

    ##! A server is run in local mode when its address is 'localhost'
    ##! In this mode, an instance will only let a single client connect to it,
    ##! from the same machine. This is useful for editing (stuff like heightmaps etc.
    ##! only work in this mode, they are not available in multiplayer())
    local_mode = False

    ##! Private edit mode means that only a single client may connect to this instance
    ##! (which can then, like with local mode, use heightmaps etc.).
    private_edit_mode = False

    map_loaded = False


def get_instance_id():
    return get_config("Network", "instance_id", '')

def get_instance_validation():
    return get_config('Network', 'instance_validation', '')

def get_instance_address():
    return get_config('Network', 'address', 'localhost')

def check_local_mode():
    InstanceStatus.local_mode = (get_instance_address() == 'localhost')
    return InstanceStatus.local_mode


## Contacts the master server with a status update about this server. The
## response tells us what map we should be running
def update_master(params={}, act=True):
    log(logging.DEBUG, "Updating master...")

    try:
        params.update({
            'version': INTENSITY_VERSION_STRING,
            'user_interface': get_instance_address() + ':' + get_config('Network', 'port', '28787'),
            'admin_interface': get_instance_address() + ':' + get_config('Network', 'admin_port', '28789'),
#            'instance_id': get_instance_id(),
            'activity_id': get_curr_activity_id(),
            'map_asset_id': get_curr_map_asset_id(),
            'validation': get_instance_validation(),
            'players': Clients.count(),
            'max_players': get_max_clients(),
        })

        response = contact_master("instance/update", params)
    except MasterNetworkError, e:
        log(logging.DEBUG, "Error in updating master: %s" % (str(e)))
        return # No biggie, in general, hope to succeed next time...

    if 'instance_id' in response:
        set_config('Network', 'instance_id', response['instance_id'])

        # This update has been like a login - save our info
        get_master_session().set_info(response['instance_id'], response['session_id'])

    forced = get_config('Activity', 'force_activity_id', '') != '' or get_config('Activity', 'force_map_asset_id', '') != ''

    if InstanceStatus.in_standby:
        log(logging.WARNING, "In standby mode, not even considering loading a map")
        return

    if act and not InstanceStatus.map_loaded and forced:
        def do_set_map():
            set_map(get_config('Activity', 'force_activity_id', ''), get_config('Activity', 'force_map_asset_id', ''))
        main_actionqueue.add_action(do_set_map)

    if act and 'activity_id' in response and 'map_asset_id' in response and not forced:
        log(logging.DEBUG, "Master server requests us to change map")

        def do_set_map():
            set_map(response['activity_id'], response['map_asset_id'])
        main_actionqueue.add_action(do_set_map)


## Uploads an error log to the master, e.g., after a crash
def upload_error_log(error_log):
    update_master(act=False) # So we know our instance_id

    try:
        response = contact_master(
            "instance/uploadlog", 
            {
                'instance_id': get_instance_id(),
                'error_log': error_log,
            },
            POST=True
        )
    except MasterNetworkError, e:
        print "Error in updating master: %s" % (str(e))
        return False

    return True


## @return False if failed, or a dictionary with 'username' and 'can_edit'
def check_login(code):
    if check_local_mode(): return {};

    # The code is an OTP, used to identify and verify the client
    try:
        response = contact_master(
            "user/checklogin",
            {
                'instance_id': get_instance_id(),
                'code': code
            }
        )
    except MasterNetworkError, e:
        log(logging.ERROR, "Error in contacting master to check login: %s" % (str(e)))
        return False

    if response['success'] == '1':
        return response
    else:
        return False


# Prevent loops

from intensity.master import *
from intensity.server.persistence import *
from intensity.world import *

