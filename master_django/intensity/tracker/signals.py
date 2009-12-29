
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

