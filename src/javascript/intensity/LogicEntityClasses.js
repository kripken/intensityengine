
/*
 *=============================================================================
 * Copyright (C) 2008 Alon Zakai ('Kripken') kripkensteiner@gmail.com
 *
 * This file is part of the Intensity Engine project,
 *    http://www.intensityengine.com
 *
 * The Intensity Engine is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * The Intensity Engine is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with the Intensity Engine.  If not, see
 *     http://www.gnu.org/licenses/
 *     http://www.gnu.org/licenses/agpl-3.0.html
 *=============================================================================
 */


//! Global registry of logic entity types. Relates class names (as strings) with actual scripting classes.
_logicEntityClasses = {};


//! Register a logic entity type, so that when we get the name later (as a string) we can create an
//! appropriate object of that class. Note that a class should only be registered after its parent class
//! has been registered.
//! @param _class The scripting class embodying that type, e.g., Character, Door, etc.
//! @param sauerType The Sauerbraten ent_type (in string format), which is created in the Sauerbraten system, e.g., 'mapmodel'. If there
//! is no relevant Sauerbraten type, this can be ommitted. It can also be ommitted if you want to use the same Sauerbraten type as the
//! parent class, which is by far the most likely case developers will encounter (this requires that the parent class has already
//! been registered, or some other ancestor).
function registerEntityClass(_class, sauerType) {
    var _className = _class.prototype._class; // The textual name of the type, e.g., "Door"

    log(DEBUG, "Registering LE class: " + _className);

    // Find sauerType in parent classes, if relevant:
    if (sauerType === undefined) {
        traverseAncestors(
            _class,
            function(parentClass) {
                var parentClassName = parentClass._class;
                log(DEBUG, "Finding sauertype in ancestor, " + parentClassName);

                try {
                    sauerType = getEntitySauerType(parentClassName);
                } catch (e) { // InvalidLogicEntityType
                    // Perhaps a higher ancestor will be found. We don't require that you register intermediary subclasses that
                    // are not actually used.
                }
            },
            function(parentClass) {
                return sauerType !== undefined || parentClass._class === 'LogicEntity'; // Stop after finding, or when nowhere else to go
            }
        );
    }

    // Store in registry
    eval(assert(' _logicEntityClasses[_className] === undefined && "Must not exist already! Ensure each class has a different _class" '));
    _logicEntityClasses[_className] = [_class, sauerType];

    // Generate protocol data

    var stateVariableNames = [];

    var inst = new _class()
    forEach(keys(inst), function(_name) {
        var variable = inst[_name];
        log(INFO, "Considering " + _name + " -- " + typeof variable);
        if (isVariable(variable)) {
            log(INFO, "Setting up " + _name);
            stateVariableNames.push(_name);
        }
    });

    log(DEBUG, "Generating protocol data for " + stateVariableNames);

    MessageSystem.generateProtocolData(_className, stateVariableNames);
}

//! Gets the actual scripting class based on the string of its name, for a registered logic entity subclass.
//! @param _class The name of the registered class. Can also be the class itself
function getEntityClass(_className) {
/*
    if not type(_class) is str:
        log(logging.ERROR, "Got a non-string in get_logic_entity_class, %s, trying to convert" %(str(_class)))//str(logicEntityClasses)))
        _class = _class.__name__
*/
    if (_logicEntityClasses[_className] !== undefined) {
        return _logicEntityClasses[_className][0];
    } else {
        log(ERROR, "Invalid class: " + _className);
        throw new IntensityError("Invalid class: " + _className);
    }
}

//! Gets the name of the Sauerbraten type based on the string of its name, for a registered logic entity subclass.
//! @param _class The name of the registered class.
function getEntitySauerType(_className) {
/*
    if not type(_class) is str:
        log(logging.ERROR, "Got a non-string in get_logic_entity_class, %s, trying to convert" %(str(_class)))//str(logicEntityClasses)))
        _class = _class.__name__
*/
    if (_logicEntityClasses[_className] !== undefined) {
        return _logicEntityClasses[_className][1];
    } else {
        log(ERROR, "Invalid class: " + _className);
        throw new IntensityError("Invalid class: " + _className);
    }
}

//! Returns a list of the names of all registered classes. For use in the GUI for creating entities.
function listEntityClasses() {
    return filter(function(_class) {
        return getEntityClass(_class).prototype._sauerType !== 'fpsent'; // These must be added using newNPC, in a server script
    }, keys(_logicEntityClasses)).sort();
}

