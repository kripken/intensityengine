
# Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
# This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

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

