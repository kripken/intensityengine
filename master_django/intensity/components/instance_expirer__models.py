
# Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
# This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

'''
Expires server instances after a while has passed since they
updated the master.
'''

__COMPONENT_PRECEDENCE__ = 100

from datetime import datetime

from intensity.tracker.signals import list_instances


STALE_SECONDS = 15*60 # 15 minutes

def is_stale(last_update):
    return (datetime.now() - last_update).seconds > STALE_SECONDS

def expire_stale(sender, **kwargs):
    instances = kwargs['instances']
    for instance in instances:
        last_update = instance.last_update
        if is_stale(last_update):
            instance.delete()

list_instances.connect(expire_stale)

