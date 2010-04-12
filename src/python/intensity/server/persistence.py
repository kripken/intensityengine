
# Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
# This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

import os, sys, shutil, time

from intensity.base import *
from intensity.logging import *
from intensity.errors import *
from intensity.message_system import *
from intensity.asset import *
from intensity.signals import client_connect, client_disconnect, validate_client, multiple_send


## Information about a single connected client
class Client:
    def __init__(self, number, ip_addr, admin, username, user_id):
        self.number = number
        self.ip_addr = ip_addr
        self.admin = admin
        self.username = username
        self.user_id = user_id


## Information about all connected clients
class Clients:
    _map = {}

    @staticmethod
    def add(client_number, *args):
        Clients._map[client_number] = Client(client_number, *args)
        client_connect.send(None, client_number=client_number)

    @staticmethod
    def remove(client_number):
        # Client may not have fully logged in before being booted
        if client_number in Clients._map:
            client_disconnect.send(None, client_number=client_number) # Send signal first, so info here can be read
            del Clients._map[client_number]

    @staticmethod
    def count():
        return len(Clients._map)

    @staticmethod
    def get(client_number):
        return Clients._map[client_number]

    @staticmethod
    def list():
        return Clients._map.values()


def get_max_clients():
    return int(get_config('Clients', 'limit', 10))

## Tries to log in a client.
##
## Right before doing the actual login test, updates the master. This may
## give us a response that we should run a different map, if e.g. this client
## has just repurposed us to do so
def do_login(code, client_number, ip_addr):
    def fail(message):
        log(logging.ERROR,message)
        show_client_message(client_number, "Login failure", message)
        CModule.force_network_flush()
        CModule.disconnect_client(client_number, 3) # DISC_KICK... most relevant for now

    if not World.running_map():
        return fail("Login failure: instance is not running a map")

    if auth.InstanceStatus.local_mode and ip_addr == 16777343: # 16777343 == 127.0.0.1 == 1*256^3 + 127
        CModule.update_username(client_number, 'local_editor')
        CModule.set_admin(client_number, True);

        Clients.add(client_number, ip_addr, True, 'local_editor', 'local_editor')

        MessageSystem.send(client_number,
                           CModule.LoginResponse,
                           1, 1); # success, local
        return

    if auth.InstanceStatus.local_mode:
        return fail("Login failure: instance is in local mode, but client is not local to it")

    if auth.InstanceStatus.private_edit_mode and Clients.count() >= 1:
        return fail("Login failure: instance is in private edit mode and occupied")

    if Clients.count() >= get_max_clients():
        return fail("Login failure: instance is at its maximum number of clients (%d)" % get_max_clients())

    curr_scenario_code = World.scenario_code

    # All contact to the master server is done in the side thread, here
    def side_operations():
        auth.update_master()
        response = auth.check_login(code)
        print "Response:", response
        success = response is not False

        # Results of master response are done in the main thread
        def finish_do_login():
            if not success:
                log(logging.ERROR, "Login failure: %d" % (client_number))
                show_client_message(client_number, "Login Failure", "The supplied transaction code was not verified")
                log(logging.WARNING, "Should flush network messages, but not doing so") #CModule.force_network_flush()
                CModule.disconnect_client(client_number, 3) # DISC_KICK... most relevant for now
                return

            if World.scenario_code != curr_scenario_code:
                log(logging.WARNING, "Scenario code has changed since client %d began login" % client_number)
                return fail("Server has begun a new scenario. Try connecting again") # XXX - do we want this?

            user_id = response['user_id']

            if filter(lambda client: client.user_id == user_id, Clients.list()) != []:
                show_client_message(client_number, "Login Failure", "You are already logged into this server. (If you just crashed, wait a little.)")
                CModule.force_network_flush()
                CModule.disconnect_client(client_number, 3) # DISC_KICK... most relevant for now
                return

            username = response['username']
            can_edit = response['can_edit'] == '1'

            # Validate with plugins. String results are error messages, True values are successes

            validation_errors = filter(lambda x: type(x) is str, multiple_send(validate_client, None, **{
                'client_number': client_number,
                'ip_addr': ip_addr,
                'username': username,
                'can_edit': can_edit,
            }))
            if len(validation_errors) > 0:
                for error in validation_errors:
                    log(logging.WARNING, '%s login failure: %s' % (username, error))
                return fail(validation_errors[0]) # Show user only first error. They should fix that and try again

            # Success, proceed

            CModule.update_username(client_number, username)

            if can_edit:
                CModule.set_admin(client_number, True);
                admin = True
            else:
                admin = False

            # Also creates a scripting logic entity
            MessageSystem.send(client_number,
                                CModule.LoginResponse,
                                1, 0); # success, non-local

            Clients.add(client_number, ip_addr, admin, username, user_id)

        main_actionqueue.add_action(finish_do_login)

    side_actionqueue.add_action(side_operations)


## Called when a client is disconnected
def on_logout(client_number):
    Clients.remove(client_number)


##
def request_private_edit(client_number):
    if not Clients.get(client_number).admin:
        return show_client_message(client_number, "Request denied", "You are not an administrator of this map")
    elif Clients.count() != 1:
        return show_client_message(client_number, "Request denied", "There are other clients on this server instance")
    else:
        # Success
        auth.InstanceStatus.private_edit_mode = True
        MessageSystem.send(client_number, CModule.NotifyPrivateEditMode)


## Notifies clients that we would like to send them the current map, as it has changed.
## We do so by sending them the current map's name. They then uses the asset system to acquire the
## map, in it's latest version, both .cfg and .ogz files, etc. etc., using the map name as the asset ID.
## @param client_number The identifier of the client to which to send the map, or ALL_CLIENTS (-1) for all
def send_curr_map(client_number):
    if not World.running_map():
        log(logging.WARNING, "Trying to notify clients about curr map, but no map")
        return

    log(logging.DEBUG, "Notifying clients that we would like to send them the map: %s" % (get_curr_map_asset_id()))

    MessageSystem.send(client_number, CModule.NotifyAboutCurrentScenario, get_curr_map_asset_id(), World.scenario_code)


import intensity.server.auth as auth
from intensity.world import *

