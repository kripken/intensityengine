
"""
Handles authentication and such with the server
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

from urlparse import urlparse

from intensity.logging import *


## Contacts the master server to prepare a login attempt to the appropriate instance.
## From the master server, receive an OTP and the instance
## we should connect to, using that OTP. Then does
## a log in to that instance
def login_to_instance(instance_id):
    class Output:
        pass

    keep_aliver = KeepAliver("Connecting...")

    def side_operations():
        try:
            Output.response = contact_master(
                "user/joininstance",
                {
                    'instance_id': instance_id
                },
            )
        except MasterNetworkError, e:
            Output.response = None
            Output.error = str(e)
        finally:
            keep_aliver.quit()

    # Run blocking HTTP in other thread, meanwhile do keepalive in this one, until finished
    side_actionqueue.add_action(side_operations)
    keep_aliver.wait()

    if Output.response is None:
        CModule.show_message("Error", Output.error)
        return

    CModule.set_transaction_code(Output.response['code'])

    url = urlparse.urlparse( Output.response['instance_url'] )
    host = url.hostname
    port = url.port
    CModule.connect(host, port)


## Contacts the master server to get the info to connect to our chosen ('selected') instance
def connect_to_special_instance(_type):
    class Output:
        pass

    keep_aliver = KeepAliver("Preparing to connect...")

    def side_operations():
        try:
            Output.response = contact_master("tracker/instance/get%s/" % _type)
        except MasterNetworkError, e:
            Output.response = None
            Output.error = str(e)
        finally:
            keep_aliver.quit()

    # Run blocking HTTP in other thread, meanwhile do keepalive in this one, until finished
    side_actionqueue.add_action(side_operations)
    keep_aliver.wait()

    if Output.response is None:
        CModule.show_message("Error", Output.error)
        return

    interface = Output.response['interface']
    host, port = interface.split(':')
    CModule.connect(host, int(port))


def connect_to_selected_instance():
    connect_to_special_instance('selected')


def connect_to_lobby():
    connect_to_special_instance('lobby')


## Gets the list of possible instances
def get_possible_instances():
    class Output:
        pass

    keep_aliver = KeepAliver("Retrieving list...")

    def side_operations():
        try:
            Output.response = contact_master("user/possibleinstances")
        except MasterNetworkError, e:
            log(logging.ERROR, "Error in contacting master: " + str(e))
            Output.response = None
        finally:
            keep_aliver.quit()

    # Run blocking HTTP in other thread, meanwhile do keepalive in this one, until finished
    side_actionqueue.add_action(side_operations)
    keep_aliver.wait()

    ret = []
    try:
        data = Output.response['instances']
    except KeyError:
        data = ""
    except TypeError:
        data = ""

    if data != "":
        instances = data.split("|")
        for instance in instances:
            instance_id, event_name = instance.split(",")
            ret.append( { 'instance_id': instance_id, 'event_name': event_name } )

    return ret


# Prevent loops

from intensity.master import *
from intensity.safe_actionqueue import *

