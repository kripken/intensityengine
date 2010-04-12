
# Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
# This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

from intensity.signals import client_connect, client_disconnect
from intensity.base import quit


class Data:
    counter = 0

def add(sender, **kwargs):
    Data.counter += 1

client_connect.connect(add, weak=False)

def subtract(sender, **kwargs):
    Data.counter -= 1
    if Data.counter <= 0:
        quit()
        
client_disconnect.connect(subtract, weak=False)

