// (C) 2009 Alon 'Kripken' Zakai

Library.include('library/1_3/');

Library.include('library/' + Global.LIBRARY_VERSION + '/__CorePatches');

Library.include('library/' + Global.LIBRARY_VERSION + '/Plugins');
Library.include('library/' + Global.LIBRARY_VERSION + '/Health');
Library.include('library/' + Global.LIBRARY_VERSION + '/GameManager');
Library.include('library/' + Global.LIBRARY_VERSION + '/Vehicles');
Library.include('library/' + Global.LIBRARY_VERSION + '/Chat');
Library.include('library/' + Global.LIBRARY_VERSION + '/World');
Library.include('library/' + Global.LIBRARY_VERSION + '/mapelements/PlotTriggers');
Library.include('library/' + Global.LIBRARY_VERSION + '/mapelements/WorldSequences');
Library.include('library/' + Global.LIBRARY_VERSION + '/modes/Racing');

// Default materials, etc.

Library.include('library/' + Global.LIBRARY_VERSION + '/MapDefaults');

// Textures

Library.include('yo_frankie/');

Library.include('textures/gk/brick3/');
Library.include('textures/gk/concrete3/');
Library.include('textures/gk/deco3/');
Library.include('textures/gk/ground3/');
Library.include('textures/gk/metal3/');
Library.include('textures/gk/morter3/');
Library.include('textures/gk/rock3/');
Library.include('textures/gk/walls3/');
Library.include('textures/gk/wood3/');
Library.include('textures/gk/set4/');
Library.include('textures/gk/set5/');

// Map settings

Map.fogColor(0, 0, 0);
Map.fog(9999);
Map.loadSky("skyboxes/gk2/nnu_sb01");
Map.skylight(100, 100, 100);
Map.ambient(20);
Map.shadowmapAmbient("0x202020");
Map.shadowmapAngle(300);


//// Player class

GamePlayer = registerEntityClass(
    bakePlugins(
        Player,
        [
            Vehicles.plugin,
            Vehicles.extraPlugins.floorOrientation,
//            Vehicles.extraPlugins.limitYawing,
            Health.plugin,
            GameManager.playerPlugin,
            Chat.playerPlugin,
            WorldSequences.plugins.player,
            RacingMode.playerPlugin,
            {
                _class: "GamePlayer",
                HUDModelName: '',

                accessories: new StateJSON(),

                init: function() {
                    this.modelName = '';
                    this.accessories = {};
                    this.eyeHeight = 6;
                    this.aboveEye = 6;
                    this.radius = 8;

                    forEach(['tag_exhaust', 'tag_tread_fl', 'tag_tread_fr', 'tag_tread_rl', 'tag_tread_rr'], function(tag) {
                        this.attachments.push('*' + tag);
                    }, this);
                },

                clientActivate: function() {
                    this.renderDynamic = function(HUDPass, needHUD) {
                        if (!this.initialized) return;

                        if (needHUD) return;

                        if (this.renderingArgsTimestamp !== currTimestamp) {
                            // Same naming conventions as in rendermodel.cpp in sauer

                            var state = this.clientState;

                            if (state == CLIENTSTATE.SPECTATOR || state == CLIENTSTATE.SPAWNING) {
                                return;
                            }

                            var mdlname = HUDPass && needHUD ? this.HUDModelName : this.modelName;
                            var yaw = this.yaw;
                            var pitch = this.floorOrientation.pitch;
//log(ERROR, pitch);
                            var o = this.position.copy();
                            var basetime = this.startTime;
                            var physstate = this.physicalState;
                            var inwater = this.inWater;
                            var move = this.move;
                            var strafe = this.strafe;
                            var vel = this.velocity.copy();
                            var falling = this.falling.copy();
                            var timeinair = this.timeInAir;
                            var anim = this.decideVehicleAnimation(state, physstate, move, strafe, vel, falling, inwater, timeinair);

                            var flags = MODEL.LIGHT;
                            if (this !== getPlayerEntity()) {
                                flags |= MODEL.CULL_VFC | MODEL.CULL_OCCLUDED | MODEL.CULL_QUERY;
                            }
                            flags |= MODEL.FULLBRIGHT; // TODO: For non-characters, use: flags |= MODEL.CULL_DIST;
                            var fade = 1.0;
                            if (state == CLIENTSTATE.LAGGED) {
                                fade = 0.3;
                            } else {
                                flags |= MODEL.DYNSHADOW;
                            }

                            this.renderingArgs = [this, mdlname, anim, o.x, o.y, o.z, yaw, pitch, flags, basetime];
                            this.renderingArgsTimestamp = currTimestamp;
                        }

                        CAPI.renderModel.apply(this, this.renderingArgs);
                    };
                },

                createRenderingArgs: function(yaw, pitch, anim, o, flags, basetime) {
                    pitch = 0; // Unless we override. But we ignore the camera.

                    var ret = [
                        [this, this.modelName, anim, o.x, o.y, o.z, yaw, pitch, flags, basetime]
                    ];

/*
                    forEach(keys(this.accessories), function(accessory) {
                        ret.push(
                            [this, 'space/ships/1/' + accessory, anim, o.x, o.y, o.z, yaw, pitch, flags, basetime]
                        );
                    }, this);
*/

                    return ret;
                },

                getThrustDirection: function() {
                    return new Vector3().fromYawPitch(this.yaw, 0).normalize(); // No pitching for thrust
                },

                getExhaustPosition: function() {
                    return CAPI.getAttachmentPosition(this, 'tag_exhaust');
                },

                clientAct: function(seconds) {
                    // Dust emitters
                    if (this.clientState !== CLIENTSTATE.EDITING) {
                        var velocityMagnitude = this.velocity.magnitude();
                        if (this.onFloor && velocityMagnitude > 20) {
                            forEach(['tag_tread_fl', 'tag_tread_fr', 'tag_tread_rl', 'tag_tread_rr'], function(tread_tag) {
                                var position = CAPI.getAttachmentPosition(this, tread_tag);
                                Effect.splash(PARTICLE.SMOKE, 2, 0.3, position, 0x525252, clamp(velocityMagnitude/25, 0.5, 4), velocityMagnitude, velocityMagnitude);
                           }, this);
                        }
                    }

                    if (this.health <= 0) {
                       Effect.splash(PARTICLE.SMOKE, 4, 0.5, this.position.copy(), 0x101010, 10, 100, -25);
                    }

                    if (this === getPlayerEntity() && this.raceStatus === RacingMode.STATUS.inProgress) {
                        // Racing timer
                        var time = decimal2(Global.time - this.raceStartTime);
                        CAPI.showHUDText("Time: " + time + " seconds", 0.5, 0.1, 0.75, 0xFFCC33);

                        if (time > 12 && this.worldSequences['racetrack'] < 0) {
                            this.health = 0;
                        }
                    }
                },
            },
        ]
    )
);

//// Racetrack sequence

RacetrackSequencePlugin = {
    _class: 'RacetrackSequence',

    init: function() {
        this.sequenceId = 'racetrack';
        this.collisionRadiusWidth = 20;
    },
};

RacetrackFlameMarkerPlugin = {
    shouldAct: true,

    init: function() {
        this.sequenceIsMandatory = true;
    },

    clientAct: function() {
        if (!this.flameMarker) {
            var flameMarkerEntity = getEntityByTag('flamemarker_' + this.uniqueId);

            if (!flameMarkerEntity) {
                log(WARNING, 'Cannot find flame marker for ' + this.uniqueId);
                return;
            } else {
                this.flameMarker = flameMarkerEntity.position.copy();
            }
        }

        if (!this.sequenceIsMandatoryPassed && getPlayerEntity().worldSequences && getPlayerEntity().worldSequences[this.sequenceId] === this.sequenceNumber - 1) {        
            Effect.flame(PARTICLE.FLAME, this.flameMarker, 5.0, 5.0, 0xDD8869, 2, 3.0, 100, 0.4, -6);
        }
    },
};

registerEntityClass(bakePlugins(AreaTrigger, [WorldSequences.plugins.areaTrigger, RacetrackSequencePlugin]));

registerEntityClass(bakePlugins(AreaTrigger, [WorldSequences.plugins.areaTrigger, RacetrackSequencePlugin, {
    _class: 'RacetrackSequenceStart',
}]));

registerEntityClass(bakePlugins(AreaTrigger, [WorldSequences.plugins.areaTrigger, RacetrackSequencePlugin, {
    _class: 'RacetrackSequenceFinish',

    onSequenceArrival: function(entity) {
        if (entity === getPlayerEntity()) {
            GameManager.getSingleton().playerFinishedRace = entity.uniqueId;
        }
    },
}]));

//// Application

ApplicationManager.setApplicationClass(Application.extend({
    _class: "GameApplication",

    getPcClass: function() {
        return "GamePlayer";
    },

    // Replace this with appropriate behaviour for when a player falls of the map
    clientOnEntityOffMap: Health.dieIfOffMap,

    getScoreboardText: GameManager.getScoreboardText,

//    performJump: function(down) {
//        if (down) log(ERROR, "nojumpforyou");
//    },

//    clientClick: function(button, down) {
//        if (down) {
//            log(ERROR, "Bararum");
//            WorldSignals.emit(SIGNAL.LOUD_NOISE, getPlayerEntity().getCenter());
//        }
//    },
}));

ApplicationManager.instance.connect('actionKey', GameManager.managerPlugins.highScoreTable.actionKey);

// Setup game

GameManager.setup([
    GameManager.managerPlugins.messages,
    GameManager.managerPlugins.eventList,
    GameManager.managerPlugins.highScoreTable.plugin,
    RacingMode.managerPlugin,
    RacingMode.highScores.managerPlugin,
    {
        racingMode: {
            maxTime: 3*60,
        },

        clientActivate: function() {
            this.addHUDMessage({
                text: "Press 'H' for high scores",
                color: 0xDDEEFF,
                duration: 4.0,
                size: 0.8,
                y: 0.25,
            });
        },

        startRace: function() {
            forEach(getEntitiesByTag('start_door'), function(entity) {
                entity.state = 'open';
            });

            GameManager.getSingleton().eventManager.add({
                secondsBefore: 10,
                func: function() {
                    forEach(getEntitiesByTag('start_door'), function(entity) {
                        entity.state = 'closed';
                    });
                },
            });
        },
    },
]);

if (Global.SERVER) { // Run this only on the server - not the clients
    var entities = CAPI.readFile("./entities.json");
    loadEntities(entities);

    GameManager.getSingleton().registerTeams([
        {
            _name: 'drivers',
            setup: function(player) {
                player.defaultModelName = 'speedtank/' + [
                    'blue', 'green', 'grey', 'purple', 'red'
                ][Math.floor(Math.random()*5)];
            },
        },
    ]);
}

// Patches

DeathAction.prototype.secondsLeft = 2.0;

