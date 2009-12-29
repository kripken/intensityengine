
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

