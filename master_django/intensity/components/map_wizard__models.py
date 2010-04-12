
# Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
# This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

from __future__ import with_statement

__COMPONENT_PRECEDENCE__ = 100

import os, threading

from django.utils.safestring import SafeString

from intensity.tracker.signals import account_tools
import intensity.conf as intensity_conf


def map_wizard(sender, **kwargs):
    return SafeString('''<input type="button" onclick="window.location.href='/tracker/tools/map_wizard/'" value="Map creation wizard">''')

account_tools.connect(map_wizard)

