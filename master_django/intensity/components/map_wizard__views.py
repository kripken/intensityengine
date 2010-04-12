
# Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
# This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

import logging, re

from django.contrib.auth.decorators import login_required
from django.views.generic.simple import direct_to_template

from intensity.models import AssetInfo, Activity, ServerInstance
from intensity.views import getpost_form
from intensity.components.map_wizard__forms import MapWizardForm
from intensity.utility import DummyRequest
from intensity.tracker.views import asset_clone, activity_new, activity_requisition
from intensity.components.instance_selector__views import select


## Do the actual work
def wizard(account, location, original, requisition):
    dummy_request = DummyRequest(account)

    # Clone the original asset

    response = asset_clone(dummy_request, original.uuid)
    url = response['Location']
    m = re.match('/tracker/asset/view/(\w+)/$', url)
    asset_id = m.group(1)
    assert(asset_id != original.uuid)
    asset = AssetInfo.objects.get(uuid=asset_id)
    asset.location = location
    asset.save()

    # Create an activity

    dummy_request.POST['asset_id'] = asset_id
    response = activity_new(dummy_request)
    url = response['Location']
    m = re.match('/tracker/activity/view/(\w+)/$', url)
    activity_id = m.group(1)
    activity = Activity.objects.get(uuid=activity_id)
    activity.name = location.replace('base/', '').replace('.tar.gz', '') + ' v1.0'
    activity.save()

    # If requested, requisition and select a server
    if requisition:
        # Requisition
        activity_requisition(dummy_request, activity_id)
        message = dummy_request.session['message'].lower()
        success = 'success' in message
        if not success:
            return 'Map creation wizard made your map asset and activity, but could not find a server. Try to requisition one later for your new activity.'

        # Select
        instances = ServerInstance.objects.filter(activity = activity).filter(requisitioner = account)
        assert(len(instances) == 1)
        select(dummy_request, instances[0].uuid)

    return True


@login_required
def view(request):
    def on_valid(form):
        logging.info('Map wizard - view')

        result = wizard(request.account, form.cleaned_data['location'], form.cleaned_data['original'], form.cleaned_data['requisition'])

        if type(result) is str:
            request.session['message'] = result
        else:
            if form.cleaned_data['requisition']:
                request.session['message'] = 'Map creation wizard finished successfully. To start editing your map, open the client program, log in and then select "connect to selected."'
            else:
                request.session['message'] = 'Map creation wizard finished successfully. To start editing your map, run it on a server and connect to that server.'

    return getpost_form(request, 'map_wizard.html', MapWizardForm, on_valid, '/tracker/account/')

