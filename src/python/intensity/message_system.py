"""
Messaging functionality.

1. Some higher-level protocol management for StateVariables.
2. An interface to our ENet-based messaging system, with convenient Python notation.
We also provide some nice wrappers around common messages
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


from logging import *


## Send to this client number in order to send to *all* clients. This is the Sauerbraten convention.
ALL_CLIENTS = -1

## Simple class to make sending messages in Python nicer. In particular, no need to worry about client numbers,
## and have the following syntax:
##
##  MessageSystem.send(MessageType, args)               for client->server,
##
##  MessageSystem.send(LogicEntity, MessageType, args)  for server->client (to the client with that LogicEntity, a player's LogicEntity),
##  or
##
##  MessageSystem.send(clientNumber, MessageType, args) for server->client with explicit client number.
class MessageSystem:
    protocol_names_to_ids = {}
    protocol_ids_to_names = {}

    @classmethod
    def send(self, *args):
        if hasattr(args[0], "client_number"):
            # This is a server->client message, find client num from GE
            server = True
            client_number = args[0].client_number
        elif type(args[0]) == int:
            # This is a server->client message, given an explicit clientnum
            server = True
            client_number = args[0]
        else:
            server = False

        if server:
            args = args[1:] # We are done with the first argument

        message_type = args[0]
        args = args[1:]

        log(logging.DEBUG, "Python MessageSystem: Sending %s with %s\n" % (str(message_type.__name__), str(args)))

        if server:
            message_type(client_number, *args)
        else:
            message_type(*args)

    ## Generate protocol IDs for state variable names. For now, a simple enumeration, but in
    ## the future this might take into consideration how often each is used, using standard
    ## algorithms
    ## @param state_variable_names A list of SV names, for which we generate protocol information.
    ## @return A dictionary, giving each name a protocol ID
    @classmethod
    def generate_protocol_data(self, class_name, state_variable_names):
        state_variable_names.sort() # Ensure we have the same order on both client and server,
                                    # do not just assume we are running the same Python version
                                    # etc. and things will 'just work'

        names_to_ids = {}
        ids_to_names = {}
        for i in range(len(state_variable_names)):
            names_to_ids[state_variable_names[i]] = i
            ids_to_names[i] = state_variable_names[i]

        self.protocol_names_to_ids[class_name] = names_to_ids
        self.protocol_ids_to_names[class_name] = ids_to_names

    @classmethod
    def to_protocol_id(self, class_name, state_variable_name):
        return self.protocol_names_to_ids[class_name][state_variable_name]

    @classmethod
    def from_protocol_id(self, class_name, protocol_id):
        return self.protocol_ids_to_names[class_name][protocol_id]


#

def show_client_message(client, title, text):
    MessageSystem.send(client, CModule.PersonalServerMessage, -1, title, text )

