
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


from django.http import HttpResponse, HttpResponseRedirect
from django.views.generic.simple import direct_to_template
from django.contrib.auth.decorators import login_required
from django import dispatch
from django.core.servers.basehttp import FileWrapper

from intensity.models import AssetInfo
from intensity.signals import singleton_send, multiple_send
from intensity.tracker.signals import store_asset, retrieve_asset
from intensity.register.decorators import login_or_session_required
import intensity.conf as intensity_conf


@login_or_session_required
def upload(request, uuid):
    asset = AssetInfo.objects.get(uuid=uuid)
    if intensity_conf.get('Instances', 'let_anyone_edit') != '1':
        owner_uuids = [owner.uuid for owner in asset.owners.all()]
        assert request.account.uuid in owner_uuids, 'You must be an owner of this asset to upload content'

    if request.method == 'GET':
        return direct_to_template(request, template="do_upload.html", extra_context = {'asset': asset})
    else:
        # Receive the uploaded file, and hand off reception to the appropriate service provider
        asset_file = request.FILES['file']
        ret = multiple_send(store_asset, None, asset=asset, asset_file=asset_file)
        if reduce(lambda x, y: x and y, ret) is True:
            request.session['message'] = 'Upload was successful.'
            return HttpResponseRedirect('/tracker/asset/view/%s/' % asset.uuid)
        else:
            raise Exception(str(ret))

@login_or_session_required
def download(request, uuid):
    asset = AssetInfo.objects.get(uuid=uuid)

    # TODO: Check permission to download this asset - add signal. Meanwhile allow all logged in users to read anything

    asset_file = singleton_send(retrieve_asset, None, asset_uuid = uuid)

    response = HttpResponse(FileWrapper(asset_file), content_type='application/octet-stream')
    response['Content-Disposition'] = 'attachment; filename=%s' % (uuid)
#    response['Content-Length'] = asset_file.tell()
    return response

