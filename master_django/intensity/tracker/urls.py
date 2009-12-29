
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


from django.conf.urls.defaults import *
from django.views.generic.simple import direct_to_template
from django.views.generic.list_detail import object_detail
from django.contrib.auth.decorators import login_required
from django.views.decorators.cache import cache_page

from intensity.models import Activity
from intensity.tracker.views import instances


urlpatterns = patterns('intensity.tracker.views',
    (r'overview/$', direct_to_template, {'template': 'tracker/overview.html'}),
    (r'account/$', 'account'),
    (r'account/view/(\w+)/$', 'account'),
    # Small amount of caching, but quite useful if a lot of people 'hammer' the server instances page for changes
    (r'instances/$', cache_page(instances, 5)),
    (r'activities/$', 'activities'),
    (r'forum/$', direct_to_template, {'template': 'tracker/forum.html', 'extra_context': {'forum': 1}}),
#    (r'activity/(?P<object_id>\d+)/$', object_detail, {'queryset': Activity.objects.all(), 'template_name': 'tracker/activity.html'}),

    (r'activity/view/(\w+)/$', 'activity'),
    (r'activity/new/$', 'activity_new'),
    (r'activity/delete/(\w+)/$', 'activity_delete'),
    (r'activity/requisition/(\w+)/$', 'activity_requisition'),

    (r'instance/unrequisition/(\w+)/$', 'instance_unrequisition'),

    (r'assets/$', 'assets'),
#    (r'activity/(?P<object_id>\d+)/$', object_detail, {'queryset': Activity.objects.all(), 'template_name': 'tracker/activity.html'}),
    (r'asset/view/(\w+)/$', 'asset'),
    (r'asset/new/$', 'asset_new'),
    (r'asset/upload/(\w+)/$', 'asset_upload'),
    (r'asset/download/(\w+)/$', 'asset_download'),
    (r'asset/delete/(\w+)/$', 'asset_delete'),
    (r'asset/clone/(\w+)/$', 'asset_clone'),
)

