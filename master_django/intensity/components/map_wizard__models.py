
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

__COMPONENT_PRECEDENCE__ = 100

import os, threading

from django.utils.safestring import SafeString

from intensity.tracker.signals import account_tools
import intensity.conf as intensity_conf


def map_wizard(sender, **kwargs):
    return SafeString('''<input type="button" onclick="window.location.href='/tracker/tools/map_wizard/'" value="Map creation wizard">''')

account_tools.connect(map_wizard)

