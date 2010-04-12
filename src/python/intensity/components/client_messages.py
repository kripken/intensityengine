
# Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
# This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.


from intensity.base import *
from intensity.logging import *
from intensity.server.persistence import Clients
from intensity.signals import client_connect, client_disconnect

def get_client_name(client_number):
    try:
        client = Clients.get(client_number)
        return client.username
    except Exception, e:
        return '?'

# Handle login-logout

def login(sender, **kwargs):
    client_number = kwargs['client_number']
    CModule.send_text_message(-1, '<< %s has arrived >>' % get_client_name(client_number), False)
client_connect.connect(login, weak=False)

def logout(sender, **kwargs):
    client_number = kwargs['client_number']
    CModule.send_text_message(-1, '<< %s has left >>' % get_client_name(client_number), False)
client_disconnect.connect(logout, weak=False)

