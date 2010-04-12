
# Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
# This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

## Quitting operations shared by both client and server

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

validate_client = Signal(providing_args=['client_number', 'ip_addr', 'username', 'can_edit'])
client_connect = Signal(providing_args=['client_number'])
client_disconnect = Signal(providing_args=['client_number'])

text_message = Signal(providing_args=['client_number', 'text'])
def signal_text_message(client_number, text):
    text_message.send(None, client_number=client_number, text=text)

# Generic way to signal components. WARNING: Can be called from untrusted code! Check these messages before acting on them
signal_component = Signal(providing_args=['component_id', 'data'])
def signal_signal_component(component_id, data):
    return str( multiple_send(signal_component, None, component_id=component_id, data=data) )

show_components = Signal()
def signal_show_components(): # C++ access is to this
    show_components.send(None)

