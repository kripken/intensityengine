
# Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
# This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

"""
Listener on the separate admin port, which allows repurposing the instance
and other stuff. As opposed to the main interface/port, which is for the
actual game.
"""

import os, cgi, BaseHTTPServer, SimpleHTTPServer, urlparse
import threading

from intensity.base import *

class AdminListenerRequestHandler(SimpleHTTPServer.SimpleHTTPRequestHandler):
    def do_GET(self):
        parsed = urlparse.urlparse(self.path)

        print "GET", self.path, parsed.path

        if parsed.path == "/repurpose":
            if InstanceStatus.map_loaded:
                self.send_response(500)
                self.send_header('Warning', 'Already running a map')
                self.end_headers()
                return

            # If we were in standby mode, leave it - we have been manually repurposed
            InstanceStatus.in_standby = False

            query = cgi.parse_qs(parsed.query)
            assert(get_config('Network', 'instance_id', "XXXX") == query['instance_id'][0])
            update_master() # This will change the map, as the master server will tell us to

            # Redirect the user
            if 'return_to' in query:
                return_to = query['return_to'][0]
                self.send_response(302)
                self.send_header('Location', return_to)
                self.end_headers()
            else:
                self.send_response(200)
                self.end_headers()

        elif parsed.path == "/unpurpose":
            self.send_response(200)
            self.end_headers()

            print "Shutting down due to unpurposing"
            quit()

        else:
            print "Admin listener error: Got a request: %s" % self.path


## Thread for admin listening
class AdminListenerThread(threading.Thread):
    def run(self):
        port = int(get_config('Network', 'admin_port', 28789))
        print "Starting threaded admin listener on port:", port

        server_class = BaseHTTPServer.HTTPServer
        handler_class = AdminListenerRequestHandler
        server_address = ('', port)
        httpd = server_class(server_address, handler_class)

        httpd.serve_forever()


# Main

admin_listener_thread = AdminListenerThread()
admin_listener_thread.setDaemon(True) # Do not stop quitting when the main thread quits
admin_listener_thread.start()


from intensity.server.auth import update_master, InstanceStatus

