
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


//! Distributed entity system
//!
//! Yes, we are aware of "Note on Distributed Computing, A," Kendall, Waldo,
//! Wollrath and Wyant - hiding distribution isn't a universal solution. This
//! code here, however, appears to work well for its limited scope ;)


// Initialize classes with client or server content, as needed

eval(assert(" Global.CLIENT || Global.SERVER "));
eval(assert(" !(Global.CLIENT && Global.SERVER) "));
log(DEBUG, format("Generating LogicEntity system with CLIENT = {0}", Global.CLIENT));


//
// Logic Entities
//

RootLogicEntity = Class.extend({
    //! The string form of the class. Should be set by each new class
    _class: "LogicEntity",

    //! Whether this entity performs actions, i.e., whether its act/clientAct
    //! should be called each frame. This is true for most dynamic entities, but
    //! generally not true for static entities like lights.
    shouldAct: true,

    //! An array of tags for this entity. Tags are used for many things, from
    //! marking an entity defined in the 3D editor for easy fetching in scripting,
    //! to grouping of entities, to managing persistence.
    //!
    //! Persistence can be managed by tags, as a way to decide
    //! which entities are persistent and which are transient. For example, when
    //! editing a map the characters editing it are transient - we don't want them
    //! to be saved when we do a save of the map. But we do want entities like lights,
    //! particle effects, mapmodels, etc., to persist in such a way.
    //!
    //! A more complex example is where we have a 'template' for a map, like say
    //! a generic office room. We them customize it, and want only the new entities
    //! we created to persist, but not the 'template' entities - these are for us
    //! fixed, immutable - and not the editing characters - these are for us transient.
    //!
    //! Persistence tags are simply normal tags. Example tags might be 'transient',
    //! 'template', 'map', etc. etc. - completely user defined. Then, when the map
    //! entities are saved, we can specify which tags to save.
    tags: new StateArray(),

    //! Whether this entity should be exported when we do 'export entities'. A
    //! character would generally not be persistent, but static entities would.
    _persistent: new StateBoolean(),

    //! Operations performed on both client and server. This needs to occur either
    //! in the constructor or during activation.
    _generalSetup: function() {
        log(DEBUG, "RootLogicEntity._generalSetup");

        if (this._generalSetupComplete) {
            return;
        }

        //! We have signals
        Object.addSignalMethods(this);

        //! The action system that manages the entity's actions.
        this.actionSystem = new ActionSystem(this);

        //! The actual values of state variables
        this.stateVariableValues = {};

        // Caching reads from script into C++ (search for "// Caching")
        this.stateVariableValueTimestamps = {};

        //! Set to true once this entity is deactivated. At that point the entity will be removed from memory as
        //! soon as nothing else references it.
        this.deactivated = false;

        //! Set up variable-related things
        this._setupVariables();

        this._generalSetupComplete = true;
    },

    _generalDeactivate: function() {
        this.clearActions();

        CAPI.unregisterLogicEntity(this.uniqueId);

        this.deactivated = true;
    },

    _getStateDatum : function(key) {
        return this.stateVariableValues[key];
    },

    //! Called each frame.
    //! @param seconds How many seconds the frame lasts. For example, at 80 frames per second (fps),
    //!                seconds will be equal to 0.0125 (or close to it, because each frame can be
    //!                a little more or less).
    act: function(seconds) {
        this.actionSystem.manageActions(seconds);
    },

    //! Queues an action for this entity.
    queueAction: function(action) {
        this.actionSystem.queue(action);
    },

    //! Clears the action system of all actions.
    clearActions: function() {
        this.actionSystem.clear();
    },

    addTag: function(tag) {
        if (!this.hasTag(tag)) {
            this.tags.push(tag);
        }
    },

    removeTag: function(tag) {
        log(DEBUG, "remove");
        this.tags = filter(function(_tag) { return _tag !== tag; }, this.tags.asArray());
    },

    hasTag: function(tag) {
        log(INFO, "I can has " + tag + ", looking in: " + serializeJSON(this.tags));
        return (findIdentical(this.tags.asArray(), tag) >= 0);
    },

    //! Internal utility to set up C handlers, given their names
    _setupHandlers: function(handlerNames) {
        var prefix = onModifyPrefix();

        for (var handler in handler_names) {
            this.connect(prefix + handler, this["_set_" + handler]);
        }
    },

    //! Internal utility to set up variable-related stuff
    _setupVariables: function() {
//        log(DEBUG, "Setting up variables for " + this.uniqueId);

        // Go over all StateVariables and add getters and setters for them, copying them
        // from the appropriate StateVariable class
        var _names = keys(this); // We are adding more in the loop, so use a static copy

        forEach(iter(_names), function(_name) {
            var variable = this[_name];

//            log(DEBUG, "Considering " + _name + " -- " + typeof variable);

            if (isVariable(variable)) {

//                log(DEBUG, "Setting up " + _name);

                variable._register(_name, this);
            }
        }, this); // Need 'this' as 'self' due to js issue

//        log(DEBUG, "Setting up variables complete");
    },

    //! Serializes all state variables into their 'data' form, and returns
    //! a dictionary with them (that can then be serialized using e.g. JSON)
    //! Variables with hasHistory set to false are not included here.
    //! @param targetClientNumber If provided, the client who we will send to.
    //!                           Otherwise, ignore permissions and such.
    //! @param kwargs.compressed   If not, will not compress the variable
    //!                            names into protocol ids, which is useful
    //!                            for serializing them for storage.
    //!                            When compressed, we return a string - as we
    //!                            logically must. When uncompressed, an object.
    createStateDataDict: function(targetClientNumber, kwargs) {
        targetClientNumber = defaultValue(targetClientNumber, MessageSystem.ALL_CLIENTS);
        kwargs = defaultValue(kwargs, {});

        log(DEBUG, "createStateDataDict():" + this._class + this.uniqueId + ',' + targetClientNumber);

        var ret = {};

        forEach(iter(keys(this)), function(_name) {
            var variable = this[_name];
            if (isVariable(variable) && variable.hasHistory) {
                // Do not send private data
                if (targetClientNumber >= 0 && !variable.shouldSend(this, targetClientNumber)) return;

                var value = this[variable._name];
                if (value != undefined) {
                    log(INFO, "createStateDataDict() adding " + variable._name + ":" + serializeJSON(value));
                    ret[!kwargs.compressed ? variable._name : MessageSystem.toProtocolId(this._class, variable._name)] = variable.toData( value );
                    log(INFO, "createStateDataDict() currently...");
                    log(INFO, "createStateDataDict() currently: " + serializeJSON(ret));
                }
            }
        }, this);

        log(DEBUG, "createStateDataDict() returns: " + serializeJSON(ret));

        if (!kwargs.compressed) {
            return ret;
        }

        // Pre-compression - keep numbers as numbers, not strings

        forEach(keys(ret), function(key) {
            if (!isNaN(ret[key]) && ret[key] !== '') {
                ret[key] = parseFloat(ret[key]); // Keep 
            }
        });

        ret = serializeJSON(ret);

        log(DEBUG, "pre-compression: " + serializeJSON(ret));

        // Compression tricks - used only if they don't break things

        forEach([
            function(data) { return data.replace(/", "/g, '","'); },
            function(data) { return data.replace(/"(\w+)":/g, '$1:'); },
            function(data) { return data.replace(/:"(\d+)"/g, ':$1'); },
            function(data) { return data.replace(/"\[\]"/g, '[]'); },
            function(data) { return data.replace(/:"(\d+)\.(\d+)"/g, ':$1.$2'); },
            function(data) { return data.replace(/, /g, ','); },
        ], function(func) {
            var next = func(ret);
            if (next.length < ret.length && serializeJSON(evalJSON(next)) === serializeJSON(evalJSON(ret))) {
                ret = next;
            }
        });

        log(DEBUG, "compressed: " + ret);

        return ret.substr(1, ret.length-2); // Remove {, }
    },

    //! Updates the entire state data. In general, only used for initial initialization.
    //! Should not be needed by casual developers. Calls setStateDatum for each state data element (datum).
    //! @param stateData The actual state data, in network-appropriate form (needs to be parsed).
    _updateCompleteStateData: function(stateData) {
        log(DEBUG, format("updating complete state data for {0} with {1} ({2})", this.uniqueId, stateData, typeof stateData));

        if (stateData[0] !== '{') {
            stateData = '{' + stateData + '}';
        }

        var newStateData = evalJSON(stateData); // FIXME XXX: Use json.org version of this, which is safer

        eval(assert(' typeof newStateData === "object" '));

        // We are now initialized and can be used normally. This is done before the next loop because
        // we need 'get' accesses in the onModify's to succeed. IMPORTANT: onModify's may occur when not all variables
        // are yet set - so if this is critical, it should be checked for. TODO: Perhaps a 'fully_initialized' variable,
        // set after the next loop?
        this.initialized = true;

        // Set each datum separately, calling onModify's as necessary, etc.
        forEach(items(newStateData), function(item) {
            var key = item[0];
            var value = item[1];
            if (!isNaN(key)) {
                key = MessageSystem.fromProtocolId(this._class, key); // Numbers are protocol identifiers
            }

            log(DEBUG, format("update of complete state datum: {0} = {1}", key, value));
            this._setStateDatum(key, value, undefined, true); // true - this is an internal op, we are sending raw state data
            log(DEBUG, format("update of complete state datum ok"));
        }, this);

        log(DEBUG, "update of complete state data complete");
    }

});

//! Client version of Logic Entity (LE). At runtime, 'LogicEntity' on the client in fact points to this
//! class. Developers write Logic Entities without caring whether they are
//! derived from this or from a Server LogicEntity; the client-server system uses a single class to
//! manage both. That single class inherits from this or from the server version depending on whether
//! the running instance is a client or a server.
var ClientLogicEntity = RootLogicEntity.extend({

    //! Called when the entity is activated, on each client.
    clientActivate: function(kwargs) {
        this._generalSetup();

        if (!this._sauerType) {
            log(DEBUG, "non-Sauer entity going to be set up:" + this._class, + "," + this._sauerType);
            CAPI.setupNonSauer(this); // Does C++ registration, etc. Sauer types need special registration, which is done by them
        }

        //! Set to True once we receive a complete state data from the server. Only with the complete
        //! state data can we render and manage this entity appropriately.
        //! Once all LEs are initialized, the client can start to run the current map/world.
        this.initialized = false;
    },

    clientDeactivate: function() {
        this._generalDeactivate();
    },

/* XXX Old 'virtual world'-style system, with click scripts on each entity. For now we don't do this by
        default, but it can be of course done with a few lines of code. The current system calls performClick on
        Application (which can then do whatever we want).
    //! Called when the player clicks on the entity. This does the following: It calls
    //! client_click(), which can contain user-defined behavior on the client. If this does not return True (i.e.,
    //! if it returns False or anything else, or nothing), then the server is contacted, and runs click(). By
    //! default client_click does nothing, so the default behavior is to run the server's click() (somewhat like
    //! Second Life), but overriding client_click can allow responsive behaviors that occur immediately on the client,
    //! and can override (or not override) actions on the server.
    performClick: function(button) {
        if (!this.clientClick(button)) {
            MessageSystem.send(CAPI.DoClickEntity, button, this.uniqueId);
        }
    },

    //! Called when an entity is clicked. See perform_click() for more details about how this fits in the big picture.
    clientClick: function(button) {
    },
*/
    //! Set a state datum (i.e., a member of the state data).
    //! If done by order from the server, then we set directly, in blind obedience, and runs whatever client-side
    //! scripting is needed. Should never be used by casual developers. This function calls "client_onModify_X"
    //! functions which handle client effects of changing the value (showing animations, etc.).
    //! Note how we do not set the value using getattr/setattr, we set it directly, bypassing the StateVariable,
    //! because setting normally leads to the usual client-side behaviour (of sending a request to the server).
    //! We call _client_onModify_X first, before setting the value, so reading returns the old value. This is
    //! useful to see if the value rose and so forth.
    //! @param actorUniqueId The UID of the source of this change (FFU), or null if it was the
    //!                      client itself in a script. Currently any value except for null is
    //!                      considered as a server update.
    _setStateDatum : function(key, value, actorUniqueId) {
        log(DEBUG, format("Setting state datum: {0} = {1} for {2}", key, serializeJSON(value), this.uniqueId));

        var variable = this[_SV_PREFIX + key];

//        eval(assert(" !variable.customSynch "));
//log(DEBUG, "customSynch: " + variable.customSynch + " and cH:" + this._controlledHere);

        var customSynchFromHere = variable.customSynch && this._controlledHere;
        var clientSet = variable.clientSet;

        // If this originated from a client-side script over here, send a request, unless
        // this state variable is actually controlled here (and synched using some other method).
        // Or, if this is a variable that is set on the client, send a notification (not a request,
        // as we will be doing the update right after this).
        if ( actorUniqueId === null && !customSynchFromHere ) {
            log(DEBUG, "Sending request/notification to server");

            // TODO: Suppress message sending of the same value, at least for *some* SVs
            MessageSystem.send(variable.reliable ? CAPI.StateDataChangeRequest : CAPI.UnreliableStateDataChangeRequest,
                               this.uniqueId,
                               MessageSystem.toProtocolId(this._class, variable._name),
                               variable.toWire(value));
        }

        // If from server, or set on clientSide, do update now
        if ( actorUniqueId !== null || clientSet || customSynchFromHere ) {
            log(INFO, "Updating locally");
            // If this originated from the server, translate
            if (actorUniqueId !== null) {
                value = variable.fromWire(value);
            } 
            eval(assert(" variable.validate(value) "));
            this.emit( 'client_onModify_' + key, value, actorUniqueId !== null);
            this.stateVariableValues[key] = value;
        }
    },

    //! Performs actions. Overridden by LogicEntity types that have, e.g., default behaviours
    //! when the action list is empty. Note that client_act differs from act; act is run on the server, once
    //! (as there is only one server), while client_act is run on each client locally.
    //! @param seconds How many seconds the frame lasts. For example, at 80 frames per second (fps),
    //!                seconds will be equal to 0.0125 (or close to it, because each frame can be
    //!                a little more or less).
    clientAct: function(seconds) {
        log(INFO, "ClientLogicEntity.clientAct, " + this.uniqueId);
        this.actionSystem.manageActions(seconds);
    },

    //! If left at null, then the normal Sauer rendering pipeline is used. Otherwise,
    //! this is a function that is called during dynamic rendering (i.e., it will be
    //! called multiple times, for shadowing, etc. - like a dynent)
    renderDynamic: null
});


//! Server version of LogicEntity. At runtime, 'LogicEntity' on the server in fact points to this
//! class.
var ServerLogicEntity = RootLogicEntity.extend({
    //! Changed to true after sending the first complete notification. Until that point, there is no
    //! sense in sending out state variable updates, as the others haven't yet received the entity,
    //! and they will receive the full data at that later time anyhow
    sentCompleteNotification: false,

    //! Only called on creation, not on loading of an existent entity in the database.
    //! __activate__ is called for existing entities loaded from the database.
    init: function(uniqueId, kwargs) {
        log(DEBUG, "ServerLogicEntity.init(" + uniqueId + ", " + kwargs + ")");

        eval(assert(' uniqueId !== undefined '));
        eval(assert(' typeof uniqueId === "number" '));

        this.uniqueId = uniqueId;

        this._logicEntitySetup();

        //! Common shared variables
        this.tags = [];

        kwargs = defaultValue(kwargs, {});
        this._persistent = defaultValue(kwargs._persistent, false);
    },

    //! Called when the entity is registered and becomes 'active' in memory. Should perform necessary setup to enter the game,
    //! for example, send messages to clients informing them of the new entity, etc.
    //!
    //! Note that typically the overridder would call this parent *last*, since in particular this requires the state data,
    //! client number (if any), etc., to be ready to send. If this is an issue, then it is best to override this and
    //! not call the parent at all.
    activate: function(kwargs) {
        log(DEBUG, "ServerLogicEntity.activate(" + kwargs + ")");

        this._logicEntitySetup();

        if (!this._sauerType) {
            log(DEBUG, "non-Sauer entity going to be set up:" + this._class, + "," + this._sauerType);
            CAPI.setupNonSauer(this); // Does C++ registration, etc. Sauer types need special registration, which is done by them
        }

/*
            // Probably not needed, as only Sauer entities can queue/flush, but no harm; might be important in the future
            if hasattr(this, "_cpp_addr"):
                this._flushQueuedStateVariableChanges()
*/

        //! Apply the state data, if one was given
        if (kwargs !== undefined && kwargs.stateData !== undefined) {
            this._updateCompleteStateData(kwargs.stateData);
        }

        this.sendCompleteNotification(MessageSystem.ALL_CLIENTS);
        this.sentCompleteNotification = true;

        log(DEBUG, "LE.activate complete");
    },

    //! Override if the notification uses a different message type.
    sendCompleteNotification: function(clientNumber) {
        clientNumber = defaultValue(clientNumber, MessageSystem.ALL_CLIENTS);
        var clientNumbers = clientNumber === MessageSystem.ALL_CLIENTS ? getClientNumbers() : [clientNumber];

        log(DEBUG, format("LE.sendCompleteNotification: {1}, {2}", this.clientNumber, this.uniqueId));

        forEach(clientNumbers, function(currClientNumber) {
            MessageSystem.send(
                currClientNumber,
                CAPI.LogicEntityCompleteNotification,
                this.clientNumber ? this.clientNumber :  -1,
                this.uniqueId,
                this._class,
                this.createStateDataDict(currClientNumber, { compressed: true }) // Custom data per client
            );
        }, this);

        log(DEBUG, "LE.sendCompleteNotification complete");
    },

    _logicEntitySetup: function() {
        // This can be called by __init__ and __activate__. Should be done only once, in both cases, i.e., whether loaded
        // from database (only __activate__ is called) and whether just created from scratch (both __init__ and __activate__
        // are called). TODO: Just do this in activate? No, we need this done in __init__, as soon as possible
        if (!this.initialized) {
            log(DEBUG, "LE setup");

            this._generalSetup();

            //! See _queueStateVariableChange. As an object (/dict), we will not repeat the same
            //! value more than once - only the last one
            this._queuedStateVariableChanges = {};
            this._queuedStateVariableChangesComplete = false;

            //! Server GEs are always initialized (unlike client ones, which must be initialized).
            //! Still, we need this variable to exist (even though it always contains 'True')
            //! because the same script might run on both client and server,
            //! and for the client it needs to check for initialization. Also tells us we called this function (_logicEntitySetup)
            //! already.
            this.initialized = true;

            log(DEBUG, "LE setup complete");
        }
    },

    //! Should perform any cleanup necessary when deactivating this logic entity. In particular, to
    //! remove relevant memory structures, and notify other clients of the removal.
    deactivate: function() {
        this._generalDeactivate();

        // Notify all clients to remove this logic entity, it is not longer in action
        MessageSystem.send(MessageSystem.ALL_CLIENTS, CAPI.LogicEntityRemoval, this.uniqueId);
    },

    //! Called when an entity is clicked. See ClientLogicEntity::perform_click() for more details about how this fits in the big picture.
    click: function(button, clicker) {
    },

    //! Changes a state datum based on a network message, or a local direct change by a script on the server,
    //! in which case actorUniqueId is None.
    //!
    //! If the change is made (no user scripting, or user scripting said ok), then the value is set,
    //! and messages sent out to all clients, so that the remote copies of this SV are up to date.
    //! A notification is also send to all subscribers to the event "onModify_X". Once the messages arrive
    //! at the clients, the event "client_onModify_X" is called on each client.
    //!
    //! We call _onModify_X first, before setting the value, so reading returns the old value. This is
    //! useful to see if the value rose and so forth. If an onModify handler raises "CancelStateDataUpdate",
    //! the value will not be stored. Note however that other handlers may or may not be called in
    //! such a case - the result is unpredictable (some may have been called before this handler).
    //!
    //! @param key The name of the state variable
    //! @param value The value to set
    //! @param actorUniqueId The UID of the client that triggered this change, or null, if it was
    //!                      the server
    //! @param internalOp Whether this is an internal server operation. Such operations do NOT
    //!                   send messages, and give as their input not the wire format but the
    //!                   data format. An example of an internal operation is the server
    //!                   reading a state data from a dumpfile and applying it to an entity.
    _setStateDatum: function(key, value, actorUniqueId, internalOp) {
        log(INFO, format("Setting state datum: {0} = {1} ({2}) : {3}, {4}", key, value, typeof value, serializeJSON(value)), value._class);

        var _class = this._class;

        var variable = this[_SV_PREFIX + key];

        if (!variable) {
            log(WARNING, "Ignoring state data setting for unknown (possibly deprecated) variable: " + key);
            return;
        }

        // If this is over the wire, need to translate it, and check if it is even valid to get clients to do this
        if (actorUniqueId) {
            value = variable.fromWire(value); // Translate to correct type

            if (!variable.clientWrite) {
                log(ERROR, format("Client {0} tried to change {1}", actorUniqueId, key));
                return;
            }
        } else if (internalOp) {
            value = variable.fromData(value); // Translate to correct type
        }

        log(DEBUG, format("Translated value: {0} = {1} ({2}) : {3}, {4}", key, value, typeof value, serializeJSON(value)));
        log(DEBUG, "value._class: " + value._class);

/*
        if ( !variable.validate(value) ) {
            log(ERROR, format("Client {0} tried to change {1} to {2}", actorUniqueId, key, value));
            return;
        }
*/
        // Notify listeners for this event
        try {
            this.emit('onModify_' + key, value, actorUniqueId);
        } catch (e) {
            if (e === "CancelStateDataUpdate") {
                return; // Stop, without setting the value
            } else {
                throw e;
            }
        }

        this.stateVariableValues[key] = value;

        log(INFO, "New state data: " + this.stateVariableValues[key]);

        var customSynchFromHere = variable.customSynch && this._controlledHere;

        if ( (!internalOp) && variable.clientRead ) {
            if (!customSynchFromHere) {
                // Notify clients to change this value, if it has changed

//                // TODO: Activate this code in the future, to suppress message sending of the same value.
//                // But this is not for *all* variables, some do want such changes. Add suppress_duplicate_messages=False
//                // to state variables.
//                // Request server to change this value, if it has changed
//                try:
//                    old_value = getattr(this, key)
//                except:
//                    pass // If no old value, no problem
//                else:
//                    if value == old_value:
//                        return // If no change, do not bother sending an update

                if (!this.sentCompleteNotification) {
                    return; // No need to send individual updates until the entire entity is sent
                }

                var args = [
                    null,
                    variable.reliable ? CAPI.StateDataUpdate : CAPI.UnreliableStateDataUpdate,
                    this.uniqueId,
                    MessageSystem.toProtocolId(_class, key),
                    variable.toWire(value),
                    (variable.clientSet && actorUniqueId) ? getEntity(actorUniqueId).clientNumber : -1,
                ];

                forEach(getClientNumbers(), function(clientNumber) {
                    // Do not send private data
                    if (!variable.shouldSend(this, clientNumber)) return;

                    args[0] = clientNumber;
                    MessageSystem.send.apply(MessageSystem, args);
                }, this);
            }
        }
    },

    //! Internal utility. This is needed as in our __init__s we may set state variable data that cannot yet be
    //! copied to the C++ entity, as CAPI.setupXXXX has not yet been called - the C++ entity doesn't exist yet.
    //! We queue such things here, and flushes them out with _flush_queued_state_variable_changed.
    _queueStateVariableChange: function(key, value) {
        log(DEBUG, format("Queueing SV change: {0} - {1} ({2})", key, value, typeof value));

        this._queuedStateVariableChanges[key] = value;
    },

    //! Whether the C side is already set up, so we can call cGetter/cSetter on wrapped variables
    //! TODO: Might need such a thing for the client as well.
    canCallCFuncs: function() {
        return (this._queuedStateVariableChanges === null);
    },

    //! This is called after the CAPI.setupXXXX call. See _queueStateVariableChange.
    _flushQueuedStateVariableChanges: function() {
        log(DEBUG, "Flushing Queued SV changes for " + this.uniqueId);

        if (this.canCallCFuncs()) {
            return; // We have already been called
        }

        var changes = this._queuedStateVariableChanges;
        this._queuedStateVariableChanges = null; // Also a marker that we are done with this
        eval(assert(" this.canCallCFuncs() "));

        forEach(keys(changes), function(key) {
            var value = changes[key];

            var variable = this[_SV_PREFIX + key];

            log(DEBUG, format("(A) Flushing queued SV change: {0} - {1} (real: {2})", key, value, this.stateVariableValues[key]));
            this[key] = this.stateVariableValues[key];
            log(DEBUG, format("(B) Flushing of {0} - ok", key));
        }, this);

        this._queuedStateVariableChangesComplete = true;
    }
});

// This is the switch that sets it all in place, which allows the same class to be written for both client and server
if (Global.CLIENT) {
    LogicEntity = ClientLogicEntity;
} else {
    LogicEntity = ServerLogicEntity;
}

