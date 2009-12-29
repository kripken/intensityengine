
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


'''
Expires server instances after a while has passed since they
updated the master.
'''

__COMPONENT_PRECEDENCE__ = 100

from datetime import datetime

from intensity.tracker.signals import list_instances


STALE_SECONDS = 15*60 # 15 minutes

def is_stale(last_update):
    return (datetime.now() - last_update).seconds > STALE_SECONDS

def expire_stale(sender, **kwargs):
    instances = kwargs['instances']
    for instance in instances:
        last_update = instance.last_update
        if is_stale(last_update):
            instance.delete()

list_instances.connect(expire_stale)

