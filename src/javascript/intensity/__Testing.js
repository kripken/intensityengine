
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

// Simple testing framework. Allows testing of javascript modules
// as units, with fake CAPI/MessageSystem/etc. environments set up.

// Create environment

var saveCAPI, saveMessageSystem;

Testing = {};

Testing.replaceEnvironment = function() {
    __entitiesStore = {};

    // CAPI
    try {
        saveCAPI = CAPI;
    } catch (e) {
        saveCAPI = undefined;
    };

    CAPI = {
        feedback: {},
        StateDataChangeRequest: "CAPI.StateDataChangeRequest",
        LogicEntityCompleteNotification: "CAPI.LogicEntityCompleteNotification",
        LogicEntityRemoval: "CAPI.LogicEntityRemoval",
        StateDataUpdate: "CAPI.StateDataUpdate",
        currTime: function() {
            return 17001;
        },
        setupNonSauer: function(entity) {
            CAPI.feedback['setupNonSauer'] = entity.uniqueId;
        },
        setupCharacter: function(entity) {
            CAPI.feedback['setupCharacter'] = entity.uniqueId;
        },
        setupExtent: function(entity) {
            CAPI.feedback['setupExtent'] = entity.uniqueId;
        },
        unregisterLogicEntity: function(uniqueId) {
            CAPI.feedback.unregisterLogicEntity = uniqueId;
        },
        setAnimation: function(entity) {
            CAPI.feedback['setAnimation'] = entity.uniqueId;
        },
        setModelName: function(entity) {
            CAPI.feedback['setModelName'] = entity.uniqueId;
        },
        setAttachments: function(entity) {
            CAPI.feedback['setAttachments'] = entity.uniqueId;
        },
        setMaxSpeed: function(entity) {
            CAPI.feedback['setMaxSpeed'] = entity.uniqueId;
        },
        setYaw: function(entity) {
            CAPI.feedback['setYaw'] = entity.uniqueId;
        },
        setPitch: function(entity) {
            CAPI.feedback['setPitch'] = entity.uniqueId;
        },
        setMove: function(entity) {
            CAPI.feedback['setMove'] = entity.uniqueId;
        },
        setStrafe: function(entity) {
            CAPI.feedback['setStrafe'] = entity.uniqueId;
        },
        setDynentO: function(entity) {
            CAPI.feedback['setDynentO'] = entity.uniqueId;
        },
        setDynentVel: function(entity) {
            CAPI.feedback['setDynentVel'] = entity.uniqueId;
        },
        setRadius: function(entity) {
            CAPI.feedback['setRadius'] = entity.uniqueId;
        },
        setAboveeye: function(entity) {
            CAPI.feedback['setAboveeye'] = entity.uniqueId;
        },
        setEyeheight: function(entity) {
            CAPI.feedback['setEyeheight'] = entity.uniqueId;
        },
        setBlocked: function(entity) {
            CAPI.feedback['setBlocked'] = entity.uniqueId;
        },
        setJumping: function(entity) {
            CAPI.feedback['setJumping'] = entity.uniqueId;
        },
        setExtentO: function(entity) {
            CAPI.feedback['setExtentO'] = entity.uniqueId;
        },
        getExtentO: function(entity) {
            return [700,11,5];
        },
        getAttr1: function(entity) {
            return entity.uniqueId + 11;
        },
        getAttr2: function(entity) {
            return entity.uniqueId + 22;
        },
        getAttr3: function(entity) {
            return entity.uniqueId + 33;
        },
        getAttr4: function(entity) {
            return entity.uniqueId + 44;
        },
        setAttr1: function(entity) {
            CAPI.feedback['attr1'] == entity.uniqueId + 1;
        },
        setAttr2: function(entity) {
            CAPI.feedback['attr2'] == entity.uniqueId + 2;
        },
        setAttr3: function(entity) {
            CAPI.feedback['attr3'] == entity.uniqueId + 3;
        },
        setAttr4: function(entity) {
            CAPI.feedback['attr4'] == entity.uniqueId + 4;
        },
        setCollisionRadiusWidth: function(entity) {
        },
        setCollisionRadiusHeight: function(entity) {
        },
    };

    // MessageSystem
    try {
        saveMessageSystem = MessageSystem;
    } catch (e) {
        saveMessageSystem = undefined;
    };

    MessageSystem = {
        feedback: [],
        ALL_CLIENTS: 9,
        send: function(target, msg, param1, param2, param3) {
            var dump = format("MSG: {0} {1} {2},{3},{4}", target, msg, param1, param2, param3);
//          log(DEBUG, dump);
            this.feedback.push(dump);
        },
        toProtocolId: function(_class, key) {
            return _class + "|" + key;
        },
        generateProtocolData: function(_className, stateVariableNames) {
            this.feedback.push([_className, stateVariableNames]);
        }
    };
};

// Restore environment

Testing.restoreEnvironment = function () {
    if (saveCAPI) {
        CAPI = saveCAPI;
    }

    if (saveMessageSystem) {
        MessageSystem = saveMessageSystem;
    }

    __entitiesStore = {};
};

