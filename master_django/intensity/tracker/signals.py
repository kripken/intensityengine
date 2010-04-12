
# Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
# This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

from django import dispatch

## Assets

# Generate a redirect URL for uploading an asset. This might be on
# a separate asset server, for example
asset_upload_redirect = dispatch.Signal(providing_args=['uuid'])

asset_download_redirect = dispatch.Signal(providing_args=['uuid'])

asset_delete_trigger = dispatch.Signal(providing_args=['uuid'])


## Handle an uploaded asset. The service provider should save
## it (locally/remotely/etc.) and return true if successful,
## or the error message
store_asset = dispatch.Signal(providing_args=['asset', 'asset_file'])

## Retrieve a previously stored asset. Returns an opened file
## that can be read normally.
retrieve_asset = dispatch.Signal(providing_args=['asset_uuid'])

initialize_asset_storage = dispatch.Signal(providing_args=['asset'])
destroy_asset_storage = dispatch.Signal()


## Server instances

requisition_instance = dispatch.Signal(providing_args=['activity'])

unrequisition_instance = dispatch.Signal(providing_args=['instance'])

validate_instance = dispatch.Signal(providing_args=['instance', 'validation'])

## Allows additional columns of data to be shown in the instance list
list_instances = dispatch.Signal(providing_args=['instances', 'request'])

pre_instance_update = dispatch.Signal(providing_args=['instance'])
post_instance_update = dispatch.Signal(providing_args=['instance'])

## Error log for an instance
receive_error_log = dispatch.Signal(providing_args=['instance', 'requisitioner', 'content'])


## Things to appear as 'tools' in the account page
account_tools = dispatch.Signal(providing_args=['account'])

