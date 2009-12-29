
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


from intensity.tracker.signals import asset_upload_redirect, asset_download_redirect
import intensity.conf as intensity_conf


__COMPONENT_PRECEDENCE__ = 100


def base():
    scheme = 'http://' if intensity_conf.get('Network', 'auth') == '0' else 'https://'
    return scheme + intensity_conf.get('Network', 'address') + ':' + intensity_conf.get('Network', 'port')

def create_upload_url(sender, **kwargs):
    asset_uuid = kwargs['uuid']
    return base() + '/tracker/asset/do_upload/%s/' % asset_uuid

asset_upload_redirect.connect(create_upload_url)

def create_download_url(sender, **kwargs):
    asset_uuid = kwargs['uuid']
    return base() + '/tracker/asset/do_download/%s/' % asset_uuid

asset_download_redirect.connect(create_download_url)

