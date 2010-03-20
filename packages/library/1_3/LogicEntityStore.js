
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


// Caching by time delay

cacheByTimeDelay = function(func, delay) {
    func.lastTime = -delay*2;
    return function() {
        if (Global.time - func.lastTime >= delay) {
            func.lastCachedValue = func.apply(this, arguments);
            func.lastTime = Global.time;
        }
        return func.lastCachedValue;
    }
};


__entitiesStore = {}; //! Local store of entities, in Python. Parallels the C++ LogicData store, has same interface as server's persistence
__entitiesStoreByClass = {};

//! Same interface as the server's persistence system, but accesses just the local client's store of active LogicEntities.
//! @param uniqueId The unique id of the entity to be retrieved.
//! @return The logic entity corresponding to that unique id.
getEntity = function(uniqueId) {
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
getEntitiesByTag = function(withTag) {
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
getEntityByTag = function(withTag) {
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


//! Returns all the entities of a particular class (and subclasses)
//! @param _class The class to filter by (either the name of the class, or the actual class)
getEntitiesByClass = function(_class) {
    if (typeof _class === 'function') {
        _class = _class.prototype._class;
    }

    if (__entitiesStoreByClass[_class]) {
        return __entitiesStoreByClass[_class];
    } else {
        return [];
    }
}


//! Returns all the client entities, i.e., that are avatars of players
getClientEntities = function() {
    return getEntitiesByClass('Player');
}


//! Returns all the client numbers
getClientNumbers = function() {
    return map(function(client) { return client.clientNumber; }, getClientEntities());
}


isPlayerEditing = function(player) {
    if (Global.CLIENT) {
        player = defaultValue(player, getPlayerEntity());
    }
    return player && player.clientState === CLIENTSTATE.EDITING;
}


//! Returns the Logic Entities close to a particular entity (the entity itself is ignored).
//! @param origin The logic entity or position around which we look.
//! @param max_distance How far to look.
//! @param _class If given (default is None), then consider only LogicEntities that are instances of this class or its subclasses.
//! Useful for example to find all close-by doors, and not characters, etc.
//! @param with_tag If provided, then only entities having this tag will be taken into consideration.
//! @param unsorted By default we sort the output; this can disable that.
//! @return A list, from close to far, of tuples of the form (entity, distance)
getCloseEntities = function(origin, maxDistance, _class, withTag, unsorted) {
    var ret = [];

    var entities = _class ? getEntitiesByClass(_class) : values(__entitiesStore);
    for (var i = 0; i < entities.length; i++) {
        var otherEntity = entities[i];

        if ( withTag && !entity.hasTag(withTag) ) continue;
        if (!otherEntity.position) continue;

        var distance = origin.subNew(otherEntity.position).magnitude();

        if (distance <= maxDistance) {
            ret.push( [otherEntity, distance] );
        }
    }

    // Sort results by distance
    if (!unsorted) {
        ret.sort(function(a, b) { return (a[1] - b[1]); });
    }

    return ret;
}


addEntity = function(_className, uniqueId, kwargs, _new) {
//    if (_className.indexOf('Light') !== -1) return;
//    if (_className.indexOf('Flicker') !== -1) return;

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

    // Caching

    forEach(items(_logicEntityClasses), function(pair) {
        var className = pair[0];
        var classClass = pair[1][0];
        if (ret instanceof classClass) {
            if (!__entitiesStoreByClass[className]) {
                __entitiesStoreByClass[className] = [];
            }
            __entitiesStoreByClass[className].push(ret);
        }
    });

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
removeEntity = function(uniqueId) {
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

    // Caching

    var entity = __entitiesStore[uniqueId];
    forEach(items(_logicEntityClasses), function(pair) {
        var className = pair[0];
        var classClass = pair[1][0];
        if (entity instanceof classClass) {
            __entitiesStoreByClass[className].splice(
                findIdentical(__entitiesStoreByClass[className], entity),
                1
            );
        }
    });

    delete __entitiesStore[uniqueId];
}


//! Removes all entities from the store. This typically used when a map is removed from memory.
removeAllEntities = function() {
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
Global.lastmillis = 0; //<! Sauer-relevant value of lastmillis, useful for basetime of animations, etc.
Global.profiling = null; //!< To enable, place something like { interval: 1.0 } in this

Global.queuedActions = []; //!< Add actions here, that will be run the first time manageActions
                           //!< is called. That is only done *after* all entities are loaded
                           //!< and the scenario has started. So it is useful e.g. if you need
                           //!< to rely on a combination of entities to be present (like the
                           //!< GameManager)

manageActions = function(seconds, lastmillis) {
    log(INFO, "manageActions: queued");

    var currentActions = Global.queuedActions.slice(); // Work on copy as these may add more!
    Global.queuedActions = [];
    forEach(currentActions, function(action) { action(); });

    Global.time += seconds;
    Global.currTimeDelta = seconds;
    Global.lastmillis = lastmillis;

    log(INFO, "manageActions: " + seconds);

    if (Global.profiling) {
        if (!Global.profiling.counter) {
            Global.profiling.data = {};
            Global.profiling.counter = seconds;
        } else {
            Global.profiling.counter += seconds;
            if (Global.profiling.counter >= Global.profiling.interval) {
                Global.profiling.counter = 0; // Will reset data next time
            }
        }
    }
    var time;

    var entities = values(__entitiesStore);
    var i;
    for (i = 0; i < entities.length; i++) {
        var entity = entities[i];
//        log(INFO, "manageActions for: " + entity.uniqueId);
        if (entity.deactivated) {
            continue;
        }

        if (entity.shouldAct === false) {
            continue;
        }
        if (entity.shouldAct !== true && ((Global.CLIENT && !entity.shouldAct.client) || (Global.SERVER && !entity.shouldAct.server))) {
            continue;
        }

        if (Global.profiling) {
            time = CAPI.currTime();
        }

        if (Global.CLIENT) {
            entity.clientAct(seconds);
        } else {
            entity.act(seconds);
        }

        if (Global.profiling) {
            time = CAPI.currTime() - time;
            if (Global.profiling.data[entity._class] === undefined) Global.profiling.data[entity._class] = 0;
            Global.profiling.data[entity._class] += time;
        }
    }

    if (Global.profiling && Global.profiling.counter === 0) {
        log(ERROR, "---------------profiling (time per second)---------------");
        var sortedKeys = keys(Global.profiling.data);
        sortedKeys.sort(function(a, b) { return Global.profiling.data[b] - Global.profiling.data[a]; });
        forEach(sortedKeys, function(_class) {
            log(ERROR, "profiling: " + _class + ': ' + (Global.profiling.data[_class]/(1000*Global.profiling.interval)));
        });
        log(ERROR, "---------------profiling (time per second)---------------");
    }
}

// Replaces previous C++ system using Cube's octree
manageTriggeringCollisions = cacheByTimeDelay(function() {
    var time;
    if (Global.profiling && Global.profiling.data) {
        time = CAPI.currTime();
    }

    var entities = getEntitiesByClass('AreaTrigger');

    forEach(getClientEntities(), function(player) {
        if (isPlayerEditing(player)) return;

        var i;
        for (i = 0; i < entities.length; i++) {
            var entity = entities[i];

            if (World.isPlayerCollidingEntity(player, entity)) {
                if (Global.CLIENT) {
                    entity.clientOnCollision(player);
                } else {
                    entity.onCollision(player);
                }
            }
        }
    });

    if (Global.profiling && Global.profiling.data) {
        var _class = '__TriggeringCollisions__';
        time = CAPI.currTime() - time;
        if (Global.profiling.data[_class] === undefined) Global.profiling.data[_class] = 0;
        Global.profiling.data[_class] += time;
    }
}, defaultValue(Global.triggeringCollisionsDelay, 1/10)); // Important for performance, until we have a script octree

//! Perform dynamic rendering for all entities that need it. See renderDynamic
//! in LogicEntity. Should only be called on the client, of course.
//! @param thirdperson True is we are in thirdperson mode. In this case
//!                    the player entity should not be rendered
//!                    (HUD models should be drawn in renderHUDModels)
renderDynamic = function(thirdperson) {
    log(INFO, "renderDynamic");

    var time;

    var player = getPlayerEntity();
    if (!player) return;

    var entities = values(__entitiesStore);
    var i;
    for (i = 0; i < entities.length; i++) {
        var entity = entities[i];

        log(INFO, "renderDynamic for: " + entity.uniqueId);

        if (entity.deactivated || entity.renderDynamic === null) {
            continue;
        }

        if (Global.profiling && Global.profiling.data) {
            time = CAPI.currTime();
        }

        if (entity.useRenderDynamicTest) {
            if (!entity.renderDynamicTest) {
                Rendering.setupDynamicTest(entity);
            }

            if (!entity.renderDynamicTest()) continue;
        }

        entity.renderDynamic(false, !thirdperson && entity === player);

        if (Global.profiling && Global.profiling.data) {
            var _class = entity._class + '::renderDynamic';
            time = CAPI.currTime() - time;
            if (Global.profiling.data[_class] === undefined) Global.profiling.data[_class] = 0;
            Global.profiling.data[_class] += time;
        }
    };
}

renderHUDModels = function() {
    var player = getPlayerEntity();
    if (player.HUDModelName && player.clientState !== CLIENTSTATE.EDITING) {
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
    setPlayerUniqueId = function(uniqueId) {
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
    getPlayerEntity = function() {
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
    setStateDatum = function(uniqueId, keyProtocolId, value) {
        entity = getEntity(uniqueId);
        // The entity might not exist if this state datum update is due to an object not yet sent to us from the
        // server. We might in the future want TODO that the server only sends such updates when the client is
        // ready to accept them. Then 'None' here would be an error
        if (entity !== null) {
            var key = MessageSystem.fromProtocolId(entity._class, keyProtocolId);
            log(DEBUG, "setStateDatum: " + uniqueId + ' , ' + keyProtocolId + ' , ' + key);
            entity._setStateDatum(key, value);
        }
    }

    //! Checks whether the client has all necessary info to actually run the scenario, i.e. the current
    //! application. In particular, tests if all LogicEntities are initialized, and if the player logic entity
    //! has been created in completion.
    testScenarioStarted = function() {
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
    getNewUniqueId = function() {
        var ret = 0;
        forEach(keys(__entitiesStore), function(uniqueId) {
            ret = Math.max(ret, uniqueId);
        });
        ret = ret + 1
        log(DEBUG, "Generating new unique ID: " + ret);
        return ret;
    }

    newEntity = function(_className, kwargs, forceUniqueId, returnUniqueId) {
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

    newNPC = function(_className) {
        var npc = CAPI.addNPC(_className);
        npc._controlledHere = true;
        return npc;
    }

    //! Send a client all the data on the currently active entities. This include in-map entities (mapmodels etc.)
    //! and non-map (NPCs, non-Sauers, etc.)
    //! @param clientNumber The identifier of the client to which to send all data, or ALL_CLIENTS (-1) for all of them.
    sendEntities = function(clientNumber) {
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

        // Send in correct order, we need e.g. GameManager there first
        var ids = keys(__entitiesStore);
        ids.sort();
        for (var i = 0; i < ids.length; i++) {
            __entitiesStore[ids[i]].sendCompleteNotification(clientNumber);
        }
    }


    //! Sets a state datum for a logic entity, as a response to a client asking to do so. It
    //! translates protocol ids to normal names.
    //!
    //! @param uniqueId The unique id corresponding to the entity for which we will change the state datum.
    //! @param keyProtocolId The protocol_id of the state datum.
    //! @param value The value to set for that state datum.
    setStateDatum = function(uniqueId, keyProtocolId, value, actorUniqueId) {
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
    loadEntities = function(serializedEntities) {
        log(DEBUG, "Loading entities...: " + serializedEntities + typeof(serializedEntities));

        var entities = evalJSON(serializedEntities);

        forEach(entities, function(entity) {
            log(DEBUG, format("loadEntities: {0}", serializeJSON(entity)));
            var uniqueId = entity[0];
            var _class = entity[1];
            var stateData = entity[2];
            log(DEBUG, format("loadEntities: {0},{1},{2}", uniqueId, _class, stateData));

            if (_class === 'PlayerStart') _class = 'WorldMarker'; // backwards compatibility

            addEntity(_class, uniqueId, { 'stateData': serializeJSON(stateData) }); // TODO: See comment below on parsing speed
        });

        log(DEBUG, "Loading entities complete");
    }
}

//! Serializes the (persistent) entities and returns them in a form that can later be
//! read by loadEntities
saveEntities = function() {
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
            ret.push(serializeJSON([uniqueId, _class, stateData]));
        }
    });

    log(DEBUG, "Saving entities complete");

    return '[\n' + ret.join(',\n') + '\n]\n\n';
}


// Caching per Global.timestamp

cacheByGlobalTimestamp = function(func) {
    return function() {
        if (func.lastTimestamp !== Global.currTimestamp) {
            func.lastCachedValue = func.apply(this, arguments);
            func.lastTimestamp = Global.currTimestamp;
        }
        return func.lastCachedValue;
    }
};


CAPI.getTargetPosition = cacheByGlobalTimestamp(CAPI.getTargetPosition);
CAPI.getTargetEntity = cacheByGlobalTimestamp(CAPI.getTargetEntity);


Rendering = {
    setupDynamicTest: function(entity) {
        var currEntity = entity;
        entity.renderDynamicTest = cacheByTimeDelay(function() {
            var playerCenter = getPlayerEntity().center;
            if (currEntity.position.subNew(playerCenter).magnitude() > 256) {
                if (!hasLineOfSight(playerCenter, currEntity.position)) return false;
            }
            return true;
        }, 1/3);
    },
};
