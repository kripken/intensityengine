
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

