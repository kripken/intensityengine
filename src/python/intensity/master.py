
"""
Handles communication with the master server
"""

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

import cgi, httplib, urllib, urlparse

from intensity.base import *
from intensity.logging import *
from intensity.auth import get_full_password

class MasterNetworkError(Exception):
    pass

def get_master_server():
    return get_config("Network", "master_server", "MISSING MASTER SERVER")

def get_master_protocol():
#    # Try https first (the other direction hangs, instead of giving an error, on problems
    _name = get_config('Network', 'master_server_protocol', 'http') # XXX: Switch to http by default, for speed, but see comment above
    if _name == 'https':
        return httplib.HTTPSConnection
    elif _name == 'http':
        return httplib.HTTPConnection
    else:
        assert(0) # 'Unknown master protocol: "%s"' % _name)

def set_master_protocol(protocol):
    if protocol == httplib.HTTPSConnection:
        _name = 'https'
    elif protocol == httplib.HTTPConnection:
        _name = 'http'
    else:
        assert(0) # 'Unknown master protocol (set): "%s"' % protocol

    set_config('Network', 'master_server_protocol', _name)

def get_redirect_url(response):
    return filter(lambda header: header[0] == 'location', response.getheaders())[0][1]

## @param request The request to send to the master server. No need for initial "/"
## @param params A dictionary of parameters to send
## @param session A MasterSession object containing our session info. We use this
##                to send the session info, which many/all requests need.
def contact_master(request, params={}, POST=False, accept_redirect=False):
    if get_config('Network', 'master_server', '') == '':
        log(logging.DEBUG, 'Request to master ignored, as have no master: %s' % request)
        return {}

    master_server = get_master_server()

    # Parse params into httplib format
    params_vec = []
    for key, value in params.iteritems():
        if value is None:
            value = ""
        params_vec.append( (key, value) )

    # Add session info, if any
    get_master_session().add_info(params_vec)

    log(logging.DEBUG, "Sending a request to the master server: %s \n params: %s \n (POST=%s)" % (request, str(params_vec), str(POST)))

    # Do the actual HTTP request
    try:
        kwargs = {}
        if sys.version[0:3] >= '2.6':
            kwargs['timeout'] = 15 # Do not lock up (problematic in fullscreen) forever
        conn = get_master_protocol()(master_server, **kwargs)
    except Exception, e:
        raise MasterNetworkError("Error in starting master server request: %s" % (str(e)))

    try:
        if not POST:
            conn.request( "GET", "/" + request + "?" + urllib.urlencode( params_vec ) )
        else:
            headers = {
                "Content-type": "application/x-www-form-urlencoded",
                "Accept": "text/plain"
            }
            conn.request( "POST", "/" + request, urllib.urlencode( params_vec ) )
    except Exception, e: # Network error of some sort
        if 'unknown protocol' in str(e) and get_master_protocol() is httplib.HTTPSConnection:
            set_master_protocol(httplib.HTTPConnection) # Try http
            return contact_master(request, params, POST)

        raise MasterNetworkError("ERROR in master server request (A) %s (param len: %d): %s" % (request, len(str(params)), str(e)))

    try:
        response = conn.getresponse()
    except httplib.BadStatusLine:
        raise MasterNetworkError("ERROR in master server request (C) %s (param len: %d): Bad status line" % (request, len(str(params))))
    except Exception, e:
        raise MasterNetworkError("ERROR in master server request (D): %s" % str(e))

    # Return redirect URL if we allow that
    if accept_redirect and response.status in [301, 302]:
        return get_redirect_url(response)

    if response.status != 200:
        log(logging.DEBUG, 'Master error content: %s / %s' % (str(response.getheaders()), response.read()))
        conn.close()
        raise MasterNetworkError("ERROR in master server request (B) %s (param len: %d): %d, %s" % (request, len(str(params)), response.status, response.reason))

    data = response.read()
    log(logging.DEBUG, "Raw response from master server: %s" % data)
    conn.close()

    ret = cgi.parse_qs(data)

    for key in ret.iterkeys():
        ret[key] = ret[key][0] # No need for unaesthetic arrays of size 1

    if 'error' in ret:
        raise MasterNetworkError("Master returned an error: " + ret['error'])

    log(logging.DEBUG, "Response from master server: %s" % (ret))

    return ret


## An authenticated session with the master. TODO: Use httpS for this
class MasterSession:
    def __init__(self):
        self.my_id = ''
        self.session_id = ''

    ## Connects to the master server and starts a session
    ## Clients: We give the master our username, and it gives us our user_id back
    ## (the username is nice and human-readable, but in the future may
    ## not be entirely unique). Servers: We give our instance_id
    def connect(self, identifier, raw_password):
        if Global.CLIENT:
            request_base = "user"
        else:
            request_base = "instance"

        class Output:
            pass

        current_op = None

        def make_side_operations(operation):
            def side_operations():
                try:
                    Output.response = operation()
                except MasterNetworkError, e:
                    log(logging.ERROR, "Error in contacting master: " + str(e))
                    if Global.CLIENT:
                        CModule.show_message("Error", str(e))
                    Output.response = None
                finally:
                    keep_aliver.quit()
            return side_operations

        # Run blocking HTTP in other thread, meanwhile do keepalive in this one, until finished

        def get_salt():
            # If we have '$' in the password, then it is already a fully-hashed and salted password, ready to be sent
            if '$' in raw_password:
                Output.hashed_password = raw_password
                return True

            ret = contact_master(
                request_base + "/startsession",
                {
                    'identifier': identifier,
                    'version': INTENSITY_VERSION_STRING,
                }
            )
            Output.hashed_password = get_full_password(ret['algorithm'], ret['salt'], raw_password)
            return True

        def send_credentials():
            return contact_master(
                request_base + "/startsession",
                {
                    'identifier': identifier,
                    'hashed_password': Output.hashed_password,
                    'version': INTENSITY_VERSION_STRING,
                }
            )

        phases = [(get_salt, 'Preparing to log in...'), (send_credentials, 'Logging in...')]
        for operation, message in phases:
            keep_aliver = KeepAliver(message)
            side_actionqueue.add_action(make_side_operations(operation))
            keep_aliver.wait()
            if Output.response is None:
                return (False,)

        if type(Output.response) is bool:
            log(logging.ERROR, "Error in login: Failed 2nd step")
            return (False,)

        try:
            self.set_info(Output.response['your_id'], Output.response['session_id'])
        except KeyError:
            log(logging.ERROR, "Error in contacting master: missing key in response: %s" % str(Output.response))
            return (False,)

        return (True, Output.hashed_password)

    ## Set the info from an external process - like an instance update, which for now is how instances 'login'
    def set_info(self, my_id, session_id):
        self.my_id = my_id
        self.session_id = session_id

        if Global.CLIENT:
            # The default transaction code is the id + the session id.
            CModule.set_transaction_code(self.my_id + ',' + self.session_id)

    def add_info(self, params_vec):
        if self.my_id == '': return

        if Global.CLIENT:
            params_vec.append( ('user_id', self.my_id) )
        else:
            params_vec.append( ('instance_id', self.my_id) )

        params_vec.append( ('session_id', self.session_id) )


## The singleton master session
master_session = MasterSession()

def get_master_session():
    global master_session
    return master_session

## Log into the master
def login_to_master(username, raw_password):
    return get_master_session().connect(username, raw_password)

## Use a known login (like from the browser window
def use_master_login(user_id, session_id):
    get_master_session().set_info(user_id, session_id)

# Prevent loops

from intensity.base import *
from intensity.safe_actionqueue import *

