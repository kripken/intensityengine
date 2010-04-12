
# Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
# This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

from __future__ import with_statement

from intensity.tests import get_redirect_url
from intensity.models import AssetInfo, ServerInstance, Activity
from intensity.tracker.tests import TrackerTestCase, ReplaceRepurposing, FakeInstanceValidation
from intensity.tests import IntensityConfSetting


class MapWizardTestCase(TrackerTestCase):
    def testMapWizard(self):
        self.doLogin()
        def test(location, params={}):
            full_params = {
                'location': location,
                'original': AssetInfo.get_emptymap().id,
            }
            full_params.update(params)
            response = self.client.post('/tracker/tools/map_wizard/', full_params)
            url = get_redirect_url(response)
            self.assertContains(self.client.get(url), 'successfully')
            new_asset = AssetInfo.objects.get(location=location) # should exist
            assert(new_asset.kb_size == AssetInfo.get_emptymap().kb_size)
            new_activity = Activity.objects.get(asset = new_asset) # should exist

        test('base/whatamap.tar.gz')

        # Test with reqqing

        with ReplaceRepurposing():
            with FakeInstanceValidation('aproof'):
                instance_id = self.createInstance(user='uzr1', admin='admk1', players=0, max_players=10, validation='aproof')

                self.assertEquals(ServerInstance.objects.filter(status=ServerInstance.STATUS.Inactive).__len__(), 1)
                test('base/evenbetter.tar.gz', { 'requisition': True })
                self.assertEquals(ServerInstance.objects.filter(status=ServerInstance.STATUS.Inactive).__len__(), 0)


__COMPONENT_PRECEDENCE__ = 1

