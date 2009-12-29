## Quitting operations shared by both client and server


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


from _dispatch import Signal

#

def singleton_send(self, sender, **named):
    '''
    Sends a signal using Django's system, and assumes a single provider
    is registered. Will fail otherwise. Returns the single response from
    that provider.
    '''
    responses = self.send(sender, **named)
    if len(responses) != 1:
        raise Exception('Expected a single component to provide %s (%s), but got %d' % (self, sender, len(responses)))

    return responses[0][1]    


def multiple_send(self, sender, **named):
    '''
    Sends a signal using Django's system, and returns just the return values (no senders).
    '''
    responses = self.send(sender, **named)
    return map(lambda response: response[1], responses)


#

shutdown = Signal()

bandwidth_out = Signal(providing_args=['bytes'])
def signal_bandwidth_out(bytes): # C++ access is to this
    bandwidth_out.send(None, bytes=bytes)

client_connect = Signal(providing_args=['client_number'])
client_disconnect = Signal(providing_args=['client_number'])

text_message = Signal(providing_args=['client_number', 'text'])
def signal_text_message(client_number, text):
    text_message.send(None, client_number=client_number, text=text)

# Generic way to signal components. WARNING: Can be called from untrusted code! Check these messages before acting on them
signal_component = Signal(providing_args=['component_id', 'data'])
def signal_signal_component(component_id, data):
    return str( signal_component.send(None, component_id=component_id, data=data) )

