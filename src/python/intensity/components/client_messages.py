
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

