
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


# TODO: Separate thread for no lag/lockups, and adjust left/right volumes for spatial

from intensity.base import *
from intensity.logging import *
from intensity.signals import signal_component

from intensity.components.thirdparty import vlc

# Globals

print "Loading VLC component"

instance=vlc.Instance()
player=instance.media_player_new()
instance.audio_set_volume(50)

# Safely do things in a side thread

actionqueue = SafeActionQueue()
actionqueue.main_loop()

# Main entry point

def receive(sender, **kwargs):
    component_id = kwargs['component_id']
    data = kwargs['data']

    print data

    def act():
        try:
            if component_id == 'VLC':
                parts = data.split('|')
                command = parts[0]
                params = '|'.join(parts[1:])
                if command == 'play':
                    if params != '':
                        media=instance.media_new(params)
                        player.set_media(media)
                        player.play()
                    else:
                        player.stop()
                elif command == 'setvolume':
                    instance.audio_set_volume(int(params))
        except Exception, e:
            log(logging.ERROR, "Error in VLC component: " + str(e))

    actionqueue.add_action(act)

    return ''

signal_component.connect(receive, weak=False)

# Examples:
#receive(None, **{ 'component_id': 'VLC', 'data': 'play|http://scfire-mtc-aa02.stream.aol.com:80/stream/1022' })
#receive(None, **{ 'component_id': 'VLC', 'data': 'setvolume|10' })
#receive(None, **{ 'component_id': 'VLC', 'data': 'play|' }) # Stop playing

