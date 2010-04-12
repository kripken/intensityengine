
# Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
# This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

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

