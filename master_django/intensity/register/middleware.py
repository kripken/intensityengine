
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


class LazyAccount(object):
    def __get__(self, request, obj_type=None):
        if not hasattr(request, '_cached_account'):
            from intensity.models import UserAccount
            request._cached_account = UserAccount.objects.get(user=request.user)
        return request._cached_account

class AccountMiddleware(object):
    def process_request(self, request):
        assert hasattr(request, 'user'), "The Intensity Engine account middleware requires auth middleware to be installed. Edit your MIDDLEWARE_CLASSES setting to insert 'django.contrib.auth.middleware.AuthenticationMiddleware'."
        request.__class__.account = LazyAccount()
        return None

