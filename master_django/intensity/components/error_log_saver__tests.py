
# Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
# This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

from __future__ import with_statement

from intensity.tests import get_redirect_url
from intensity.models import ServerInstance, Activity
from intensity.tracker.tests import TrackerTestCase, ReplaceRepurposing, FakeInstanceValidation
from intensity.components.error_log_saver__models import get_error_log, get_last_error_log_index
from intensity.tests import IntensityConfSetting


class ErrorLogSaverTestCase(TrackerTestCase):
    def testErrorLogSaving(self):
        self.doLogin()
        asset_id = self.createAsset()
        activity_id = self.createActivity(asset_id)

        with FakeInstanceValidation('aproof'):
            instance_id = self.createInstance(user='uzr1', admin='admk1', players=0, max_players=10, validation='aproof')

            def get_instance():
                return ServerInstance.objects.get(uuid=instance_id)

            for i in range(4):
                last_error_log_index = get_last_error_log_index(self.account)

                with ReplaceRepurposing():
                    response = self.client.post('/tracker/activity/requisition/%s/' % activity_id)
                    url = get_redirect_url(response)
                    self.assertContains(self.client.get(url), 'successfully')

                with IntensityConfSetting('Email', 'username', ''): # Disable normal email
                    response = self.client.post('/instance/uploadlog', {
                        'instance_id': instance_id,
                        'session_id': get_instance().session_id,
                        'error_log': '1234onluggage' + str(i),
                    })

                self.assertEqual(last_error_log_index + 1, get_last_error_log_index(self.account)) # added one log

                self.assertTrue('1234onluggage' + str(i) in get_error_log(self.account, get_last_error_log_index(self.account))) # content is there

                self.assertContains(
                    self.client.get('/tracker/account/error_log/view/%d/' % get_last_error_log_index(self.account)),
                    '1234onluggage' + str(i)
                ) # View via web

                self.assertContains(self.client.get('/tracker/account/error_log/view/'), '1234onluggage' + str(i)) # No arg, get last -this


__COMPONENT_PRECEDENCE__ = 1

