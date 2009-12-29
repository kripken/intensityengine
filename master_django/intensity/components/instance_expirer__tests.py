
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


import time
import datetime as datetime

from intensity.models import ServerInstance
from intensity.tracker.tests import TrackerTestCase


class InstanceExpirerTestCase(TrackerTestCase):
    def testX(self):
        instance_id = self.createInstance()
        instance = ServerInstance.objects.get(uuid=instance_id)
        instance.last_update = datetime.datetime.now()
        instance.save()
        time.sleep(5.5) # Wait for cache to flush
        self.client.get('/tracker/instances/') # Trigger possible expirings

        instance = ServerInstance.objects.get(uuid=instance_id) # Still here

        instance.last_update = datetime.datetime.now() - datetime.timedelta(seconds=5*60)
        instance.save()
        time.sleep(5.5) # Wait for cache to flush
        self.client.get('/tracker/instances/') # Trigger possible expirings
        instance = ServerInstance.objects.get(uuid=instance_id) # Still here

        instance.last_update = datetime.datetime.now() - datetime.timedelta(seconds=60*60)
        instance.save()
        time.sleep(5.5) # Wait for cache to flush
        self.client.get('/tracker/instances/') # Trigger possible expirings
        self.assertRaises(ServerInstance.DoesNotExist, ServerInstance.objects.get, uuid=instance_id) # Expired

__COMPONENT_PRECEDENCE__ = 1

