
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

