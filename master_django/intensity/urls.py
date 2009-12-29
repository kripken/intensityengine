
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
from django.views.generic.simple import direct_to_template, redirect_to

from intensity.register.views import register, reset_password, change_password, signup_for_notify

urlpatterns = patterns('',
    (r'^$', redirect_to, {'url': '/tracker/overview/'}),
    (r'^tracker/', include('intensity.tracker.urls')),
)

# Legacy master urls
urlpatterns += patterns('intensity.tracker.views',
    (r'instance/update$', 'instance_update'),
    (r'asset/getinfo$', 'asset_getinfo'),
    (r'user/startsession$', 'account_startsession'),
    (r'user/checklogin$', 'account_instancelogin'),
    (r'instance/uploadlog$', 'instance_uploadlog'),
)

urlpatterns += patterns('django.contrib.auth.views',
    (r'^accounts/$', 'login'),
    (r'^accounts/profile/$',  redirect_to, {'url': '/tracker/account/'}),
    (r'^accounts/login/$',  'login'),
    (r'^accounts/logout/$', 'logout'),

    (r'^accounts/register/$', register),
    (r'^accounts/register/finish/$', direct_to_template, {'template': 'registration/register_finish.html'}),
    (r'^accounts/changepassword/$', change_password),
    (r'^accounts/changepassword/finish/$', direct_to_template, {'template': 'registration/password_change_finish.html'}),
    (r'^accounts/resetpassword/$', reset_password),
    (r'^accounts/resetpassword/finish/$', direct_to_template, {'template': 'registration/password_reset_finish.html'}),
#    (r'^reset/(?P<uidb36>[0-9A-Za-z]+)-(?P<token>.+)/$', 'password_reset_confirm'),
    (r'^reset/done/$', 'password_reset_complete'),
    (r'^accounts/register/signup_for_notify/$', signup_for_notify),
    (r'^accounts/register/signup_for_notify/finish/$', direct_to_template, {'template': 'registration/signup_for_notify_finish.html'}),
)

## Import components
import intensity.component_manager as component_manager

urlpatterns += component_manager.load_urls()

