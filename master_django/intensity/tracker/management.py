
# Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
# This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

import os

from django.db.models.signals import post_save, post_syncdb

from intensity.models import AssetInfo
from intensity.signals import singleton_send
from intensity.tracker.signals import initialize_asset_storage


## Set up some initial non-database data, like the 'emptymap' map asset

def need_initial_data(sender, **kwargs):
    from django.conf import settings
    if settings.DATABASE_NAME != ':memory:':
        # We are not testing, create the non-DB asset storage now
        need_more = singleton_send(initialize_asset_storage, None, asset=kwargs['instance'])
        if not need_more:
            post_save.disconnect(need_initial_data)

post_save.connect(need_initial_data, sender=AssetInfo, weak=False)

