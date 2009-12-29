
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



__entitiesStore = {}; //! Local store of entities, in Python. Parallels the C++ LogicData store, has same interface as server's persistence

//! Same interface as the server's persistence system, but accesses just the local client's store of active LogicEntities.
//! @param uniqueId The unique id of the entity to be retrieved.
//! @return The logic entity corresponding to that unique id.
function getEntity(uniqueId) {
    log(INFO, "getEntity" + uniqueId);
    var ret = __entitiesStore[uniqueId];
    if (ret !== undefined) {
        log(INFO, format("getEntity found entity {0} ({1})", uniqueId, ret.uniqueId));
        return ret;
    } else {
        log(INFO, format("getEntity could not find entity {0}", uniqueId));
        return null;
    }
}


//! @param withTag If given, only active entities with that tag are returned
//! @return All the currently active logic entities (i.e., registered LEs), currently in memory and running.
function getEntitiesByTag(withTag) {
    var ret = [];
    forEach(values(__entitiesStore), function(entity) {
        if (entity.hasTag(withTag)) {
            ret.push(entity);
        }
    });
    return ret;
}


//! A singleton version of getEntities: returns the single entity having a tag. If there are
//! no such entities, or more than one, None is returned.
function getEntityByTag(withTag) {
    var ret = getEntitiesByTag(withTag);
    if (ret.length == 1) {
        return ret[0];
    } else if (ret.length > 1) {
        log(WARNING, "Attempt to get a single entity with tag '" + withTag + "', but several exist");
        return null;
    } else {
        log(WARNING, "Attempt to get a single entity with tag '" + withTag + "', but no such entity exists");
        return null;
    }
}

//! Returns the Logic Entities close to a particular entity (the entity itself is ignored).
//! @param origin The logic entity or position around which we look.
//! @param max_distance How far to look.
//! @param _class If given (default is None), then consider only LogicEntities that are instances of this class or its subclasses.
//! Useful for example to find all close-by doors, and not characters, etc.
//! @param with_tag If provided, then only entities having this tag will be taken into consideration.
//! @param unsorted By default we sort the output; this can disable that.
//! @return A list, from close to far, of tuples of the form (entity, distance)
function getCloseEntities(origin, maxDistance, _class, withTag, unsorted) {
    var ret = [];

    forEach(values(__entitiesStore), function(otherEntity) {
        if ( _class && !(otherEntity instanceof _class) ) {
            return;
        }

        if ( withTag && !entity.hasTag(withTag) ) {
            return;
        }

        if (!otherEntity.position) return;

        var distance = origin.subNew(otherEntity.position).magnitude();

        if (distance <= maxDistance) {
            ret.push( [otherEntity, distance] );
        }
    });

    // Sort results by distance
    if (!unsorted) {
        ret.sort(function(a, b) { return (b[1] - a[1]); });
    }

    return ret;
}


function addEntity(_className, uniqueId, kwargs, _new) {
    uniqueId = defaultValue(uniqueId, 1331); // Useful for debugging

    log(DEBUG, format("Adding new Scripting LogicEntity of type {0} with unique ID {1}", _className, uniqueId));
    log(DEBUG, format("   with arguments: {0}, {1}", serializeJSON(kwargs), _new));

    eval(assert(' getEntity(uniqueId) === null ')); // Cannot re-create!

    var _class = getEntityClass(_className);

    var ret = new _class();

    if (Global.CLIENT) {
        ret.uniqueId = uniqueId;
    } else {
        // SERVER
        if (_new) {
            ret.init(uniqueId, kwargs);
        } else {
            ret.uniqueId = uniqueId;
        }
    }

    __entitiesStore[ret.uniqueId] = ret;
    eval(assert(' getEntity(uniqueId) ===  ret '));

    // Done after setting the unique ID and placing in the global store, because C++
    // registration relies on both.

    log(DEBUG, "Activating");

    if (Global.CLIENT) {
        ret.clientActivate(kwargs);
    } else {
        ret.activate(kwargs);
    }

    return ret;
}


//! Removes a logic entity from the local store. Called from LogicData::unregisterLogicEntity. Calls __nregister__
//! @param uniqueId The unique id of the entity to unregister
function removeEntity(uniqueId) {
    log(DEBUG, format("Removing Scripting LogicEntity: {0}", uniqueId));

    if (__entitiesStore[uniqueId] === undefined) {
        log(WARNING, "Cannot remove entity " + uniqueId + " as it does not exist");
        return;
    }

    __entitiesStore[uniqueId].emit('preDeactivate');

    if (Global.CLIENT) {
        __entitiesStore[uniqueId].clientDeactivate();
    } else {
        __entitiesStore[uniqueId].deactivate();
    }

    delete __entitiesStore[uniqueId];
}


//! Removes all entities from the store. This typically used when a map is removed from memory.
function removeAllEntities() {
    forEach(keys(__entitiesStore), function(uniqueId) {
        removeEntity(uniqueId);
    });
}

//! This changes every time a new frame is started. It can
//! be used in scripts to know if they are running in the same frame as some
//! previous point in time at which they made a note to themselves of the
//! timestamp. This is useful to not repeat calculations that do not change
currTimestamp = 0;
Global.currTimestamp = currTimestamp;

startFrame = function() {
    currTimestamp += 1; // backwards compatibility
    Global.currTimestamp = currTimestamp;
}

Global.time = 0; //!< Total time passed
Global.currTimeDelta = 1.0; //!< Current frame time. Initialized to 1.0 just to give a valid value if anyone reads it.

manageActions = function(seconds) {
    Global.time += seconds;
    Global.currTimeDelta = seconds;

    log(INFO, "manageActions: " + seconds);

    forEach(values(__entitiesStore), function(entity) {
//        log(INFO, "manageActions for: " + entity.uniqueId);
        if (entity.deactivated) {
            return;
        }

        if (!entity.shouldAct) {
            return;
        }

        if (Global.CLIENT) {
            entity.clientAct(seconds);
        } else {
            entity.act(seconds);
        }
    });
}

//! Perform dynamic rendering for all entities that need it. See renderDynamic
//! in LogicEntity. Should only be called on the client, of course.
//! @param thirdperson True is we are in thirdperson mode. In this case
//!                    the player entity should not be rendered
//!                    (HUD models should be drawn in renderHUDModels)
function renderDynamic(thirdperson) {
    log(INFO, "renderDynamic");

    forEach(values(__entitiesStore), function(entity) {
        log(INFO, "renderDynamic for: " + entity.uniqueId);

        if (entity.deactivated || entity.renderDynamic === null) {
            return;
        }

        entity.renderDynamic(false, !thirdperson && entity === getPlayerEntity());
    });
}

function renderHUDModels() {
    var player = getPlayerEntity();
    if (player.HUDModelName) {
        player.renderDynamic(true, true);
    }
}


/*
function deactivateAllEntities() {
    log(DEBUG, "Deactivating all logic entities from Scripting");

    uniqueIds = keys(__entitiesStore);

    forEach(uniqueIds, function(uniqueId) {
        deactivateEntity(uniqueId);
    });
}
*/

// Client stuff

if (Global.CLIENT) {
    //! The player's logic entity
    playerLogicEntity = null;

    //! Sets the unique ID of the player (reflects ClientSystem::uniqueId). Creates the playerLogicEntity global
    //! which can then be accessed by get_playerLogicEntity().
    //! @param uniqueId The unique id of the player's LogicEntity.
    function setPlayerUniqueId(uniqueId) {
        log(DEBUG, format("Setting player unique ID to {0}", uniqueId));

        if (uniqueId !== null) {
            playerLogicEntity = getEntity(uniqueId);
            playerLogicEntity._controlledHere = true;

            eval(assert(' uniqueId === null || playerLogicEntity !== null '));
        } else {
            playerLogicEntity = null;
        }
    }

    //! @return The player logic entity, i.e., the logic entity of the character the player controls.
    function getPlayerEntity() {
        return playerLogicEntity;
    }

    //! Sets a state datum for a logic entity, as a response to a server update telling us to do so. It
    //! translates protocol ids to normal names.
    //! This does *not* do the normal operation of sending a request to the server for a real change, it just
    //! performs a local change. Therefore it only makes sense as a response to a server command.
    //!
    //! Note that this function parallels the persistence.py function of the same name. They
    //! will probably diverge, however.
    //!
    //! @param uniqueId The unique id corresponding to the entity for which we will change the state datum.
    //! @param keyProtocolId The protocol_id of the state datum.
    //! @param value The value to set for that state datum.
    function setStateDatum(uniqueId, uniqueId, value) {
        entity = getEntity(uniqueId);
        // The entity might not exist if this state datum update is due to an object not yet sent to us from the
        // server. We might in the future want TODO that the server only sends such updates when the client is
        // ready to accept them. Then 'None' here would be an error
        if (entity !== null) {
            var key = MessageSystem.fromProtocolId(entity._class, uniqueId);
            entity._setStateDatum(key, value);
        }
    }

    //! Checks whether the client has all necessary info to actually run the scenario, i.e. the current
    //! application. In particular, tests if all LogicEntities are initialized, and if the player logic entity
    //! has been created in completion.
    function testScenarioStarted() {
        log(INFO, "Testing whether the scenario started");

        if (getPlayerEntity() === null) {
            log(INFO, "...no, player logic entity not created yet");
            return false;
        }

        log(INFO, "...player entity created");

        if (playerLogicEntity.uniqueId < 0) {
            log(INFO, "...no, player not unique ID-ed");
            return false;
        }

        log(INFO, "...player entity unique ID-ed");

        forEach(values(__entitiesStore), function(entity) {
            if (!entity.initialized) {
                log(INFO, format("...no, {0} is not initialized", entity.uniqueId));
                return false;
            }
        });

        log(INFO, "...yes");
        return true;
    }
}

// Server stuff

if (Global.SERVER) {

    //! Returns a new UniqueID that isn't used by anything. This UiD is *NOT* reserved, it remains
    //! valid only as long as no other entity has been created (which, using this same function,
    //! might well want the same UiD)
    function getNewUniqueId() {
        var ret = 0;
        forEach(keys(__entitiesStore), function(uniqueId) {
            ret = Math.max(ret, uniqueId);
        });
        ret = ret + 1
        log(DEBUG, "Generating new unique ID: " + ret);
        return ret;
    }

    function newEntity(_className, kwargs, forceUniqueId, returnUniqueId) {
        log(DEBUG, "New logic entity: " + forceUniqueId);

        if (forceUniqueId === undefined) {
            forceUniqueId = getNewUniqueId();
        }

        var ret = addEntity(_className, forceUniqueId, kwargs, true);

        if (returnUniqueId) { // A convenience
            return ret.uniqueId;
        } else {
            return ret;
        }
    }

    function newNPC(_className) {
        var npc = CAPI.addNPC(_className);
        npc._controlledHere = true;
        return npc;
    }

    //! Send a client all the data on the currently active entities. This include in-map entities (mapmodels etc.)
    //! and non-map (NPCs, non-Sauers, etc.)
    //! @param clientNumber The identifier of the client to which to send all data, or ALL_CLIENTS (-1) for all of them.
    function sendEntities(clientNumber) {
        log(DEBUG, "Sending active logic entities to " + clientNumber);

        var numEntities = 0; // TODO: Better JS-ey way to do this?
        for (var item in __entitiesStore) {
            numEntities++;
        };

        MessageSystem.send(
            clientNumber,
            CAPI.NotifyNumEntities,
            numEntities
        );

        forEach(values(__entitiesStore), function(entity) {
            entity.sendCompleteNotification(clientNumber);
        });
    }


    //! Sets a state datum for a logic entity, as a response to a client asking to do so. It
    //! translates protocol ids to normal names.
    //!
    //! @param uniqueId The unique id corresponding to the entity for which we will change the state datum.
    //! @param keyProtocolId The protocol_id of the state datum.
    //! @param value The value to set for that state datum.
    function setStateDatum(uniqueId, keyProtocolId, value, actorUniqueId) {
        var entity = getEntity(uniqueId);
        // The entity might not exist if this state datum update is due to an object meanwhile destroyed, etc.
        if (entity !== null) {
            var key = MessageSystem.fromProtocolId(entity._class, keyProtocolId);
            entity._setStateDatum(key, value, actorUniqueId);
        }
    }


    //! Loads the entities for a map into memory. Entities - at least static ones - were held in
    //! the Sauerbraten .ogz file, but we no longer do that. Instead, we read all the entities
    //! in the database that correspond to that map. This in particular lets us handle static and
    //! dynamic entities in the same manner.
    function loadEntities(serializedEntities) {
        log(DEBUG, "Loading entities...: " + serializedEntities + typeof(serializedEntities));

        var entities = evalJSON(serializedEntities);

        forEach(entities, function(entity) {
            log(DEBUG, format("loadEntities: {0}", serializeJSON(entity)));
            var uniqueId = entity[0];
            var _class = entity[1];
            var stateData = entity[2];
            log(DEBUG, format("loadEntities: {0},{1},{2}", uniqueId, _class, stateData));

            addEntity(_class, uniqueId, { 'stateData': serializeJSON(stateData) }); // TODO: See comment below on parsing speed
        });

        log(DEBUG, "Loading entities complete");
    }

    //! Serializes the (persistent) entities and returns them in a form that can later be
    //! read by loadEntities
    function saveEntities() {
        var ret = [];

        log(DEBUG, "Saving entities...:");

        forEach(values(__entitiesStore), function(entity) {
            if (entity._persistent) {
                log(DEBUG, "Saving entity " + entity.uniqueId);
                var uniqueId = entity.uniqueId;
                var _class = entity._class;
                var stateData = entity.createStateDataDict();
                                                  // TODO: Also, store as serialized here, not as dict, to save all the
                                                  // parse-unparsing that makes things slow
                ret.push([uniqueId, _class, stateData]);
            }
        });

        log(DEBUG, "Saving entities complete");

        return serializeJSON(ret);
    }

}

