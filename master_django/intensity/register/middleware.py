
# Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
# This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

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

