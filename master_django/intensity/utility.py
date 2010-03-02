
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


from django.core.files import File
from django.contrib.auth.models import User

from intensity.models import AssetInfo, UserAccount
from intensity.signals import multiple_send
from intensity.tracker.signals import store_asset


##
## Injects a local file as content into an asset. Creates the asset if it does
## not exist, using a specified username, otherwise uses the existing asset
## (so the original owner will remain, etc.).
##
## Example usage (in Django shell):
##      >> from intensity.utility import *
##      >> inject_asset('textures/gk/metal.tar.gz', '../upload/metal.tar.gz')
## To import an entire directory:
##      >> from intensity.utility import *
##      >> import os
##      >> for name in os.listdir('../upload/'): inject_asset('textures/gk/' + name, '../upload/' + name)
##
def inject_asset(location, filename, username = '__initial_data_creator__'):
    assets = AssetInfo.objects.filter(location=location)
    if len(assets) == 0:
        user = User.objects.get(username=username)
        account = UserAccount.objects.get(user=user)

        asset = AssetInfo.objects.create(
            location = location,
            hash_value = '',
            type_x = AssetInfo.TYPE.Both,
            kb_size = 0,
            comment = '',
        )
        asset.owners.add(account)
        asset.save()
    else:
        asset = assets[0]

    multiple_send(store_asset, None, asset = asset, asset_file = File(open(filename, 'rb')))


class DummyRequest:
    def __init__(self, account=None):
        if account is None:
            self.user = None
            self.account = None
        else:
            self.user = account.user
            self.account = account

        self.session = {}
        self.GET = {}
        self.POST = {}


## Clean up non-ascii: Also do it for name in Activity
'''
for x in AssetInfo.objects.all():
    try:
        print x.location + x.comment
    except UnicodeEncodeError:
        print x, "is bad"
###        x.delete()
'''

