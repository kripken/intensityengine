
# Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
# This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

"""
Handles authentication and such with the server
"""

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

