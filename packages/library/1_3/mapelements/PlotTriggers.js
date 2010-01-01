
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


Library.include('library/' + Global.LIBRARY_VERSION + '/mapelements/EditingTools.js');


StatePlugin = {
    state: new StateString(),
    states: new StateArrayStringComma(),
    stateModelNames: new StateArrayStringComma(),
    stateAnimations: new StateArrayStringComma(),

    init: function() {
        this.states = ['closed', 'open'];
        this.stateModelNames = ['tree', 'areatrigger'];
        this.stateAnimations = ['ANIM_IDLE|ANIM_LOOP', 'ANIM_TRIGGER']
    },

    activate: function() {
        this.connect('onModify_stateModelNames', function(stateModelNames) {
            this.setState(this.state, stateModelNames);
        });
        this.connect('onModify_stateAnimations', function(stateAnimations) {
            this.setState(this.state, undefined, stateAnimations);
        });
    },

    setState: function(state, stateModelNames, stateAnimations) {
        stateModelNames = defaultValue(stateModelNames, this.stateModelNames.asArray());
        stateAnimations = defaultValue(stateAnimations, this.stateAnimations.asArray());
        for (var i = 0; i < this.states.length; i++) {
            if (this.states.get(i) === state) {
                this.state = state; // Note that this is not a state var. Add one if you want synching
                var modelName = stateModelNames[i] ? stateModelNames[i] : 'areatrigger';
                if (this.visualModelName !== undefined) {
                    this.visualModelName = modelName;
                } else {
                    this.modelName = modelName;
                }
                try {
                    this.animation = eval(stateAnimations[i]);
                } catch (e) {
                    MessageSystem.showClientMessage(MessageSystem.ALL_CLIENTS, 'Error in updating animation with "' + stateAnimations[i] + '"');
                    this.animation = ANIM_IDLE|ANIM_LOOP;
                }
                break;
            }
        }
    },
};

//! Plot triggers are areas in the map that must be passed in order to
//! continue in the plot. For example, buttons you must press in order for
//! the door to the final boss to open.
PlotTrigger = registerEntityClass(bakePlugins(ResettableAreaTrigger, [ChildEntityPlugin, StatePlugin, {
    _class: "PlotTrigger",
    parentEntityClass: 'PlotBarrier',
    debugDisplay: {
        color: 0xFF55FF,
        radius: 1.5,
    },
    visualModelName: new StateString(),

    init: function() {
        this.stateModelNames = ['', ''];
        this.visualModelName = '';
    },

    activate: function() {
        this.setState('closed');

        this.connect('onModify_state', function(state) {
            // Allow editing changes to take effect
            if (state !== this.state && state !== this.oldState) {
                this.oldState = state; // Prevent infinite recursion
                this.setState(state);
            }

            if (state === 'open') {
                var parent = getEntity(this.parentEntity);
                if (parent) {
                    GameManager.getSingleton().eventManager.add({
                        secondsBefore: 0,
                        func: function() {
                            parent.refreshBarrier();
                        },
                        entity: parent,
                    });
                }
            } else {
                this.readyToTrigger = true;
            }
        });
    },

    onTrigger: function(collider) {
        this.setState('open');
    },

    renderDynamic: function() {
        if (!this.visualModelName || this.visualModelName === 'areatrigger') return;

        var o = this.position
        var anim = this.animation;
        var flags = MODEL.LIGHT | MODEL.CULL_VFC | MODEL.CULL_DIST | MODEL.CULL_OCCLUDED | MODEL.CULL_QUERY;
        var args = [GameManager.getSingleton(), this.visualModelName, anim, o.x, o.y, o.z + this.collisionRadiusHeight, this.yaw, 0, 0, flags, this.startTime];
        GameManager.getSingleton().renderingHashHint = this.uniqueId;
        CAPI.renderModel2.apply(null, args);
    },

    createStateDataDict: function() {
        var ret = this._super.apply(this, arguments);

        // On the client, we are saving entities...
        if (Global.CLIENT) {
            ret['state'] = 'closed';
        }

        return ret;
    },
}]));


//! A plot trigger that when triggered lets the player pick up an item. The item must be
//! taken to unlock the barrier (as opposed to normal PlotTriggers, which open the barrier
//! upon triggering
PlotTriggerItem = registerEntityClass(bakePlugins(PlotTrigger, [{
    _class: "PlotTriggerItem",

    HUDIcon: new StateString(), //! What to show on the HUD when carried
    carryingPlayer: new StateInteger(), //!< When carried, the player carrying
    droppedPosition: new StateVector3(), //!< When dropped, the position it will fall to
    droppedModelName: new StateString(),
    HUDPosition: new StateArrayFloat(),

    init: function() {
        this.states = ['closed', 'moving', 'open'];
        this.stateModelNames = ['nut', '', ''];
        this.stateAnimations = ['ANIM_IDLE|ANIM_LOOP', 'ANIM_TRIGGER', 'ANIM_TRIGGER']
        this.HUDIcon = 'packages/gamehud/.../.../...';
        this.HUDPosition = [0.92, 0.75]; // XXX
        this.droppedModelName = 'nut';
        this.droppedPosition = [0,0,0];
    },

    activate: function() {
        this.carryingPlayer = -1;
        this.droppedPosition = [0,0,0];
    },

    onTrigger: function(collider) {
        if (this.state === 'closed') {
            this.setState('moving');
            this.carryingPlayer = collider.uniqueId;
        }
    },

    act: function() {
        // Handle moving item
        if (this.state === 'moving') {
            var carrier = getEntity(this.carryingPlayer);

            if (this.carryingPlayer >= 0) {
                if (!Health.isActiveEntity(carrier)) {
                    // Dropped because player is not active, or left game
                    this.carryingPlayer = -1;
                    if (carrier && !carrier.deactivated) {
                        this.droppedPosition = carrier.position.asArray();
                    } else {
                        this.droppedPosition = [0,0,0];
                        this.setState('closed');
                    }
                } else {
                    // Check if player arrives at target
                    var barrier = getEntity(this.parentEntity);
                    if (barrier) {
                        // Check for arrival
                        if (World.isPlayerCollidingEntity(carrier, barrier)) {
                            this.setState('open');
                            this.carryingPlayer = -1;
                            this.droppedPosition = [0,0,0];
                            barrier.refreshBarrier();
                        }
                    }
                }
            } else {
                // Check pickup by other players
                if (!this.pickupEvent) {
                    this.pickupEvent = GameManager.getSingleton().eventManager.add({
                        secondsBefore: 0,
                        secondsBetween: 1/5,
                        func: bind(function() {
                            var players = getEntitiesByClass('Player');
                            for (var i = 0; i < players.length; i++) {
                                var player = players[i];
                                if (!Health.isActiveEntity(player)) continue;
                                if (World.isPlayerCollidingEntity(player, {
                                    position: this.droppedPosition,
                                    collisionRadiusWidth: this.collisionRadiusWidth,
                                    collisionRadiusHeight: this.collisionRadiusHeight,
                                })) {
                                    this.carryingPlayer = player.uniqueId;
                                    this.pickupEvent = null;
                                    return false;
                                }
                            }
                        }, this),
                        entity: this,
                    });
                }
            }
        }
    },
 
    clientActivate: function() {
        this.connect('client_onModify_carryingPlayer', function(uniqueId) {
            var player = getEntity(uniqueId);
            if (player) {
                Sound.play('0ad/fs_sand7.ogg', player.position.copy());
            }
        });

        this.connect('client_onModify_state', function(state) {
            if (state === 'open') {
                var parent = getEntity(this.parentEntity);
                if (parent) {
                    Sound.play('0ad/alarmcreatemiltaryfoot_1.ogg', parent.position.copy());
                }
            }
        });
    },

    clientAct: function() {
        // Display

        if (this.state === 'moving') {
            var carrier = getEntity(this.carryingPlayer);

            var position = Health.isActiveEntity(carrier) ? carrier.position.copy() : this.droppedPosition;
            // HUD
            if (carrier === getPlayerEntity()) {
                var factors = Global.gameHUD.calcFactors();
                CAPI.showHUDImage(this.HUDIcon, factors.x*this.HUDPosition.get(0), factors.y*this.HUDPosition.get(1), factors.x*32/Global.screenWidth, factors.y*32/Global.screenHeight);
            } else {
                // Radar
                if (carrier !== getPlayerEntity) {
                    GameManager.getSingleton().drawRadarElement(position, this.HUDIcon);
                }
            }
        }
    },

    renderDynamic: function() {
        if (this.state === 'moving') {
            var carrier = getEntity(this.carryingPlayer);

            var position = Health.isActiveEntity(carrier) ? carrier.position.copy() : this.droppedPosition;

            // World
            if (!carrier) {
                CAPI.renderModel2.apply(null, [this, this.droppedModelName, ANIM_IDLE|ANIM_LOOP, position.x, position.y, position.z, this.yaw, 0, 0, MODEL.LIGHT | MODEL.CULL_VFC | MODEL.CULL_DIST | MODEL.DYNSHADOW, this.startTime]);
//                Effect.splash(PARTICLE.SPARK, 2, 0.3, position, 0xFF00FF, 1.2, 50, -20);
            }
        }
    }
}]));


//! Plot barriers stay in place until all relevant plot triggers are triggered
PlotBarrierPlugin = {
    _class: 'PlotBarrier',
    shouldAct: true,
    childEntityClass: 'PlotTrigger',

    activate: function() {
        this.resetBarrier();

        this.refreshEvent = GameManager.getSingleton().eventManager.add({
            secondsBefore: 0,
            secondsBetween: 5,
            func: bind(this.refreshBarrier, this),
            entity: this,
        }, this.refreshEvent);
    },

    resetBarrier: function() {
        this.setState('closed');

        forEach(getEntitiesByClass(this.childEntityClass), function(trigger) {
            if (trigger.parentEntity === this.uniqueId) {
                trigger.reset();
            }
        }, this);
    },

    refreshBarrier: function() {
        if (this.state === 'open' && getClientEntities().length === 0) {
            this.resetBarrier();
            return -1;
        }

        var triggers = filter(
            function(trigger) {
                return trigger.parentEntity === this.uniqueId;
            },
            getEntitiesByClass(this.childEntityClass),
            this
        );
        if (triggers.length === 0) return;
        var remaining = filter(function(trigger) { return trigger.state !== 'open'; }, triggers);
        var shouldOpen = remaining.length === 0;
        if (shouldOpen && this.state !== 'open') {
            this.setState('open');
        } else if (!shouldOpen && this.state !== 'closed') {
            this.resetBarrier();
        }

        return -1;
    },
};

PlotBarrier = registerEntityClass(bakePlugins(Mapmodel, [StatePlugin, PlotBarrierPlugin]));


DoorOpeningAction = Action.extend({
    doStart: function() {
        this.actor.originalPosition = this.actor.position.copy();

        this.totalTime = this.actor.doorParams.get(2);
        this.secondsLeft = this.totalTime;
        this.speed = this.actor.doorParams.get(1)/this.totalTime;
        this.originalPosition = this.actor.position.copy();

        var dir = this.actor.doorParams.get(0);
        if (Math.abs(dir) === 1) {
            this.direction = new Vector3().fromYawPitch(this.actor.yaw, 0).mul(dir*this.speed);
        } else {
            this.direction = new Vector3(0, 0, sign(dir)*this.speed);
        }

        this.timer = new RepeatingTimer(1/30); // 30 fps max for CAPI commands

        if (Global.CLIENT) {
            Sound.play('swarm/door.ogg', this.originalPosition);
        }
    },

    doExecute: function(seconds) {
        if (this.timer.tick(seconds)) {
            var position = this.originalPosition.addNew(this.direction.mulNew(this.totalTime - this.secondsLeft));
            CAPI.setExtentO(this.actor, position);
        }
        return this._super.apply(this, arguments);
    },
});


Door = registerEntityClass(bakePlugins(PlotBarrier, [{
    _class: 'Door',

    //! Direction (+1=right, -1=left, +2=up, -2=down), distance, timeToOpen
    doorParams: new StateArrayFloat(),

    init: function() {
        this.stateModelNames = ['gk/swarm/door512x1024', 'gk/swarm/door512x1024'];
        this.doorParams = [-1, 70, 10];
    },

    activate: function() {
        this.connect('onModify_state', this.checkOpenDoor);
    },

    clientActivate: function() {
        this.connect('client_onModify_state', this.checkOpenDoor);
    },

    checkOpenDoor: function(state) {
        this.clearActions(); // Stop opening
        if (state === 'open') {
            this.queueAction(new DoorOpeningAction());
        }
    },

    resetBarrier: function() {
        if (this.originalPosition) {
            this.position = this.originalPosition;
        }
        this._super();
    },

    createStateDataDict: function() {
        var ret = this._super.apply(this, arguments);

        // On the client, we are saving entities... for that, save the ORIGINAL door position!
        if (Global.CLIENT && this.originalPosition) {
            ret['state'] = 'closed';
            ret['position'] = WrappedCVector3.prototype.toData(this.originalPosition);
        }

        return ret;
    },
}]));


//! A plot trigger whose parents are other plot triggers
RecursivePlotTrigger = registerEntityClass(bakePlugins(PlotTrigger, [PlotBarrierPlugin, {
    _class: "RecursivePlotTrigger",

    onTrigger: function(collider) { },
}]));

RecursivePlotTriggerItem = registerEntityClass(bakePlugins(PlotTriggerItem, [PlotBarrierPlugin, {
    _class: "RecursivePlotTriggerItem",

    onTrigger: function(collider) { },
}]));

