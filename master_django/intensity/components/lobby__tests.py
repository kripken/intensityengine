
# Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
# This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

from __future__ import with_statement

import cgi

from intensity.models import Activity, ServerInstance
from intensity.tests import IntensityConfSetting
from intensity.tracker.tests import TrackerTestCase


class LobbyTestCase(TrackerTestCase):
    def testX(self):
        instance_id1 = self.createInstance(user='uzr1', admin='admk1', players=5, max_players=10)
        instance_id2 = self.createInstance(user='localhost:2022', admin='admk2', players=5, max_players=10)
        instance_id3 = self.createInstance(user='whever:2022', admin='admk3', players=5, max_players=10)

        self.doLogin()

        activity = Activity.objects.get(uuid=self.createActivity(self.createAsset()))
        instance_1 = ServerInstance.objects.get(uuid=instance_id1)
        instance_1.activity = activity
        instance_1.save()
        instance_2 = ServerInstance.objects.get(uuid=instance_id2)
        instance_2.activity = activity
        instance_2.save()
        instance_3 = ServerInstance.objects.get(uuid=instance_id3)
        instance_3.activity = activity
        instance_3.save()

        def test(possibles):
            with IntensityConfSetting('Lobby', 'possibles', ','.join(possibles)):
                possibles = set(possibles)
                total = set([])
                for i in range(20*len(possibles)):
                    response = self.client.get('/tracker/instance/getlobby/', {
                        'user_id': self.account.uuid,
                    })
                    parsed = cgi.parse_qs(response.content)
                    if len(possibles) > 0:
                        self.assertTrue(parsed['interface'][0] in possibles)
                        total.add(parsed['interface'][0])
                if len(possibles) > 0:
                    self.assertEquals(total, possibles)
                else:
                    self.assertTrue('localhost' not in str(total))

        test([])
        test(['uzr1'])
        test(['whever:2022'])
        test(['uzr1', 'whever:2022'])

        self.doLogout()

__COMPONENT_PRECEDENCE__ = 1

