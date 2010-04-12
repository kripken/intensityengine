
# Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
# This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

__COMPONENT_PRECEDENCE__ = 100


from django.conf.urls.defaults import *
from django.views.generic.simple import direct_to_template, redirect_to


urlpatterns = patterns('',
    (r'^tracker/instance/select/(\w+)/$', 'intensity.components.instance_selector__views.select'),
    (r'^tracker/instance/getselected/$', 'intensity.components.instance_selector__views.getselected'),
)

