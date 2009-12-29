
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

import cgi

from intensity.tests import get_redirect_url
from intensity.models import ServerInstance, Activity
from intensity.tracker.tests import TrackerTestCase, ReplaceRepurposing, FakeInstanceValidation
from intensity.version import *


class InstanceManagerTestCase(TrackerTestCase):
    def testShutdownAfterBeingUsed(self):
        self.doLogin()
        asset_id = self.createAsset()
        activity_id = self.createActivity(asset_id)

        with FakeInstanceValidation('aproof'):
            instance_id = self.createInstance(user='uzr1', admin='admk1', players=0, max_players=10, validation='aproof')

            activity = Activity.objects.get(uuid=activity_id)

            def get_instance():
                return ServerInstance.objects.get(uuid=instance_id)

            self.assertEquals(get_instance().status, ServerInstance.STATUS.Inactive)

            def run_test(add_player):
                with ReplaceRepurposing():
                    response = self.client.post('/tracker/activity/requisition/%s/' % activity_id)
                    url = get_redirect_url(response)
                    self.assertContains(self.client.get(url), 'successfully')

                for i in range(3):
                    parsed = self.updateInstance(user='uzr1', admin='admk1', players=0, max_players=10, validation='aproof') # Still empty
                    self.assertEquals(parsed['activity_id'][0], activity_id) # Told to run the activity
                    instance = get_instance()
                    self.assertEquals(instance.players, 0)
                    self.assertEquals(instance.activity, activity) # Still set to that, instance should do that
                    self.assertEquals(instance.status, ServerInstance.STATUS.Preparing)

                for i in range(3):
                    add_player()
                    instance = get_instance()
                    self.assertTrue(instance.players >= 1)
                    self.assertEquals(instance.activity, activity) # Still running
                    self.assertEquals(instance.status, ServerInstance.STATUS.Preparing)

                for i in range(3):
                    parsed = self.updateInstance(user='uzr1', admin='admk1', players=0, max_players=10, validation='aproof') # The someone has left
                    self.assertTrue('activity_id' not in parsed) # NOT told to run the activity
                    instance = get_instance()
                    self.assertEquals(instance.players, 0)
                    self.assertEquals(instance.activity, None) # Freed up
                    self.assertEquals(instance.requisitioner, None) # Freed up
                    self.assertEquals(instance.status, ServerInstance.STATUS.Inactive)

            def update_with_player():
                parsed = self.updateInstance(user='uzr1', admin='admk1', players=1, max_players=10, validation='aproof') # Got someone
                self.assertEquals(parsed['activity_id'][0], activity_id) # Told to run the activity

            run_test(update_with_player)

            def instancelogin():
                response = self.client.get('/user/startsession', {
                    'identifier': self.username,
                    'hashed_password': self.user.password,
                    'version': INTENSITY_VERSION_STRING,
                })
                self.assertEquals(response.status_code, 200)
                parsed = cgi.parse_qs(response.content)
                account_id = parsed['your_id'][0]
                user_session_id = parsed['session_id'][0]
                instance_session_id = ServerInstance.objects.get(uuid=instance_id).session_id
                response = self.client.get('/user/checklogin', {
                    'instance_id': instance_id,
                    'session_id': instance_session_id,
                    'code': self.account.uuid + ',' + user_session_id,
                })

            run_test(instancelogin)

__COMPONENT_PRECEDENCE__ = 1

