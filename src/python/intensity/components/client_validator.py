
# Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
# This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

from intensity.base import *
from intensity.logging import *
from intensity.signals import validate_client


def handle(sender, **kwargs):
    client_number = kwargs['client_number']
    ip_addr = kwargs['ip_addr']
    username = kwargs['username']
    can_edit = kwargs['can_edit']

    allowed_usernames = get_config('ClientValidator', 'allowed_usernames', '').split(',')
    if username in allowed_usernames:
        return True

    allow_editors = get_config('ClientValidator', 'allowe_editors', '0') == '1'
    if allow_editors and can_edit:
        return True

    return 'You are not authorized to enter this server instance'
validate_client.connect(handle, weak=False)

