
# Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
# This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

from django.contrib.auth.decorators import login_required

from intensity.models import UserAccount, ServerInstance

def login_or_session_required(function):
    def ret(request, *args, **kwargs):
        valid = False
        if request.user.is_authenticated():
            valid = True
        else:
            data = request.GET if request.method == 'GET' else request.POST
            entity = None
            if 'user_id' in data:
                entity = UserAccount.objects.get(uuid=data['user_id'])
            elif 'instance_id' in data:
                entity = ServerInstance.objects.get(uuid=data['instance_id'])

            if entity is not None:
                valid = ( entity.session_id != '' and entity.session_id == data.get('session_id', '') )
                if valid:
                    # Add user account to where it is expected. Note that for server instances we still use the term 'account'.
                    request.account = entity

        if valid:
            return function(request, *args, **kwargs)
        else:
            # Failure - use Django auth redirect method
            return login_required(function)(request, *args, **kwargs)

    return ret

