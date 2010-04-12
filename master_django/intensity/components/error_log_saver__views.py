
# Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
# This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

from django.contrib.auth.decorators import login_required
from django.http import HttpResponse

from intensity.components.error_log_saver__models import get_error_log, get_last_error_log_index


@login_required
def view(request, index=-1):
    if index == -1:
        index = get_last_error_log_index(request.account)
    else:
        index = int(index)

    return HttpResponse(get_error_log(request.account, index), mimetype='text/plain')

