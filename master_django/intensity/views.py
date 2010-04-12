
# Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
# This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

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

