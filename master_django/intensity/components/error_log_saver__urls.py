
# Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
# This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

__COMPONENT_PRECEDENCE__ = 100


from django.conf.urls.defaults import *


urlpatterns = patterns('',
    (r'^tracker/account/error_log/view/$', 'intensity.components.error_log_saver__views.view'),
    (r'^tracker/account/error_log/view/(\w+)/$', 'intensity.components.error_log_saver__views.view'),
)

