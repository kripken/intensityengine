
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

