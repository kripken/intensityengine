
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
from intensity.world import restart_map
from intensity.signals import signal_component


def receive(sender, **kwargs):
    component_id = kwargs['component_id']
    data = kwargs['data']

    try:
        if component_id == 'MapControl':
            parts = data.split('|')
            command = parts[0]
            params = '|'.join(parts[1:])
            if command == 'restart':
                main_actionqueue.add_action(restart_map)
    except Exception, e:
        log(logging.ERROR, "Error in MapControl component: " + str(e))

    return ''

signal_component.connect(receive, weak=False)

