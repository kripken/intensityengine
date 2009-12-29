
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

import re, os, cgi
from StringIO import StringIO
from datetime import datetime

from django.test import TestCase
from django.test import client as test_client
from django.contrib.auth.models import User
from django.core.servers.basehttp import FileWrapper

from intensity.models import UserAccount, AssetInfo, ServerInstance, Activity
from intensity.tests import get_redirect_url, EmailSender, IntensityConfSetting
from intensity.signals import singleton_send
from intensity.tracker.signals import validate_instance, initialize_asset_storage, destroy_asset_storage, asset_download_redirect
from intensity.tracker.fixtures import create_initial_content
from intensity.version import *

from intensity.components.asset_storer__views import retrieve_asset
from intensity.components.local_asset_storer__models import set_dir as set_local_asset_dir

set_local_asset_dir(testing=True)


class FakeInstanceValidation:
    def __init__(self, secret='proofz'):
        self.secret = secret
    def __enter__(self):
        from intensity.tracker.signals import validate_instance
        def do_validate(sender, **kwargs):
            return kwargs['validation'] == self.secret
        self.fake_instance_validator = do_validate
        validate_instance.connect(self.fake_instance_validator)
    def __exit__(self, x, y, z):
        validate_instance.disconnect(self.fake_instance_validator)

class ReplaceRepurposing:
    def __init__(self, func=lambda self, command: True):
        self.func = func
    def __enter__(self):
        self.save = ServerInstance.contact_instance
        ServerInstance.contact_instance = self.func
    def __exit__(self, x, y, z):
        ServerInstance.contact_instance = self.save # Restore the class

asset_counter = 1

class TrackerTestCase(TestCase):
    def setUp(self):
        create_initial_content.INITIAL_ASSET_LOCATIONS = create_initial_content.INITIAL_ASSET_LOCATIONS__ORIGINAL.copy()
        for location in create_initial_content.INITIAL_ASSET_LOCATIONS__ORIGINAL:
            singleton_send(initialize_asset_storage, None, asset=AssetInfo.objects.get(location='base/' + location))

        self.username = 'regular'
        self.password = 'sacred'
        self.user = User.objects.create_user(username=self.username, email='a@a.ca', password=self.password)
        self.account = UserAccount.objects.get(user=self.user)

    def tearDown(self):
        self.user.delete()

        singleton_send(destroy_asset_storage, None)

    def doLogin(self):
        return self.client.login(username=self.username, password=self.password)

    def doLogout(self):
        self.client.logout()

    def createAsset(self):
        # Create asset, check success and redirect to right page
        response = self.client.post('/tracker/asset/new/')
        url = get_redirect_url(response)
        m = re.match('^/tracker/asset/view/(\w+)/$', url)
        self.assertTrue(m is not None)
        asset_id = m.group(1)
        self.assertRedirects(response, '/tracker/asset/view/%s/' % asset_id)
        self.assertContains(self.client.get('/tracker/asset/view/%s/' % asset_id), 'Upload content')
        asset = AssetInfo.objects.get(uuid=asset_id)
        global asset_counter
        asset.location = 'base/test%d.tar.gz' % asset_counter # Make an activity-friendly asset by default
        asset.save()
        asset_counter += 1
        return asset_id

    def uploadAsset(self, asset_id, content, params=None):
        if params is None:
            params = {}
        response = self.client.post('/tracker/asset/upload/%s/' % asset_id, params)
        url = get_redirect_url(response)
        as_file = StringIO()
        as_file.write(content)
        as_file.seek(0)
        as_file.name = 'xxNamexx'
        params.update({ 'file': as_file })
        response = self.client.post(url, params)
        as_file.close()

        url = get_redirect_url(response)
        self.assertEquals(url, '/tracker/asset/view/%s/' % asset_id)
        self.assertContains(self.client.get(url), 'Upload was successful') # Which should show a nice message

    def updateInstance(self, user='uzr', admin='admk', players=5, max_players=91, validation=''):
        response = self.client.get('/instance/update', {
            'user_interface': user,
            'admin_interface': admin,
            'players': players,
            'max_players': max_players,
            'version': INTENSITY_VERSION_STRING,
            'validation': validation,
        })
        parsed = cgi.parse_qs(response.content)
        return parsed

    def createInstance(self, user='uzr', admin='admk', players=5, max_players=91, validation=''):
        return self.updateInstance(user, admin, players, max_players, validation)['instance_id'][0]

    def createActivity(self, asset_id):
        response = self.client.post('/tracker/activity/new/', {
            'asset_id': asset_id, 
        })
        url = get_redirect_url(response)
        m = re.match('^/tracker/activity/view/(\w+)/$', url)
        if m is None: return None # creation failed, maybe we wanted that
        activity_id = m.group(1)
        self.assertRedirects(response, '/tracker/activity/view/%s/' % activity_id)
        return activity_id


# Actual tests
class TrackerTestCases(TrackerTestCase):

    def testPages(self):
        self.assertContains(self.client.get('/tracker/overview/'), '<title>Syntensity</title>')

        # Without login, get limited functionality
        self.assertContains(self.client.get('/tracker/instances/'), 'Current Activity') # Nothing to limit yet
        self.assertNotContains(self.client.get('/tracker/activities/'), 'Show only mine')
        self.assertNotContains(self.client.get('/tracker/assets/'), 'Show only mine')
        self.assertRedirects(self.client.get('/tracker/account/'), 'http://testserver/accounts/login/?next=/tracker/account/') # No accnt

        self.doLogin()
        asset_id = self.createAsset()
        activity_id = self.createActivity(asset_id)
        self.doLogout()

        self.assertNotContains(self.client.get('/tracker/asset/view/%s/' % asset_id), 'Clone')
        self.assertNotContains(self.client.get('/tracker/asset/view/%s/' % asset_id), 'Create an activity')
        self.assertNotContains(self.client.get('/tracker/asset/view/%s/' % asset_id), 'Download')
        self.assertNotContains(self.client.get('/tracker/asset/view/%s/' % asset_id), 'Modify')

        self.assertNotContains(self.client.get('/tracker/activity/view/%s/' % activity_id), 'Requisition')
        self.assertNotContains(self.client.get('/tracker/activity/view/%s/' % activity_id), 'Modify')

        # With login, get full access
        self.doLogin()
        self.assertContains(self.client.get('/tracker/instances/'), 'Current Activity')
        self.assertContains(self.client.get('/tracker/activities/'), 'Show only mine')
        self.assertContains(self.client.get('/tracker/assets/'), 'Show only mine')

        self.assertContains(self.client.get('/tracker/asset/view/%s/' % asset_id), 'Clone')
        self.assertContains(self.client.get('/tracker/asset/view/%s/' % asset_id), 'Create an activity')
        self.assertContains(self.client.get('/tracker/asset/view/%s/' % asset_id), 'Download')
        self.assertContains(self.client.get('/tracker/asset/view/%s/' % asset_id), 'Update')

        self.assertContains(self.client.get('/tracker/activity/view/%s/' % activity_id), 'Requisition')
        self.assertContains(self.client.get('/tracker/activity/view/%s/' % activity_id), 'Update')

        self.doLogout()

    def testAccountPage(self):
        # Ensure manual login works with this user/pass
        self.assertEquals(self.doLogin(), True)
        self.doLogout()

        # Redirect to login if not logged in
        self.assertRedirects(self.client.get('/tracker/account/'), 'http://testserver/accounts/login/?next=/tracker/account/')

        # Logging in there redirects to right place
        self.assertRedirects(
            self.client.post('/accounts/login/', {'username': 'regular', 'password': 'sacred', 'next': '/tracker/account/'}),
            '/tracker/account/',
        )

        # Subsequent goings to that page require no more logging in
        self.assertContains(self.client.get('/tracker/account/'), 'seen by others')

    def testAsset(self):
        self.doLogin()

        asset_id = self.createAsset()

        asset = AssetInfo.objects.get(uuid=asset_id)
        self.assertEquals(asset.kb_size, 0) #No contents yet

        # Modify the location
        self.assertNotContains(self.client.get('/tracker/asset/view/%s/' % asset_id), '/test/loc/')
        self.assertRedirects(self.client.post('/tracker/asset/view/%s/' % asset_id, {
            'location': '/test/loc/',
            'dependencies': [],
            'owners': [self.account.pk],
            'type_x': AssetInfo.TYPE.Both,
            'comment': '',
        }), '/tracker/asset/view/%s/' % asset_id)
        self.assertContains(self.client.get('/tracker/asset/view/%s/' % asset_id), '/test/loc/')

        # Try an upload. We get redirected to the web page that does the actual upload
        # (this might be on a separate asset server, for example)
        response = self.client.post('/tracker/asset/upload/%s/' % asset_id)
        url = get_redirect_url(response)

        # Create dummy binary data
        dummy_data = '\x89PNG\r\n\x1a\n\x00\x00\x00\rIHDR\x00\x00\x00\x08\x00\x00\x00\x08\x08\x00\x00\x00\x00\xe1d\xe1W\x00\x00\x00\x01sRGB\x00\xae\xce\x1c\xe9\x00\x00\x00\tpHYs\x00\x00\x0b\x13\x00\x00\x0b\x13\x01\x00\x9a\x9c\x18\x00\x00\x00\x07tIME\x07\xd9\x07\x1c\x07(/\xa2\xc9V\x06\x00\x00\x00\x19tEXtComment\x00Created with GIMPW\x81\x0e\x17\x00\x00\x00:IDAT\x08\xd7=\xca\xb1\x11\xc00\x0c\xc3@\x9ekw\x1e@\x1eW3\xa8\xe0\x9e\xa8\x98"\x97\xa0}h7\t\xbd\xd5ep\xb5p\xcd\x94Q\x98s\x86,}\xfd\xe4w\xb6\xaeI\xf0}\x00\xd9m"5sk\x9e\xe2\x00\x00\x00\x00IEND\xaeB`\x82' # An actual PNG file
        assert(len(dummy_data) == 205) # Make sure not ascii mangled

        self.uploadAsset(asset_id, dummy_data)

        asset = AssetInfo.objects.get(uuid=asset_id)
        self.assertEquals(asset.kb_size, 1) # 205 bytes, rounded up to 1K
        self.assertNotEquals(asset.hash_value, '')
        self.assertNotEquals(asset.hash_value, '?')

        # Try to download it back. Again we are initially redirected.
        response = self.client.post('/tracker/asset/download/%s/' % asset_id)
        url = get_redirect_url(response)
        self.assertContains(self.client.get(url), dummy_data)

    def testAssetDelete(self):
        self.doLogin()

        asset_id = self.createAsset()
        self.assertEqual(len(AssetInfo.objects.filter(uuid=asset_id)), 1)

        # Upload something
        self.uploadAsset(asset_id, 'some dummy data')

        # Try to delete. Should redirect to a message page
        response = self.client.post('/tracker/asset/delete/%s/' % asset_id)
        url = get_redirect_url(response)

        self.assertEqual(len(AssetInfo.objects.filter(uuid=asset_id)), 0) # No longer there

        # Check no longer in asset storage system either
        self.assertRaises(IOError, singleton_send, retrieve_asset, None, asset_uuid=asset_id)

    def testAssetClone(self):
        self.doLogin()

        asset_id = self.createAsset()
        dummy_data = '\x89PNG\r\n\x1a\n\x00\x00\x00\rIHDR\x00'
        self.uploadAsset(asset_id, dummy_data)

        asset = AssetInfo.objects.get(uuid=asset_id)
        dep_asset_id = self.createAsset()
        asset.dependencies.add(AssetInfo.objects.get(uuid=dep_asset_id))
        asset.save()

        # Create a clone

        def process_clone_response(response):
            url = get_redirect_url(response)
            m = re.match('^/tracker/asset/view/(\w+)/$', url)
            self.assertTrue(m is not None)
            clone_asset_id = m.group(1)
            return clone_asset_id

        response = self.client.post('/tracker/asset/clone/%s/' % asset_id)
        clone_asset_id = process_clone_response(response)
        self.assertEquals(asset_id == clone_asset_id, False) # Different uuid

        # Check data is ok for both
        for curr_asset_id in (asset_id, clone_asset_id):
            response = self.client.post('/tracker/asset/download/%s/' % curr_asset_id)
            url = get_redirect_url(response)
            self.assertContains(self.client.get(url), dummy_data)

        asset, clone_asset = map(lambda uuid: AssetInfo.objects.get(uuid=uuid), (asset_id, clone_asset_id))
        self.assertEquals(asset.kb_size, clone_asset.kb_size)
        self.assertEquals(asset.hash_value, clone_asset.hash_value)
        self.assertEquals(set(asset.owners.all()), set(clone_asset.owners.all())) # The original asset was ours, so same ownership
        self.assertEquals(asset.dependencies.all()[0].uuid, clone_asset.dependencies.all()[0].uuid)

        # Ownership: new asset is new user's, and new user's alone, not old one
        emptymap_asset = AssetInfo.objects.get(location='base/emptymap.tar.gz')
        response = self.client.post('/tracker/asset/clone/%s/' % emptymap_asset.uuid)
        clone_asset_id = process_clone_response(response)
        clone_asset = AssetInfo.objects.get(uuid=clone_asset_id)
        self.assertNotEquals(emptymap_asset.owners, clone_asset.owners)
        self.assertEquals([owner.uuid for owner in clone_asset.owners.all()], [self.account.uuid])

    def testCreateActivity(self):
        self.doLogin()

        asset_id = self.createAsset()

        self.client.post('/tracker/asset/view/%s/' % asset_id, {
            'location': 'base/test.tar.gz',
            'owners': [self.account.pk],
            'dependencies': [],
            'type_x': AssetInfo.TYPE.Both,
            'comment': 'SomeAsset',
        })

        activity_id = self.createActivity(asset_id)

        self.assertContains(self.client.get('/tracker/activity/view/%s/' % activity_id), 'SomeAsset') # Asset comment is part of name

        # Modify the name
        name = 'New name'
        self.assertNotContains(self.client.get('/tracker/activity/view/%s/' % activity_id), name)
        self.assertRedirects(self.client.post('/tracker/activity/view/%s/' % activity_id, {
            'name': name,
        }), '/tracker/activity/view/%s/' % activity_id)
        self.assertContains(self.client.get('/tracker/activity/view/%s/' % activity_id), name)

        # Fail on trying to create invalid activities

        asset_id = self.createAsset()

        self.client.post('/tracker/asset/view/%s/' % asset_id, {
            'location': '/test/loc/',
            'owners': [self.account.pk],
            'dependencies': [],
            'type_x': AssetInfo.TYPE.Both,
            'comment': 'SomeAsset',
        })

        self.assertEquals(self.createActivity(asset_id), None)

#        print self.client.get('/tracker/account/')

        self.assertContains(self.client.get('/tracker/account/'), 'Activity could not be created')
        self.assertNotContains(self.client.get('/tracker/account/'), 'Activity could not be created')

    def testActivityDelete(self):
        self.doLogin()

        asset_id = self.createAsset()
        activity_id = self.createActivity(asset_id)
        self.assertEqual(len(Activity.objects.filter(uuid=activity_id)), 1) # There now

        # Try to delete. Should redirect to a message page
        response = self.client.post('/tracker/activity/delete/%s/' % activity_id)
        url = get_redirect_url(response)

        self.assertEqual(len(Activity.objects.filter(uuid=activity_id)), 0) # No longer there

    def testServerInstanceUpdate(self):
        self.assertEquals(ServerInstance.MODE.convert('Pooled'), ServerInstance.MODE.Pooled)
        self.assertEquals(ServerInstance.MODE.convert('Independent'), ServerInstance.MODE.Independent)
        self.assertRaises(AttributeError, ServerInstance.MODE.convert, 'What')

        # Without version
        self.assertContains(self.client.get('/instance/update', {}), 'Engine+version+mismatch')

        # With bad (older) version
        self.assertContains(
            self.client.get('/instance/update', {'version': '0.' + INTENSITY_VERSION_STRING + '.2'}),
            'Engine+version+mismatch'
        )

        # With ok (newer) version
        self.assertNotContains(
            self.client.get('/instance/update', {'version': '9.' + INTENSITY_VERSION_STRING + '.2'}),
            'Engine+version+mismatch'
        )

        # Normal tests, with normal version
        response = self.client.get('/instance/update', {
            'user_interface': 'uzer',
            'admin_interface': 'admik',
            'players': 5,
            'max_players': 17,
            'version': INTENSITY_VERSION_STRING,
        })

        instance = ServerInstance.objects.get(user_interface='uzer')

        self.assertEquals(instance.status, ServerInstance.STATUS.Inactive)

        def test_instance():
            self.assertEquals(instance.admin_interface, 'admik')
            self.assertEquals(instance.mode, ServerInstance.MODE.Independent) # We didn't validate, so not Pooled
            self.assertEquals((datetime.now() - instance.last_update).seconds < 1.0, True)
            self.assertEquals(instance.players, 5)
            self.assertEquals(instance.max_players, 17)
            self.assertEquals(instance.activity, None)
        test_instance()

        # Second instance

        self.doLogin()
        asset_id = self.createAsset()
        activity_id = self.createActivity(asset_id)
        self.doLogout()

        params_2 = {
            'user_interface': 'uzer2',
            'admin_interface': 'admik2',
            'players': 9,
            'max_players': 33,
            'validation': 'proofz',
            'activity_id': activity_id,
            'version': INTENSITY_VERSION_STRING,
        }
        response = self.client.get('/instance/update', params_2)

        instance_2 = ServerInstance.objects.get(user_interface='uzer2')
        old_session_id = instance_2.session_id
        self.assertEquals(instance_2.admin_interface, 'admik2')
        self.assertEquals(instance_2.mode, ServerInstance.MODE.Independent)
        self.assertEquals((datetime.now() - instance_2.last_update).seconds < 1.0, True)
        self.assertEquals(instance_2.players, 9)
        self.assertEquals(instance_2.max_players, 33)
        self.assertEquals(ServerInstance.objects.get(user_interface='uzer2').session_id, old_session_id) # Old session remains
        parsed = cgi.parse_qs(response.content)
        self.assertEquals(old_session_id, parsed['session_id'][0]) # Also in response
        self.assertEquals(instance_2.activity.uuid, activity_id)

        # Old tests should remain the same
        test_instance()

        # Allow validation

        with FakeInstanceValidation('proofz'):
            old_session_id = ServerInstance.objects.get(user_interface='uzer2').session_id
            response = self.client.get('/instance/update', params_2)
            instance_2 = ServerInstance.objects.get(user_interface='uzer2')
            self.assertEquals(instance_2.mode, ServerInstance.MODE.Pooled)
            new_session_id = ServerInstance.objects.get(user_interface='uzer2').session_id
            self.assertNotEquals(new_session_id, old_session_id) # New session
            parsed = cgi.parse_qs(response.content)
            self.assertEquals(new_session_id, parsed['session_id'][0]) # We also got the session_id in the response

            # Check for returns values. When giving no activity info, we
            # should be told the activity and map asset id to run

            self.doLogin()

            asset_id = self.createAsset()
            activity_id = self.createActivity(asset_id)

            self.doLogout()

            activity = Activity.objects.get(uuid=activity_id)
            instance_2.activity = activity
            instance_2.save()

            # Getting of activity IDs:

            # Temporarily disable forcing of asset location, so we get the real asset&activity
            with IntensityConfSetting('Instances', 'force_asset_location', ''):
                response = self.client.get('/instance/update', params_2)
                self.assertContains(response, 'instance_id') # We are told our instance id
                parsed = cgi.parse_qs(response.content)
                self.assertEquals(parsed['activity_id'][0], activity_id)

            # And now we will get the 'forced' activity
            with IntensityConfSetting('Instances', 'force_asset_location', 'base/emptymap.tar.gz'):
                response = self.client.get('/instance/update', params_2)
                self.assertContains(response, 'instance_id') # We are told our instance id
                parsed = cgi.parse_qs(response.content)
                self.assertEquals(parsed['activity_id'][0], 'forced_location')

            # And when giving the right info, we should not get an activity ID

            params_2.update({
                'activity_id': activity_id,
            })
            response = self.client.get('/instance/update', params_2)
            parsed = cgi.parse_qs(response.content)
            self.assertFalse(activity_id in parsed)

    def testRequisitionServerInstance(self):
        self.doLogin()
        asset_id = self.createAsset()
        activity_id = self.createActivity(asset_id)

        def try_req(fail=None, succeed=None, instance=None):
            if succeed is not None:
                fail = not succeed
            assert(fail is not None)
            if fail:
                response = self.client.post('/tracker/activity/requisition/%s/' % activity_id)
                url = get_redirect_url(response)
                self.assertContains(self.client.get(url), 'Could not find')
            else:
                # We try to repurpose an instance at localhost, this fails with an internal IOError and returns failure
                response = self.client.post('/tracker/activity/requisition/%s/' % activity_id)
                url = get_redirect_url(response)
                self.assertContains(self.client.get(url), 'Could not find')

                class Output: pass
                def do_it(*args, **kwargs): Output.data = 1567; return True

                # Now prevent this error by patching in the class
                with ReplaceRepurposing(do_it):
                    response = self.client.post('/tracker/activity/requisition/%s/' % activity_id)
                    url = get_redirect_url(response)
                    self.assertContains(self.client.get(url), 'successfully')
                    self.assertEquals(Output.data, 1567)

        try_req(fail=True) # Fail, as no server instances

        # Add a valid server instance
        params = {
            'user_interface': 'someplace.com:8888',
            'admin_interface': 'admikal_irk',
            'players': 8,
            'max_players': 22,
            'validation': 'proofz',
            'version': INTENSITY_VERSION_STRING,
        }
        response = self.client.get('/instance/update', params)
        instance = ServerInstance.objects.get(user_interface='someplace.com:8888')

        try_req(fail=True) # Fail, as no Pooled server instances (it cannot validate)

        # Validate one, but keep it active - not free for reqqing

        with FakeInstanceValidation():
            response = self.client.get('/instance/update', params)
            instance = ServerInstance.objects.get(user_interface='someplace.com:8888')
            instance.status = ServerInstance.STATUS.Active
            instance.save()

            try_req(fail=True) # Fail, as no server instances - the validated one is already active

            # And now we should succeed
            instance.status = ServerInstance.STATUS.Inactive
            instance.save()
            try_req(succeed=True, instance=instance)
            instance = ServerInstance.objects.get(user_interface='someplace.com:8888')
            self.assertEquals(instance.status, ServerInstance.STATUS.Preparing)
            self.assertEquals(instance.requisitioner, self.account)
            self.assertEquals(instance.activity, Activity.objects.get(uuid=activity_id))

            # Unreq
            response = self.client.post('/tracker/instance/unrequisition/%s/' % instance.uuid)
            url = get_redirect_url(response)
            self.assertContains(self.client.get(url), 'successfully')
            instance = ServerInstance.objects.get(user_interface='someplace.com:8888')
            self.assertEquals(instance.status, ServerInstance.STATUS.Inactive)
            self.assertEquals(instance.requisitioner, None)
            self.assertEquals(instance.activity, None)

    def testInitialData(self):
        init_user = User.objects.get(email='initial.data.creator@syntensity.com')
        self.assertEqual(init_user.is_active, False) # Do not allow this user to be logged in with

        init_asset = AssetInfo.objects.get(location='base/emptymap.tar.gz')
        init_asset_data = singleton_send(retrieve_asset, None, asset_uuid = init_asset.uuid)
        self.assertEquals(init_asset.kb_size, 1) # 1K, rounded up
        self.assertNotEquals(init_asset.hash_value, '')
        self.assertNotEquals(init_asset.hash_value, '?')
        self.assertEquals(len(init_asset.owners.all()), 1)
        init_account = UserAccount.objects.get(user=init_user)
        self.assertEquals(init_asset.owners.all()[0].uuid, init_account.uuid)

    def testNoActiveUsersByDefault(self):
        for user in User.objects.all():
            if user.username != self.user.username:
                self.assertFalse(user.is_active)

    def testGetAssetInfo(self):
        self.doLogin()
        asset_id = self.createAsset()
        asset = AssetInfo.objects.get(uuid=asset_id)
        self.assertRedirects(self.client.post('/tracker/asset/view/%s/' % asset_id, {
            'location': '/some/loc/', # Directory asset - so no content or url
            'owners': [owner.pk for owner in asset.owners.all()],
            'dependencies': [],
            'type_x': AssetInfo.TYPE.Both,
            'comment': 'hrm',
        }), '/tracker/asset/view/%s/' % asset_id)
        asset = AssetInfo.objects.get(uuid=asset_id)

        # Get the asset info
        response = self.client.get('/asset/getinfo', {
            'asset_id': asset_id,
        })
        self.assertEquals(response.status_code, 200)
        parsed = cgi.parse_qs(response.content)
        self.assertEquals(parsed['location'][0], asset.location)
        self.assertTrue('url' not in parsed)

        # And now with a non-directory asset
        asset = AssetInfo.objects.get(uuid=asset_id)
        self.assertRedirects(self.client.post('/tracker/asset/view/%s/' % asset_id, {
            'location': '/some/loc',
            'owners': [owner.pk for owner in asset.owners.all()],
            'dependencies': [],
            'type_x': AssetInfo.TYPE.Both,
            'comment': 'hrm',
        }), '/tracker/asset/view/%s/' % asset_id)
        asset = AssetInfo.objects.get(uuid=asset_id)

        # Get the asset info
        response = self.client.get('/asset/getinfo', {
            'asset_id': asset_id,
        })
        self.assertEquals(response.status_code, 200)
        parsed = cgi.parse_qs(response.content)
        self.assertEquals(parsed['location'][0], asset.location)
        self.assertEquals(parsed['url'][0], singleton_send(asset_download_redirect, None, uuid=asset_id))
        self.assertTrue('hash' not in parsed) # Hash value of '' is simply 'not there' in the parsed
        self.assertTrue('dependencies' not in parsed) # No deps
        self.assertEquals(parsed['type'][0], asset.type_x)

        # Test with dep
        dep_asset_id = self.createAsset()
        asset.dependencies.add(AssetInfo.objects.get(uuid=dep_asset_id))
        asset.save()
        response = self.client.get('/asset/getinfo', {
            'asset_id': asset_id,
        })
        self.assertEquals(response.status_code, 200)
        parsed = cgi.parse_qs(response.content)
        self.assertEquals(parsed['dependencies'][0], dep_asset_id)

        # Test with 2 deps - output as comma separated
        dep2_asset_id = self.createAsset()
        self.assertNotEquals(dep_asset_id, dep2_asset_id)
        dep2_asset_id = self.createAsset()
        asset.dependencies.add(AssetInfo.objects.get(uuid=dep2_asset_id))
        asset.save()
        response = self.client.get('/asset/getinfo', {
            'asset_id': asset_id,
        })
        self.assertEquals(response.status_code, 200)
        parsed = cgi.parse_qs(response.content)
        deps = parsed['dependencies'][0].split(',')
        self.assertEquals(set((dep_asset_id, dep2_asset_id)), set(deps)) # Order doesn't matter

        # Test 'recurse' with 2 deps - should get info on them all
        response = self.client.get('/asset/getinfo', {
            'asset_id': asset_id,
            'recurse': '1',
        })
        self.assertEquals(response.status_code, 200)
        parsed = cgi.parse_qs(response.content)

        self.assertEquals(set((asset_id, dep_asset_id, dep2_asset_id)), set(parsed['asset_id'][0].split('$')))
        self.assertEquals(len(set(parsed['location'][0].split('$'))), 3)
        self.assertTrue(asset.location in parsed['location'][0].split('$'))

        # Test hash
        asset.hash_value = 'xYz'
        asset.save()
        response = self.client.get('/asset/getinfo', {
            'asset_id': asset_id,
        })
        self.assertEquals(response.status_code, 200)
        parsed = cgi.parse_qs(response.content)
        self.assertEquals(parsed['hash'][0], asset.hash_value)

        self.doLogout()

    def testClientSession(self):
        # As if from the client program, not a web browser

        # Without a version string, fail
        self.assertContains(self.client.get('/user/startsession', { 'identifier': self.username }), 'Version+mismatch')

        # With bad (older) version
        self.assertContains(
            self.client.get('/user/startsession', { 'identifier': self.username, 'version': '0.' + INTENSITY_VERSION_STRING + '.2' }),
            'Version+mismatch'
        )

        # With ok (newer) version
        self.assertNotContains(
            self.client.get('/user/startsession', { 'identifier': self.username, 'version': '9.' + INTENSITY_VERSION_STRING + '.2' }),
            'Version+mismatch'
        )

        # Without a password, asks for the algo+salt
        response = self.client.get('/user/startsession', {
            'version': INTENSITY_VERSION_STRING,
            'identifier': self.username,
        })
        self.assertEquals(response.status_code, 200)
        parsed = cgi.parse_qs(response.content)
        self.assertEquals(parsed['algorithm'][0], 'sha1')
        self.assertTrue('salt' in parsed)
        self.assertTrue(len(parsed['salt'][0]) < 6)

        # With the password, tries to match
        response = self.client.get('/user/startsession', {
            'version': INTENSITY_VERSION_STRING,
            'identifier': self.username,
            'hashed_password': self.password+'wrong',
        })
        self.assertEquals(response.status_code, 200)
        parsed = cgi.parse_qs(response.content)
        self.assertTrue('error' in parsed) # Login error
        self.assertTrue('your_id' not in parsed) # Login error

        # Now with correct password - hashed correctly
        response = self.client.get('/user/startsession', {
            'version': INTENSITY_VERSION_STRING,
            'identifier': self.username,
            'hashed_password': self.user.password,
        })
        self.assertEquals(response.status_code, 200)
        parsed = cgi.parse_qs(response.content)
        self.assertEquals(parsed['your_id'][0], self.account.uuid)
        self.assertTrue('session_id' in parsed)

    def testClientInstanceLogin(self): # Client login to master and then to instance
        # Make instance
        instance_id = self.createInstance('uzer', 'admik', 5, 17)

        # Without instance session, any checklogin redirects to an instance login page
        response = self.client.get('/user/checklogin', {
            'instance_id': instance_id,
            'code': 'lolidk',
        })
        self.assertEquals(response.status_code, 302)

        # Log in to instance with bad creds - should fail
        session_id = ServerInstance.objects.get(uuid=instance_id).session_id
        response = self.client.get('/user/checklogin', {
            'instance_id': instance_id,
            'session_id': session_id,
            'code': 'lolidk',
        })
        self.assertEquals(response.status_code, 200)
        parsed = cgi.parse_qs(response.content)
        self.assertTrue('error' in parsed)
        self.assertTrue('success' not in parsed)

        # Log in to master
        old_session_id = UserAccount.objects.get(nickname=self.username).session_id
        response = self.client.get('/user/startsession', {
            'identifier': self.username,
            'hashed_password': self.user.password,
            'version': INTENSITY_VERSION_STRING,
        })
        self.assertEquals(response.status_code, 200)
        parsed = cgi.parse_qs(response.content)
        account_id = parsed['your_id'][0]
        session_id = parsed['session_id'][0]
        self.assertEquals(session_id, UserAccount.objects.get(nickname=self.username).session_id)
        self.assertNotEquals(session_id, old_session_id) # A new session ID should be created

        # Log in to instance with correct creds
        def try_instancelogin(can_edit):
            self.account = UserAccount.objects.get(user=self.user) # Get latest session_id from DB
            instance_session_id = ServerInstance.objects.get(uuid=instance_id).session_id
            response = self.client.get('/user/checklogin', {
                'instance_id': instance_id,
                'session_id': instance_session_id,
                'code': self.account.uuid + ',' + self.account.session_id,
            })
            self.assertEquals(response.status_code, 200)
            parsed = cgi.parse_qs(response.content)
            self.assertTrue('error' not in parsed)
            self.assertEquals(parsed['success'][0], '1')
            self.assertEquals(parsed['username'][0], self.account.nickname)
            self.assertEquals(parsed['can_edit'][0], can_edit) # No activity, but may be overridden by let_anyone_edit

        with IntensityConfSetting('Instances', 'let_anyone_edit', '0'):
            try_instancelogin('0')

        with IntensityConfSetting('Instances', 'let_anyone_edit', '1'):
            try_instancelogin('1')

        # Now make it an asset this user owns
        self.doLogin()
        asset_id = self.createAsset()
        activity_id = self.createActivity(asset_id)
        instance = ServerInstance.objects.get(uuid=instance_id)
        instance.activity = Activity.objects.get(uuid=activity_id)
        instance.save()

        response = self.client.get('/user/checklogin', {
            'instance_id': instance_id,
            'code': self.account.uuid + ',' + self.account.session_id,
        })
        parsed = cgi.parse_qs(response.content)
        self.assertEquals(parsed['can_edit'][0], '1') # Can edit our own asset now

        # Now change asset owners, and see that we can't
        asset = AssetInfo.objects.get(uuid=asset_id)
        asset.owners='zero'
        asset.save()

        with IntensityConfSetting('Instances', 'let_anyone_edit', '0'):
            response = self.client.get('/user/checklogin', {
                'instance_id': instance_id,
                'code': self.account.uuid + ',' + self.account.session_id,
            })
            parsed = cgi.parse_qs(response.content)
            self.assertEquals(parsed['can_edit'][0], '0') # Cannot edit other's assets

    def testInstanceUploadLog(self):
        # Make instance
        instance_id = self.createInstance('uzer', 'admik', 5, 17)
        session_id = ServerInstance.objects.get(user_interface='uzer').session_id

        class Content:
            emails = []

        def adder(sender, **kwargs):
            Content.emails.append(kwargs['body'])

        instance = ServerInstance.objects.get(uuid=instance_id)
        instance.status = ServerInstance.STATUS.Active

        with EmailSender(adder): # Add 'dummy' emailer for testing
            with IntensityConfSetting('Email', 'username', ''): # Disable normal email
                response = self.client.post('/instance/uploadlog', {
                    'instance_id': instance_id,
                    'session_id': session_id,
                    'error_log': '1234onluggage',
                })

            self.assertEquals(len(Content.emails), 1)
            self.assertTrue('1234onluggage' in Content.emails[0])
            self.assertTrue(self.username not in Content.emails[0]) # Didn't requisition

            instance = ServerInstance.objects.get(uuid=instance_id)
            self.assertEquals(instance.status, ServerInstance.STATUS.Inactive) # Has been deactivated after crash

            # With requisition, want username

            self.doLogin()
            asset_id = self.createAsset()
            activity_id = self.createActivity(asset_id)

            params = {
                'user_interface': 'someplace.com:8888',
                'admin_interface': 'admikal_irk',
                'players': 8,
                'max_players': 22,
                'validation': 'proofz',
                'version': INTENSITY_VERSION_STRING,
            }
            with FakeInstanceValidation():
                response = self.client.get('/instance/update', params)
                instance = ServerInstance.objects.get(user_interface='someplace.com:8888')
                instance.status = ServerInstance.STATUS.Inactive
                instance.save()

                with ReplaceRepurposing():
                    response = self.client.post('/tracker/activity/requisition/%s/' % activity_id)
                    instance = ServerInstance.objects.get(user_interface='someplace.com:8888')

                url = get_redirect_url(response)
                self.assertContains(self.client.get(url), 'successfully')

                instance = ServerInstance.objects.get(user_interface='someplace.com:8888')

                with IntensityConfSetting('Email', 'username', ''): # Disable normal email
                    response = self.client.post('/instance/uploadlog', {
                        'instance_id': instance.uuid,
                        'session_id': instance.session_id,
                        'error_log': '1234onluggage',
                    })

                self.assertTrue(self.username in Content.emails[1]) # Name appears

    def testPasswordChange(self):
        self.assertRedirects(self.client.get('/accounts/changepassword/'), 'http://testserver/accounts/login/?next=/accounts/changepassword/') # No accnt

        self.doLogin()
        self.assertContains(self.client.get('/accounts/changepassword/'), 'Enter a new password for your account')
        self.assertContains(self.client.post('/accounts/changepassword/'), 'is required')

        self.assertContains(self.client.post('/accounts/changepassword/', {
            'password1': 'imzzzzzzzzpossible',
            'password2': 'impossible',
        }), 'The two password fields didn')

        url = get_redirect_url(self.client.post('/accounts/changepassword/', {
            'password1': 'impossible',
            'password2': 'impossible',
        }))
        self.assertContains(
            self.client.get(url),
            'successfully changed',
        )

        # Fail to login with last password
        self.assertFalse(self.client.login(username=self.username, password=self.password))

        # Succeed with new one
        self.assertTrue(self.client.login(username=self.username, password='impossible'))

    def testUploadWithinClient(self):
        self.doLogin()
        asset_id = self.createAsset()

        self.account.session_id = '5'
        self.account.save()

        self.doLogout()

        # We upload as if from the client - using our session, not the normal user
        self.uploadAsset(asset_id, 'wzr', params={
            'user_id': self.account.uuid,
            'session_id': self.account.session_id,
        })


# TODO: In asset uploading, test for ownership / let_anyone_edit
# TODO: Clearer testing of http vs httpS urls in download urls


# Component tests

import intensity.component_manager as component_manager
component_tests = component_manager.load_tests()
for test in component_tests:
    # Make the test system aware of these
    exec "%s = test" % test.__name__

