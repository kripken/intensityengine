
# Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
# This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

import logging, random, urllib

from django.http import HttpResponse, HttpResponseRedirect
from django.views.generic.simple import direct_to_template
from django.contrib.auth.decorators import login_required

from intensity.models import ServerInstance, UserAccount
from intensity.components.instance_selector__models import SelectedInstance
from intensity.register.decorators import login_or_session_required
import intensity.conf as intensity_conf


@login_required
def select(request, uuid):
    instance = ServerInstance.objects.get(uuid=uuid)
    selection, created = SelectedInstance.objects.get_or_create(account = request.account)
    selection.instance = instance
    selection.save()
    request.session['message'] = 'Instance selected. In the client program, log in and then select "connect to selected."'
    return HttpResponseRedirect('/tracker/instances/')

@login_or_session_required
def getselected(request):
    instance = None
    try:
        instance = SelectedInstance.objects.get(account = request.account).instance
    except SelectedInstance.DoesNotExist:
#        return HttpResponse(urllib.urlencode([
#            ('error', "You haven't selected a server."),
#        ]))

        # Find a 'good' random server to connect to, even though one isn't selected -  temporary
        # workaround until we have a nice UI for this stuff

        possibles = intensity_conf.get('InstanceSelector', 'possibles', '')
        if possibles == '':
            instances = ServerInstance.objects.filter(activity__isnull=False) # All instances running activities
        else:
            possibles = possibles.split(',')
            instances = []
            for possible in possibles:
                try:
                    instance = ServerInstance.objects.get(user_interface=possible)
                except ServerInstance.DoesNotExist:
                    continue
                if instance.activity is not None:
                    instances.append(instance)

        instances = filter(lambda instance: instance.players < instance.max_players, instances) # Non-full
        instances = filter(lambda instance: 'localhost' not in instance.user_interface, instances) # No localhosts

        if len(instances) == 0:
            return HttpResponse(urllib.urlencode([
                ('error', "All servers (in main pool) are full."),
            ]))

        weights = map(lambda instance: 1.0/(instance.max_players - instance.players), instances) # More weight to nearly-full
        total = sum(weights)
        weights = map(lambda weight: weight/total, weights)
        x = random.random()
        for i in range(len(instances)):
            if weights[i] >= x:
                instance = instances[i]
                break
            x -= weights[i]

        if instance == None:
            logging.warning('An error occurred in randomly generating an instance')
            instance = random.choice(instances)

    return HttpResponse(urllib.urlencode([
        ('interface', instance.user_interface),
    ]))

