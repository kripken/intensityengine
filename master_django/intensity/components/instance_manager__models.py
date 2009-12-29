
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

from __future__ import with_statement

__COMPONENT_PRECEDENCE__ = 100

import threading

from intensity.models import ServerInstance
from intensity.tracker.signals import requisition_instance, unrequisition_instance, pre_instance_update


class InstanceManager:
    def __init__(self):
        self.repurposing_lock = threading.Lock()

        self.been_entered = {}

    def get_available_instances(self):
        instances = ServerInstance.objects.filter(mode=ServerInstance.MODE.Pooled).filter(status=ServerInstance.STATUS.Inactive).order_by('-last_update') # Pooled, inactive instances, the latest updated (most likely to be alive) first

        return instances

    def do_requisition(self, account, instance, activity):
        try:
            instance.repurpose(account, activity)
            return True
        except:
            return False

    def try_requisition(self, sender, **kwargs):
        activity = kwargs['activity']
        # Allow only one thread to repurpose at once - so we don't steal them from one another
        with self.repurposing_lock:
            instances = self.get_available_instances()
            for instance in instances:
                if self.do_requisition(sender, instance, activity):
                    return "Server successfully requisitioned."
            return "Could not find a free server instance to run '%s'" % activity.name

    def unrequisition(self, sender, **kwargs):
        instance = kwargs['instance']
        instance.unpurpose()
        return 'Server successfully freed'

    def update(self, sender, **kwargs):
        instance = kwargs['instance']

        if instance.mode != ServerInstance.MODE.Pooled:
            return

        been = self.been_entered.setdefault(instance.uuid, False)
        occupied = (instance.players > 0)
        if been and not occupied:
            # Deactivate the instance - it was repurposed, used, and is now empty
            instance.unpurpose()
            self.been_entered[instance.uuid] = False # Prepare for next time
        else:
            self.been_entered[instance.uuid] = been or occupied

        # TODO: Delete keys, to prevent significant memory usage?


instance_manager = InstanceManager()

requisition_instance.connect(instance_manager.try_requisition)
unrequisition_instance.connect(instance_manager.unrequisition)
pre_instance_update.connect(instance_manager.update)

