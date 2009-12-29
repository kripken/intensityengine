//CModule.run_script("getEntity(58)._canEdit = true");

//=============================================================================
// Copyright (C) 2008 Alon Zakai ('Kripken') kripkensteiner@gmail.com
//
// This file is part of the Intensity Engine project,
//    http://www.intensityengine.com
//
// The Intensity Engine is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published by
// the Free Software Foundation, version 3.
//
// The Intensity Engine is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with the Intensity Engine.  If not, see
//     http://www.gnu.org/licenses/
//     http://www.gnu.org/licenses/agpl-3.0.html
//=============================================================================


//! Simple class to make sending messages in scripting nicer. In particular, no need to worry about client numbers,
//! and have the following syntax:
//!
//!  MessageSystem.send(MessageType, args)               for client->server,
//!
//!  MessageSystem.send(LogicEntity, MessageType, args)  for server->client (to the client with that LogicEntity, a player's LogicEntity),
//!  or
//!
//!  MessageSystem.send(clientNumber, MessageType, args) for server->client with explicit client number.
MessageSystem = {
    //! Send to this client number in order to send to *all* clients. This is the Sauerbraten convention.
    ALL_CLIENTS: -1,

    protocolNamesToIds: {},
    protocolIdsToNames: {},

    send: function() {
        log(DEBUG, "MessageSystem.send");

        var server, clientNumber;

        var args = Array.prototype.slice.call(arguments); // Transform arguments into true array

        if (args[0] instanceof LogicEntity) {
            // This is a server->client message, find client num from GE
            server = true;
            clientNumber = args[0].clientNumber;
        } else if (typeof args[0] == 'number') {
            // This is a server->client message, given an explicit clientnum
            server = true;
            clientNumber = args[0];
        } else {
            server = false;
        }

        if (server) {
            args = args.slice(1); // We are done with the first argument
        }

        messageType = args[0];
        args = args.slice(1);

        if (server) {
            args.unshift(clientNumber);
        }

        log(DEBUG, format("Scripting MessageSystem: Sending {0} with {1}\n", messageType, serializeJSON(args)));

        messageType.apply(null, args);
    },

    //! Generate protocol IDs for state variable names. For now, a simple enumeration, but in
    //! the future this might take into consideration how often each is used, using standard
    //! algorithms
    //! @param stateVariableNames A list of SV names, for which we generate protocol information.
    //! @return A dictionary, giving each name a protocol ID
    generateProtocolData: function(className, stateVariableNames) {
        log(DEBUG, "Generating protocol names for " + className);// + ", variables: " + stateVariableNames);

        stateVariableNames.sort(); // Ensure we have the same order on both client and server,
                                   // do not just assume we are running the same scripting engine
                                   // etc. and things will 'just work'

        namesToIds = {};
        idsToNames = {};
        for (var i = 0; i < stateVariableNames.length; i++) {
            namesToIds[stateVariableNames[i]] = i;
            idsToNames[i] = stateVariableNames[i];
        }

        this.protocolNamesToIds[className] = namesToIds;
        this.protocolIdsToNames[className] = idsToNames;

//        log(DEBUG, "MS this: " + serializeJSON(this));
    },

    toProtocolId: function(className, stateVariableName) {
        log(DEBUG, format("Retrieving protocol ID for {0}/{1}", className, stateVariableName));

        return this.protocolNamesToIds[className][stateVariableName];
    },

    fromProtocolId: function(className, protocol_id) {
        log(DEBUG, format("Retrieving state variable name for {0}/{1}", className, protocol_id));
//        log(DEBUG, "Retrieving: " + serializeJSON(this.protocolIdsToNames[className]));

        return this.protocolIdsToNames[className][protocol_id];
    },

    removeClassInfo: function(className) {
        delete this.protocolNamesToIds[className];
        delete this.protocolIdsToNames[className];
    },

    showClientMessage: function(clientNumber, title, text) {
        if (clientNumber instanceof LogicEntity) {
            clientNumber = clientNumber.clientNumber;
        }

        MessageSystem.send(clientNumber, CAPI.PersonalServerMessage, -1, title, text);
    },
}

