
# Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
# This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

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

