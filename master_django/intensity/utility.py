
# Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
# This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

from django import forms
from django.core.files import File
from django.contrib.auth.models import User
from django.utils.translation import ugettext_lazy as _

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


## Clean up non-ascii:
'''
from intensity.models import *
for x in AssetInfo.objects.all():
    try:
        h = str(x.location)
        g = str(x.comment)
    except UnicodeEncodeError:
        print x, "is bad"
###        x.delete()
# AssetInfo.objects.filter(location__startswith="base/...") # & fix it
for x in Activity.objects.all():
    try:
        h = str(x.name)
    except UnicodeEncodeError:
        print x, "is bad"
# Activity.objects.filter(name__startswith="...") # & fix it
'''
def check_ascii(value):
    try:
        temp = str(value)
    except UnicodeEncodeError:
        raise forms.ValidationError(_("Non-ascii characters are not allowed in this field."))
    return value

