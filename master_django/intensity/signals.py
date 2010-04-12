
# Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
# This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

from django import dispatch


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


# General signals

send_email = dispatch.Signal(providing_args=['recipient', 'subject', 'body'])

# Security / CAPTCHAs etc.

prepare_security_check = dispatch.Signal(providing_args=['errors'])

verify_security_check = dispatch.Signal(providing_args=['request'])

