
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


import random, threading

from intensity.base import *
from intensity.logging import *
from intensity.signals import text_message, shutdown, client_connect, client_disconnect
from intensity.server.persistence import Clients
from intensity.base import get_config

from intensity.components.irclib.ircbot import SingleServerIRCBot
from intensity.components.irclib.irclib import nm_to_n, nm_to_h, irc_lower, ip_numstr_to_quad, ip_quad_to_numstr


# Parameters
SERVER = get_config('IRC', 'server', 'irc.freenode.net')
PORT = int(get_config('IRC', 'port', '8001'))
CHANNEL = get_config('IRC', 'channel', '#syntensity')
NICKNAME = get_config('IRC', 'nickname', 'ServerBot')
INSTANCE_NAME = get_config('IRC', 'instance_name', 'InstanceName')


## Sends a message to people connected to this Intensity Engine server
def send_intensity_message(speaker, text):
    CModule.send_text_message(-1, '[[IRC]] %s: %s' % (speaker, text), True)


class Bot(SingleServerIRCBot):
    def __init__(self, channel, nickname, server, port=6667):
        SingleServerIRCBot.__init__(self, [(server, port)], nickname, nickname)
        self.channel = channel

    def on_nicknameinuse(self, c, e):
        global NICKNAME
        NICKNAME = c.get_nickname() + "_"
        c.nick(NICKNAME)

    def on_welcome(self, c, e):
        c.join(self.channel)

    def on_pubmsg(self, c, e):
        speaker = e.source().split('!')[0]
        text = e.arguments()[0]

        INDICATOR = NICKNAME + ':'
        if text[0:len(INDICATOR)] == INDICATOR:
#            c.action(CHANNEL, '--sending messages to game servers is currently disabled--')
            text = text[len(INDICATOR):].lstrip()
            main_actionqueue.add_action(lambda : send_intensity_message(speaker, text))


# Prepare

bot = Bot(CHANNEL, NICKNAME, SERVER, PORT)

# Disconnect process

def leave_irc(sender, **kwargs):
    try:
        bot.disconnect()
    except Exception, e:
        log(logging.ERROR, "IRC plugin error: " + str(e));
shutdown.connect(leave_irc, weak=False)

# Handle our own text messages

def get_client_name(client_number):
    try:
        client = Clients.get(client_number)
        return client.username
    except Exception, e:
        return '?'

def get_instance_name():
    return '[' + INSTANCE_NAME + '] '

def on_text_message(sender, **kwargs):
    client_number = kwargs['client_number']
    text = kwargs['text']

    try:
        bot.connection.action(CHANNEL, get_instance_name() + get_client_name(client_number) + ': ' + text)
    except Exception, e:
        log(logging.ERROR, "IRC plugin error: " + str(e));

text_message.connect(on_text_message, weak=False)

# Handle login-logout

def login(sender, **kwargs):
    client_number = kwargs['client_number']
    try:
        bot.connection.action(CHANNEL, get_instance_name() + get_client_name(client_number) + ' has connected')
    except Exception, e:
        log(logging.ERROR, "IRC plugin error: " + str(e));

client_connect.connect(login, weak=False)

def logout(sender, **kwargs):
    client_number = kwargs['client_number']
    try:
        bot.connection.action(CHANNEL, get_instance_name() + get_client_name(client_number) + ' has disconnected')
    except Exception, e:
        log(logging.ERROR, "IRC plugin error: " + str(e));
client_disconnect.connect(logout, weak=False)

# Run bot

def monitor_irc():
    bot.start()
thread = threading.Thread(target=monitor_irc)
thread.setDaemon(True)
thread.start()

