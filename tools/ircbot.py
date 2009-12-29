
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


import threading, sys, os, time

sys.path += [ os.path.join(os.sep.join(os.path.dirname(__file__).split(os.sep)[:-1]), 'src', 'python')]

from intensity.components.irclib.ircbot import SingleServerIRCBot
from intensity.components.irclib.irclib import nm_to_n, nm_to_h, irc_lower, ip_numstr_to_quad, ip_quad_to_numstr


# Parameters
SERVER = 'irc.freenode.net'
PORT = 8001
CHANNEL = '#syntensity'
NICKNAME = 'UtilityBot'
FILE = sys.argv[1]


## Sends a message to people connected to this Intensity Engine server
def send_intensity_message(speaker, text):
    CModule.send_text_message(-1, '[[IRC]] %s: %s' % (speaker, text))


class Bot(SingleServerIRCBot):
    def __init__(self, channel, nickname, server, port=6667):
        SingleServerIRCBot.__init__(self, [(server, port)], nickname, nickname)
        self.channel = channel

    def on_nicknameinuse(self, c, e):
        global NICKNAME
        NICKNAME = c.get_nickname() + "_"
        c.nick(NICKNAME)

    def on_welcome(self, c, e):
        print "Welcomed"
        c.join(self.channel)

    def on_pubmsg(self, c, e):
        speaker = e.source().split('!')[0]
        text = e.arguments()[0]

        msg = '%s [%s] %s' % (time.strftime('%Y-%m-%d %H-%M-%S'), speaker, text)
        print msg
        f = open(FILE, 'a')
        f.write(msg + '\n')
        f.close()

Bot.on_pubnotice = Bot.on_pubmsg
Bot.on_action = Bot.on_pubmsg

# Prepare

bot = Bot(CHANNEL, NICKNAME, SERVER, PORT)

# Handle our own text messages

def on_text_message(sender, **kwargs):
    bot.connection.action(CHANNEL, get_instance_name() + get_client_name(client_number) + ': ' + text)

# Run bot

def monitor_irc():
    bot.start()
thread = threading.Thread(target=monitor_irc)
thread.setDaemon(True)
thread.start()

print "Looping..."

while True:
    time.sleep(1.0)

bot.disconnect()

