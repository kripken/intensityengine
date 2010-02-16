
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


getOnModifyPrefix = function() {
    if (Global.CLIENT) {
        return "client_onModify_";
    } else {
        return "onModify_";
    }
};

//! A 'special variable', used in Logic Entities. We manage these in
//! a few important ways, see StateVariable for the main use case.
Variable = Class.extend({});

//! Tests if a variable is a state variable
isVariable = function(possible) {
    return (possible instanceof Variable);
}


//
// State Variables
//

//! The prefix used to hide state variables. This is needed since we use getters/
//! setters, for user scripting convenience. This makes the actual SV hidden.
//! It is hidden with this prefix so we can still access it (and its info).
_SV_PREFIX = "__SV_";


function __getVariable(uniqueId, variableName) {
    return getEntity(uniqueId)[_SV_PREFIX + variableName];
}

function __getVariableGuiName(uniqueId, variableName) {
    var entity = getEntity(uniqueId);
    var variable = entity[_SV_PREFIX + variableName];
    if (variable.guiName)
        return variable.guiName;
    else
        return variableName;
}


//! The basis for all state variables. StateVariables (SVs) are variables that (1) persist in the database, and (2) are client-server
//! managed. Thus, they reflect the 'state' of an entity, that is both persistent and synchronized between the clients and the server.
StateVariable = Variable.extend({
    create: function(kwargs) {
        log(INFO, "StateVariable.create");

        //! The name of this SV, i.e., the name of the attribute in the parent LogicEntity corresponding to it. Needs to be detected
        //! using detect_name.
        this._name = null;

        // Parse arguments
        if (kwargs === undefined) {
            kwargs = {};
        }

        //! If True (the default), then this State Variable can be read on the client. Otherwise, reading it triggers
        //! IntensityClientInvalidStateDataRead. If such a state variable is in addition unwritable on clients, then this
        //! state variable is effectively server-only. The main use for such a variable is that it can be persisted
        //! in the database with no extra work. If read is possible but not write, then this can be seen as a simple
        //! message passing system from server to clients.
        this.clientRead = defaultValue(kwargs.clientRead, true);

        //! If True (the default), then this State Variable can be written on the client (indirectly, through a message to the server; see
        //! ClientStateVariable). If False, then setting this variable on a client triggers an error. If such a state variable
        //! is in addition unreadable on clients, then this
        //! state variable is effectively server-only. The main use for such a variable is that it is persisted
        //! in the database with no extra work. If write is possible but not read, then this can be seen as a simple
        //! message passing system from clients to server.
        this.clientWrite = defaultValue(kwargs.clientWrite, true);

        //! If True, then a special system is used to synchronize values, and there is no need for the usual system of message passing
        //! to be used for that purpose. This is relevant e.g. for position changes in a Sauerbraten dynent, which have a special
        //! network protocol for fast updates. New user-defined attributes would in general *not* use custom methods like this.
        //! That is a customSynch attribute is, for example, something like 'position' which for dynents (but not extents!) is
        //! updated over the wire 30 times/second anyhow, so no need for our additional protocol.  But extent positions do need
        //! the normal protocol that we use, and not customSynch.
        //!
        //! A customSynch method does setting on the client. How this propagates elsewhere is custom-done.
        this.customSynch = defaultValue(kwargs.customSynch, false);

        //! A clientSet variable is one that the client can change directly. When a client script tries to change
        //! such a value, the change goes through immediately - no need to wait for a server response. The client sends
        //! notification to the server, which then updates all the OTHER clients (but not the original).
        //! Note that this doesn't check that the client is modifying his/her OWN variables. Client scripts are free to
        //! change clientSet variables of other logic entities, this will succeed, and has various uses.
        this.clientSet = defaultValue(kwargs.clientSet, false);

        //! The name to appear in the visual GUI for editing. In general this will be the same name as that of the
        //! variable itthis, that is the default. By setting guiName to something, that can be overridden with
        //! a perhaps longer and more this-explanatory one for the GUI.
        this.guiName = kwargs.guiName;

        //! An alternative name. Typically, AttributeAlias is used with this. The alternate name is another
        //! name that is just a reference for this one. In particular, when we set up _stateDataInformation,
        //! we do it for the altName as well, so that python_object.altName-type things will work.
        this.altName = kwargs.altName;

        //! Whether this message is reliable or not. Reliable messages are always received, even if they
        //! must be sent multiple times, and always in order - so you cannot send a great number of them
        //! without risk of stalling the system. Unreliable messages, on the other hand, also arrive
        //! in order, but not all of them necessarily arrive. They are useful for things like position
        //! updates. However, most 'game logic' stuff should probably be done using the default, which is
        //! reliable.
        this.reliable = defaultValue(kwargs.reliable, true);

        //! Most variables have 'history' in the sense that their value remains in place, and when
        //! e.g. new clients request it, they get the current value - which might have been set a
        //! long time ago. On the other hand, variables with hasHistory: false do not send their
        //! values except during actual updates. So when the value changes connected clients will
        //! see that, but in completeEntityUpdates for new clients nothing will be sent. This is useful
        //! for signalling events using a state variable as a network protocol.
        this.hasHistory = defaultValue(kwargs.hasHistory, true);

        //! clientPrivate variables are private to the owning client. That is, for player entities,
        //! the variable is only updated to the client whose avatar that is. Players cannot read such
        //! fields from other entities. The server, on the other hand, has access to everything.
        //! By default fields are not private - they are synched to all clients.
        //! Example uses: Inventory in an RPG, private messages, etc. etc.
        this.clientPrivate = defaultValue(kwargs.clientPrivate, false);
    },

    //! Internal method that lets a variable know it's name in the parent, and related
    //! setup like setting the getter/setter in the parent.
    _register: function(_name, parent) {
        this._name = _name;

        parent[_SV_PREFIX + _name] = this; // Because anyhow the getter/setters
        delete parent[_name];              // will make it invisible. We therefore rename it.

        // Should be done only for the class, not each instance, but simpler for now TODO fix
        // We partially apply the function, as the spec of these functions is
        //  getter(variable)        (and this is bound to the LogicEntity)
        //  setter(variable, value) (and this is bound to the LogicEntity)
        log(INFO, "Setting up setter/getter for " + _name);// + ": " + this.getter + ",,," + this.setter);
        eval(assert(" this.getter !== undefined "));
        eval(assert(" this.setter !== undefined "));
        parent.__defineGetter__(_name, partial(this.getter, this));
        parent.__defineSetter__(_name, partial(this.setter, this));

        // If there is an altName for this variable, give it the same state data information, so that it can be accessed
        // exactly like the real variable it is a reference for.
        if (this.altName) {
            parent[_SV_PREFIX + this.altName] = this;
            parent.__defineGetter__(this.altName, partial(this.getter, this));
            parent.__defineSetter__(this.altName, partial(this.setter, this));
        }
    },

    readTests: function(entity) {
//        if (entity.deactivated) {
//            eval(assert(" false /* entity.deactivated */ "));
//        }
        if (!(Global.SERVER || this.clientRead)) {
            eval(assert(" false /* Global.SERVER || this.clientRead */"));
        }
    },

    writeTests: function(entity) {
        if (entity.deactivated) {
            log(ERROR, "Trying to write to a field " + this._name + " of " + entity.uniqueId + "," + entity._class);
            eval(assert(" false /* entity.deactivated */ "));
        }
        if (!(Global.SERVER || this.clientWrite)) {
            eval(assert(" Global.SERVER || this.clientWrite "));
        }
        if (!entity.initialized) {
            eval(assert(" entity.initialized "));
        }
    },

    getter: function(variable) {
        log(INFO, "getter.readtests:" + variable.readTests(this));

        return this.stateVariableValues[variable._name];
    },

    //! This function is copied to the LogicEntity, where it will have 'this' as
    //! that object. 'variable' will be the StateVariable itself.
    setter: function(variable, value) {
//        log(INFO, "StateVariable::setter");

        variable.writeTests(this);

        this._setStateDatum(variable._name, value, null); // null - no origin, i.e., from a local script change
    },

    //! Overridden in child classes with relevant functionality
    validate: function(value) {
        return true;
    },

    //! Whether this variable, on entity 'entity', should be synched to client targetClientNumber.
    shouldSend: function(entity, targetClientNumber) {
        return !this.clientPrivate || entity.clientNumber === targetClientNumber;
    },
});

        // TODO: Cache values efficiently

//! Client version of State Variable (CSV). Setting such variables in fact sends a request to the server to set them, as
//! the server is the arbiter of their value. This is transparent to users of the SV. The server will then set the
//! value, if appropriate, which triggers a message back and the value on the client is then set, so the next read
//! will show the new value. Note that one consequence is that setting a value, then immediately reading it, will
//! give the *old* value as it has yet to be sent back by the server. We might TODO a setting where the variable is
//! 'unreadable' (gives errors) if read during this state. But it would require the server to respond even if the
//! the value is not set, which raises bandwidth.

//! Server version of StateVariable (SSV). Changing such values triggers (behind-the-scenes) messages to update all
//! clients of the change, i.e., their value is synchronized between the server and the clients.
//! TODO: Perhaps do a per-SSV setting of how fast to issue updates over the wire to the clients? Some might need
//! fast updating, others we might not want to swamp the tubes with them. Also how fast from clients to the server.

string = function(x) {
    return x.toString();
}

integer = function(x) {
    return Math.round(x);
}

//! A StateVariable that holds an integer value.
StateInteger = StateVariable.extend({
    toWire: string,
    fromWire: integer,
    toData: string,
    fromData: integer
});

// Utility to parse floats into 2 decimal places
decimal2 = function(x) {
    var ret = string(x);
    var i = ret.indexOf('.');
    if (i != -1) {
        return ret.slice(0, i+3);
    } else {
        return ret;
    }
}

//! A StateVariable that holds a float value. Can enforce non-negativity
StateFloat = StateVariable.extend({
    toWire: decimal2,
    fromWire: parseFloat,
    toData: decimal2,
    fromData: parseFloat
});

//! Enum state variable, a set of integer values, typically 0..N
StateEnum = StateInteger.extend({
});

//! Boolean StateVariable.
StateBoolean = StateVariable.extend({
    toWire: string,
    fromWire: bool,
    toData: string,
    fromData: bool
});

//! String StateVariable.
StateString = StateVariable.extend({
    toWire : string,
    fromWire : string,
    toData : string,
    fromData : string
});


MAX_STATE_ARRAY_SIZE = 100; // TODO: Generalize!

//! A 'surrogate' for StateArrays: This is returned when the array is
//! 'gotten', by e.g. arr = entity.theArray;
//! The surrogate can then be used like a normal array, and
//! it relays all operations to the parent.
//! To implement arr[16] and such, we manually create 1000(?) getters/
//! setters. This is unsightly, but works.
var ArraySurrogate = Class.extend({
    _class: "ArraySurrogate",

    create: function(entity, variable) {
        log(INFO, "Setting up ArraySurrogate");

        this.entity = entity;
        this.variable = variable;

        this.__defineGetter__("length", function __get_length() {
            log(INFO, "ArraySurrogate::length");
            return this.variable.getLength(this.entity);
        });

        /* Fails on Chrome, issue 242
           http://code.google.com/p/v8/issues/detail?id=242&colspec=ID%20Type%20Status%20Priority%20Owner%20Summary%20Stars
        var that = this; // Workaround js issue
        for (var i = 0; i < MAX_STATE_ARRAY_SIZE; i++) {
            (function() {
                var j = i; // Use function scoping to fix i as j.
                log(INFO, "Setting up getters/setters for ArraySurrogate:" + j);

                var success;

                success = that.__defineGetter__("0", function() {
                    log(INFO, "ArraySurrogate::getter(" + j + ")");
                    return that.variable.getItem(that.entity, j);
                });
//                eval(assert(' success' ));

                success = that.__defineSetter__("0", function(value) {
                    log(INFO, "ArraySurrogate::setter(" + j + ", " + value + ")");
                    that.variable.setItem(that.entity, j, value);
                });
//                eval(assert(' success' ));
            })();
        }
        */
    },

    push: function(value) {
        // Workarounds for Chrome issue 242, see link elsewhere
        //this[this.length] = value;
        this.set(this.length, value);
    },

    // Workarounds for Chrome issue 242, see link elsewhere

    get: function(index) {
        return this.variable.getItem(this.entity, index);
    },

    set: function(index, value) {
        this.variable.setItem(this.entity, index, value);
    },

    asArray: function() {
        log(INFO, "asArray:" + this);

        var ret = [];
        for (var i = 0; i < this.length; i++) {
            log(INFO, "asArray(" + i + ")");
            ret.push( this.get(i) );
        }
        return ret;
    }
});

//! Array StateVariable, a list of values. Uses "|" as an internal separator, so enforces that this is not present (TODO)
StateArray = StateVariable.extend({
    _class: "StateArray",

    separator: "|",

    //! The class whose instances are our surrogates. Overridden in some child classes
    surrogateClass: ArraySurrogate,

    //! The value to which we reset when filling with new values. Can be overridden
    emptyValue: function() { return []; },

    getter: function(variable) {
        log(INFO, "StateArray.getter " + variable.readTests(this));

        // If we have nothing to give, do not return an array of undefineds, return simply undefined
        if (variable.getRaw(this) == undefined) {
            return undefined;
        }

        log(INFO, "StateArray.getter (" + variable._name + "," + variable._class + "): Creating surrogate");

        var cacheName = '__arraysurrogate_' + variable._name;
        if (this[cacheName] === undefined) {
            this[cacheName] = new variable.surrogateClass(this, variable);
        }
        return this[cacheName];
    },

    setter: function(variable, value) {
        log(INFO, "StateArray.setter:" + serializeJSON(value));
        if (value.x) {
            log(INFO, "StateArray.setter:" + value.x + "," + value.y + "," + value.z);
        }
        if (value.get) {
            log(INFO, "StateArray.setter:" + value.get(0) + "," + value.get(1) + "," + value.get(2));
        }

        var data;

        if (value.asArray) {
            data = value.asArray(); // This object can make an array of itself
        } else {
            // Read items to create a new array
            data = [];
            var i;

            for (i = 0; i < value.length; i++) {
                var val;
                if (value instanceof ArraySurrogate) {
                    val = value.get(i);
                } else {
                    val = value[i];
                }
                data[i] = val;
            }
        }

        this._setStateDatum(variable._name, data, null);

/*
        // Set to [], erasing previous content (necessary), then we build from scratch
        log(DEBUG, "Setting empty value: " + serializeJSON(variable.emptyValue()));
        this.stateVariableValues[variable._name] = variable.emptyValue();

        for (i = 0; i < data.length; i++) {
            log(DEBUG, "StateArray.setter: " + i + " : " + val);
            variable.setItem(this, i, data[i]);
        }

        eval(assert(' variable.getLength(this) === data.length '));
*/

/*
        for (i = 0; i < data.length; i++) {
            log(DEBUG, i + ": " + variable.getItem(this, i) + " vs. " + data[i]);
            eval(assert(' variable.getItem(this, i) === data[i] '));
        }
*/
    },

    toWireItem: string,

    toWire: function(value) {
        log(INFO, "toWire of StateArray:" + serializeJSON(value));
        if (value.asArray !== undefined) {
            // This is an ArraySurrogate or other class that we can get a true array form from, using asArray()
            value = value.asArray();
        }

        return '[' + map(this.toWireItem, value).join(this.separator) + ']';
    },

    fromWireItem: string,

    fromWire: function(value) {
        log(DEBUG, "fromWire of StateArray:" + serializeJSON(value));
        if (value === "[]") {
            return [];
        } else {
            return map(this.fromWireItem, value.slice(1,-1).split(this.separator));
        }
    },

    toDataItem: string,

    toData: function(value) {
        log(INFO, "(1) StateArray.toData:" + value + typeof value + serializeJSON(value));

        if (value.asArray !== undefined) {
            log(INFO, "(1.5) StateArray.toData: using asArray");
            // This is an ArraySurrogate or other class that we can get a true array form from, using asArray()
            value = value.asArray();
        }

        log(INFO, "(2) StateArray.toData:" + value + typeof value + serializeJSON(value));

        return '[' + map(this.toDataItem, value).join(this.separator) + ']';
    },

    //! Converts an *item* of the array from its data (string) form
    fromDataItem: string,

    fromData: function(value) {
        log(DEBUG, "StateArray.fromData " + this._name + "::" + value);
        if (value === "[]") {
            return [];
        } else {
            return map(this.fromDataItem, value.slice(1,-1).split(this.separator));
        }
    },

    //! Gets a raw array from the data. By default this is the
    //! stateData, but it can be overridden in child classes.
    getRaw: function(entity) {
        log(INFO, "getRaw:" + this._class);
        log(INFO, serializeJSON(entity.stateVariableValues));
        var value = entity.stateVariableValues[this._name];

        if (value === undefined) {
            value = [];
        }
        return value;
    },

    setItem: function(entity, i, value) {
        log(INFO, "setItem: " + i + " : " + serializeJSON(value));
        var array = this.getRaw(entity);
        log(INFO, "gotraw: " + serializeJSON(array));
        if (typeof value === 'string') {
            eval(assert(' value.indexOf("|") === -1 '));
        }
        array[i] = value;
        entity._setStateDatum(this._name, array, null); //entity[this._name] = array; // Simulate an assignment, to trigger all the normal behaviour XXX FIXME WHAT?
    },

    getItem: function(entity, i) {
        log(INFO, "StateArray.getItem for " + i);
        var array = this.getRaw(entity);
        log(INFO, "StateArray.getItem " + serializeJSON(array) + " ==> " + array[i]);
        return array[i]; // TODO optimize all of this
    },

    getLength: function(entity) {
        var array = this.getRaw(entity);
        if (array === undefined) {
            eval(assert(' false /* array !== undefined ; getLength */'));
        }
        return array.length;
    }
});

StateArrayString = StateArray.extend({
    _class: "StateArrayString",
});

StateArrayStringComma = StateArrayString.extend({separator: ','});

/*
registerJSON("StateArray", function(val) { return val instanceof ArraySurrogate; }, function(val) {
    var ret = format("StateArraySurrogate: (len: {1}) [", val.length);
    for (var i = 0; i < val.length; i++) {
        ret += serializeJSON(val.get(i)) + ",";
    }
    ret += "]";
    return ret;
});
*/

//! An array of float values
StateArrayFloat = StateArray.extend({
    _class: "StateArrayFloat",

    toWireItem: decimal2,
    fromWireItem: parseFloat,

    toDataItem: decimal2,
    fromDataItem: parseFloat
});

StateArrayInteger = StateArray.extend({
    _class: "StateArrayInteger",

    toWireItem: string,
    fromWireItem: integer,

    toDataItem: string,
    fromDataItem: integer,
});

//! A generic wrapper around an actual Variable. In effect creates an alias for an attribute, giving
//! it a nicer name. Used to wrap around attributes such as WrappedCVariable, to
//! make things more convenient (see e.g. static_entity.py).
VariableAlias = Variable.extend({
    //! @param targetName the name of the Variable we are an alias for
    create: function(targetName) {
        this.targetName = targetName;
    },

    //! We register by simply using the getter/setter of the variable we
    //! are an alias for.
    _register: function(_name, parent) {
        this._name = _name;

        delete parent[_name]; // Because anyhow the getter/setters will make it invisible
        var target = parent[_SV_PREFIX + this.targetName];
        parent[_SV_PREFIX + _name] = target; // Point to the true Variable

        parent.__defineGetter__(_name, partial(target.getter, target));
        parent.__defineSetter__(_name, partial(target.setter, target));

        eval(assert("!this.altName")); // If this is ever used, need to implement it
    }
});

//! A simple class attribute that calls a function for getting the value. Those functions are CAPI utilities, in general.
//!
//! These are StateVariables - persisted in the database, and synchronized between client and server. In particular, this
//! means that you can change such a variable on either client or server and have the value propagated elsewhere.
//!
//! Note: We use 'getter' to get the value, this is done instead of from the state data - the C++ value is always right, we
//! read from there. State data mirrors the C++ value.
//!
//! The parent of a WrappedCVariable is assumed to have Signal capabilities,
//! as we use those to call the cSetter.
WrappedCVariable = {
    //! We save the cGetter/cSetter functions. These are CAPI calls
    //! that are done in addition to the normal SV actions.
    //! They both receive a UniqueID parameter.
    create: function(kwargs) {
        log(INFO, "WrappedCVariable.create");

        // Save as 'raw', because we parse it later, see 'late binding' below
        this.cGetterRaw = kwargs.cGetter;
        this.cSetterRaw = kwargs.cSetter;

        delete kwargs.cGetter;
        delete kwargs.cSetter;

        this._super(kwargs);
    },

    _register: function(_name, parent) {
        this._super(_name, parent);

        // Allow use of string names, for late binding at this stage. We copy the Raw values, then eval

        this.cGetter = this.cGetterRaw;
        this.cSetter = this.cSetterRaw;

        if (typeof this.cGetter === 'string') {
            this.cGetter = eval(this.cGetter);
        }

        if (typeof this.cSetter === 'string') {
            this.cSetter = eval(this.cSetter);
        }

//        log(INFO, "_register WrappedCVariable " + _name + ":" + this.cGetter + ",,," + this.cSetter);

        if (this.cSetter !== undefined) {
            // Subscribe to the modify event, so we always call the cSetter
            var prefix = getOnModifyPrefix();
            var variable = this;
            parent.connect(prefix + _name, function(value) {
                if (Global.CLIENT || parent.canCallCFuncs()) {
                    log(INFO, format("Calling cSetter for {0}, with: {1} ({2})", variable._name, value, typeof value));
                    // We have been set up, so apply the change
                    variable.cSetter(parent, value);

                    // Caching reads from script into C++ (search for "// Caching")
                    parent.stateVariableValues[variable._name] = value;
                    parent.stateVariableValueTimestamps[variable._name] = Global.currTimestamp;
                } else {
                    // We are not yet set up, so queue this change
                    parent._queueStateVariableChange(variable._name, value);
                }
            });
        } else {
            log(DEBUG, "No cSetter for " + _name + "; not connecting to signal");
        }
//        log(INFO, "_register WrappedCVariable done.");
    },

    getter: function(variable) {
        log(INFO, "WCV getter readtests " + variable.readTests(this));

        // Caching
        var cachedTimestamp = this.stateVariableValueTimestamps[variable._name];
        if (cachedTimestamp === Global.currTimestamp) {
            return this.stateVariableValues[variable._name];
        }

        log(INFO, "WCV getter " + variable._name);
        if (variable.cGetter !== undefined && (Global.CLIENT || this.canCallCFuncs())) {
            log(INFO, "WCV getter: call C");
            // We have a special getter, and can call it, so do so
            var value = variable.cGetter(this);

            // Caching
            if (Global.CLIENT || this._queuedStateVariableChangesComplete) {
                this.stateVariableValues[variable._name] = value;
                this.stateVariableValueTimestamps[variable._name] = Global.currTimestamp;
            }

            return value;
        } else {
            log(INFO, "WCV getter: fallback to stateData since " + variable.cGetter);
            return this._super(variable);
        }
    }
};

// WrappedC versions of SVs.
// We simulate multiple inheritance in a clumsy way here.

WrappedCInteger = StateInteger.extend(WrappedCVariable);

WrappedCFloat = StateFloat.extend(WrappedCVariable);

WrappedCEnum = StateEnum.extend(WrappedCVariable);

WrappedCBoolean = StateBoolean.extend(WrappedCVariable);

WrappedCString = StateString.extend(WrappedCVariable);

// Wrapped Vectors

WrappedCArray = StateArray.extend(
    merge( // Combine the two objects, with conflicts going to the last one
           // Again, a peculiar form of multiple inheritance
        WrappedCVariable, // XXX: Should add .prototype?
        {
            _class: "WrappedCArray",
            getter: new StateArray().getter, // We need the getter from StateArray, not WrappedCVariable

            getRaw: function(entity) {
                log(INFO, "WCA.getRaw " + this._name + this.cGetter);
                if (this.cGetter !== undefined && (Global.CLIENT || entity.canCallCFuncs())) {
                    // Caching
                    var cachedTimestamp = entity.stateVariableValueTimestamps[this._name];
                    if (cachedTimestamp === Global.currTimestamp) {
                        return entity.stateVariableValues[this._name];
                    }

                    log(INFO, "WCA.getRaw: call C");

                    // Caching
                    var value = this.cGetter(entity);
                    if (Global.CLIENT || entity._queuedStateVariableChangesComplete) {
                        entity.stateVariableValues[this._name] = value;
                        entity.stateVariableValueTimestamps[this._name] = Global.currTimestamp;
                    }
                    return value;
                } else {
                    log(INFO, "WCA.getRaw: fallback to stateData");
                    var ret = entity.stateVariableValues[this._name];
                    log(INFO, "WCA.getRaw..." + ret);
                    return ret;
                }
            }
        }
    )
);


var Vector3Surrogate = ArraySurrogate.extend(
    merge(
        Vector3.prototype,
        {
            _class: "Vector3Surrogate", // Debugging XXX

            create: function(entity, variable) {
                this._super(entity, variable);

                this.entity = entity;
                this.variable = variable;

                this.__defineGetter__("length", function() {
                    log(INFO, "Vector3Surrogate.length: always 3");
                    return 3;
                });

                /* Chrome issue 242
                var translations = ['x', 'y', 'z'];
                var that = this; // Workaround js issue
                for (var i = 0; i < 3; i++) {
                    (function() {
                        var j = i; // Use function scoping to fix i as j.
                        that.__defineGetter__(string(j), function() {
                            return that.variable.getItem(that.entity, j);
                        });
                        that.__defineSetter__(string(j), function(value) {
                            that.variable.setItem(that.entity, j, value);
                        });

                        // Also allow access using vec.x etc.
                        that.__defineGetter__(translations[j], function() {
                            return that.variable.getItem(that.entity, j);
                        });
                        that.__defineSetter__(translations[j], function(value) {
                            that.variable.setItem(that.entity, j, value);
                        });
                    })();
                }
                */

                this.__defineGetter__("x", function() {
                    return this.get(0);
                });
                this.__defineGetter__("y", function() {
                    return this.get(1);
                });
                this.__defineGetter__("z", function() {
                    return this.get(2);
                });

                this.__defineSetter__("x", function(value) {
                    this.set(0, value);
                });
                this.__defineSetter__("y", function(value) {
                    this.set(1, value);
                });
                this.__defineSetter__("z", function(value) {
                    this.set(2, value);
                });
            },

            // 'Remove' super's function of the same name
            push: function(value) {
                eval(assert(' false /* No such thing as .push() for Vector3Surrogate '));
            },
        }
    )
);

Vector3ArrayPlugin = {
    _class: 'Vector3ArrayPlugin',

    surrogateClass: Vector3Surrogate,

    // Our empty value is still a triple 
    emptyValue: function() { return [0,0,0]; },

    fromDataItem: parseFloat,
    toDataItem: decimal2,

    fromWireItem: parseFloat,
    toWireItem: decimal2,

/*    getItem: function(entity, i) {
        var array = this.getRaw(entity);
        log(INFO, "StateArray.getItem " + array + " ==> " + array[i]);
        return array[i]; // TODO optimize all of this
    }*/
};

WrappedCVector3 = WrappedCArray.extend(Vector3ArrayPlugin).extend({ _class: 'WrappedCVector3' });

StateVector3 = StateArray.extend(Vector3ArrayPlugin).extend({ _class: 'StateVector3' });

var Vector4Surrogate = ArraySurrogate.extend(
    merge(
        Vector4.prototype,
        {
            _class: "Vector4Surrogate", // Debugging XXX

            create: function(entity, variable) {
                this._super(entity, variable);

                this.entity = entity;
                this.variable = variable;

                this.__defineGetter__("length", function() {
                    log(INFO, "Vector4Surrogate.length: always 4");
                    return 4;
                });

                this.__defineGetter__("x", function() {
                    return this.get(0);
                });
                this.__defineGetter__("y", function() {
                    return this.get(1);
                });
                this.__defineGetter__("z", function() {
                    return this.get(2);
                });
                this.__defineGetter__("w", function() {
                    return this.get(3);
                });

                this.__defineSetter__("x", function(value) {
                    this.set(0, value);
                });
                this.__defineSetter__("y", function(value) {
                    this.set(1, value);
                });
                this.__defineSetter__("z", function(value) {
                    this.set(2, value);
                });
                this.__defineSetter__("w", function(value) {
                    this.set(3, value);
                });
            },

            // 'Remove' super's function of the same name
            push: function(value) {
                eval(assert(' false /* No such thing as .push() for Vector4Surrogate '));
            },
        }
    )
);

Vector4ArrayPlugin = merge(
    Vector3ArrayPlugin,
    {
        surrogateClass: Vector4Surrogate,

        // Our empty value is still a triple 
        emptyValue: function() { return [0,0,0,0]; },
    }
);

WrappedCVector4 = WrappedCArray.extend(Vector4ArrayPlugin);

StateVector4 = StateArray.extend(Vector4ArrayPlugin);

//! JSON StateVariable. Note: No surrogate, so will not notice changes to internals
StateJSON = StateVariable.extend({
    toWire: serializeJSON,
    fromWire: evalJSON,
    toData: serializeJSON,
    fromData: evalJSON,
});

// Serialize entities as their uniqueIDs
registerJSON("LogicEntity", function(val) { return val.uniqueId !== undefined; }, function(val) { return val.uniqueId; }, true);

