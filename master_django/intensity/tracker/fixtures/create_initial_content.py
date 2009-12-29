
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

from django.core.files import File

from intensity.models import AssetInfo
from intensity.signals import multiple_send
from intensity.tracker.signals import store_asset


INITIAL_ASSET_LOCATIONS = set(['emptymap.tar.gz', 'storming.tar.gz'])
INITIAL_ASSET_LOCATIONS__ORIGINAL = INITIAL_ASSET_LOCATIONS.copy()

def create_initial_asset_content(asset):
    short_location = asset.location.replace('base/', '')
    if short_location in INITIAL_ASSET_LOCATIONS:
        INITIAL_ASSET_LOCATIONS.remove(short_location) # Do before send, to prevent recursion

        # Upload fixture data
        data = open(
            os.path.join(os.path.dirname(os.path.abspath(__file__)), short_location),
            'rb'
        )
        multiple_send(store_asset, None, asset = asset, asset_file = File(data))

        return len(INITIAL_ASSET_LOCATIONS) != 0 # Whether we need more
    else:
        return True # For all we know, we do need more

