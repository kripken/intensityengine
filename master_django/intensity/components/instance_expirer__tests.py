
# Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
# This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

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

