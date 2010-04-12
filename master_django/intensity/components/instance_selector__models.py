
# Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
# This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

__COMPONENT_PRECEDENCE__ = 100

from django.db import models
from django.utils.safestring import SafeString

from intensity.models import UserAccount, ServerInstance
from intensity.tracker.signals import list_instances


# Show button

def show(sender, **kwargs):
    instances = kwargs['instances']
    request = kwargs['request']

    if not request.user.is_authenticated(): return

    try:
        selected_instance_uuid = SelectedInstance.objects.get(account = request.account).instance.uuid
    except: # No such record, or instance is None
        selected_instance_uuid = None

    for instance in instances:
        if not hasattr(instance, 'component_data'): instance.component_data = SafeString('')
        if instance.uuid != selected_instance_uuid:
            instance.component_data += SafeString('''<td><input type="button" onclick="window.location.href = '/tracker/instance/select/%s/'" value="Select"></td>''' % instance.uuid)
        else:
            instance.component_data += SafeString('''<td><b><center>Selected</b></center></td>''')

list_instances.connect(show)


# Stored data

class SelectedInstance(models.Model):
    account = models.OneToOneField(UserAccount)
    instance = models.ForeignKey(ServerInstance, blank=True, null=True)

