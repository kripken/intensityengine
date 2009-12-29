
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

