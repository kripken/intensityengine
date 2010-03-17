
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


import threading, time, urllib

from intensity.base import *
from intensity.logging import *
import intensity.components.thirdparty.BeautifulSoup as BeautifulSoup
from intensity.server.persistence import Clients


def parse_servers(html):
    class Class: pass
    servers = []
    soup = BeautifulSoup.BeautifulSoup(html)
    trs = soup.findAll('tr')
    for tr in trs:
        if 'www.syntensity.com' in str(tr.contents):
            try:
                server = Class()
                server.host, server.port = tr.contents[1].contents[0].split(':')
                server.activity = str(tr.contents[5].contents[0].contents[0]).replace('"', '').replace("'", '').replace('/', '').replace('\\', '').replace('\n', '').replace('\r', '')
                server.players, server.max_players = map(int, tr.contents[7].contents[0].split('/'))
                servers.append(server)
            except:
                pass
    return servers

## Selects and sorts the servers
def select_relevant_servers(servers):
    servers = filter(lambda server: int(server.port) > 10006, servers) # XXX XXX XXX horrible hard-coding! FIXME
#    servers = filter(lambda server: server.players > 0, servers)
    servers.sort(lambda x, y: x.players - y.players)
    return servers

# Main

def scraper():
    # Get area markers in which to place portals
    class Markers:
        left = None
        right = None

#    left = CModule.run_script_string('getEntity(80).position.toString()', 'parse_instances')[1:-1].split(',')
#    right = CModule.run_script_string('getEntity(52).position.toString()', 'parse_instances')[1:-1].split(',')

    while True:
        if Clients.count() == 0:
            time.sleep(1.5)
            continue

        # Get HTML and parse
        html = urllib.urlopen('http://www.syntensity.com:8888/tracker/instances/').read()
        servers = parse_servers(html)
        servers = select_relevant_servers(servers)

        # Apply to running script engine - done in main thread
        def main_operations():
            if not CModule.has_engine(): return

            if Markers.left is None:
                Markers.left = CModule.run_script_string('getEntityByTag("portals_left").position.toString()', 'parse_instances')[1:-1].split(',')
                Markers.right = CModule.run_script_string('getEntityByTag("portals_right").position.toString()', 'parse_instances')[1:-1].split(',')
                Markers.left = map(float, Markers.left)
                Markers.right = map(float, Markers.right)
                Markers.diff = [Markers.right[0]-Markers.left[0], Markers.right[1]-Markers.left[1], Markers.right[2]-Markers.left[2]]

            curr = Markers.left[:] # copy

            new_servers = []

            for server in servers:
                server.text = ('%s (%d/%d)' % (server.activity, server.players, server.max_players))
                # Check for existing
                uid = CModule.run_script_int(str('''
                    var __temp = filter(function(entity) {
                        return entity.targetIP === '%s' && entity.targetPort === %s;
                    }, getEntitiesByClass(getEntityClass('Portal')));
                    __temp.length === 1 ? __temp[0].uniqueId : -1;
                ''' % (server.host, server.port)), 'parse_instances')
                if uid != -1:
                    # Update portal
                    CModule.run_script(str('''
                        getEntity(%s).portalText = '%s';
                    ''' % (uid, server.text)), 'parse_instances')
                else:
                    new_servers.append(server)

            for server in new_servers:
                # Create portal
                CModule.run_script(str('''
                    GameManager.getSingleton().newDynaPortal = {
                        text: '%s',
                        x: %f,
                        y: %f,
                        z: %f,
                        targetIP: '%s',
                        targetPort: %s,
                        secondsLeft: 12,
                    };
                ''' % (
                    server.text,
                    curr[0], curr[1], curr[2],
                    server.host, server.port
                )), 'parse_instances')

                if len(new_servers) > 1:
                    for i in range(3):
                        curr[i] += Markers.diff[i]/(len(new_servers) - 1)

        main_actionqueue.add_action(main_operations)

        # Sleep
        time.sleep(12.5) # So our servers never overlap


thread = threading.Thread(target=scraper)
thread.setDaemon(True)
thread.start()

