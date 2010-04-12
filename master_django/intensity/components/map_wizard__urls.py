
# Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
# This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

__COMPONENT_PRECEDENCE__ = 100


from django.conf.urls.defaults import *


urlpatterns = patterns('',
    (r'^tracker/tools/map_wizard/$', 'intensity.components.map_wizard__views.view'),
)

