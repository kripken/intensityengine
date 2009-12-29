
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

