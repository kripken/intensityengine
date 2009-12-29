
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


from intensity.models import UserAccount
import intensity.conf as intensity_conf

def account(request):
    '''
    A context processor that provides 'my_account', the Intensity Engine account info for a user,
    and shows messages for that account
    '''
    ret = {
        'my_account': request.account if request.user.is_authenticated() else None,
        'message': request.session.get('message'),
    }
    request.session['message'] = None
    return ret

def toplevel(request):
    '''
    Gives a redirect URL for the toplevel
    '''
    return { 'toplevel_root': intensity_conf.get('Sites', 'toplevel_root') }

