
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


from django.http import HttpResponseRedirect
from django.shortcuts import render_to_response
from django.template import RequestContext


def getpost_form(request, template, form_class, on_valid, finish_url, form_params={}):
    if request.method == 'POST':
        form = form_class(request.POST, **form_params)
        if form.is_valid():
            on_valid(form)
            return HttpResponseRedirect(finish_url)
    else:
        form = form_class(**form_params)

    return render_to_response(template, {
        'form': form,
    }, context_instance=RequestContext(request))

