
# Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
# This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

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

