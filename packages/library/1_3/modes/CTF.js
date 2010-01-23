
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


Library.include('library/' + Global.LIBRARY_VERSION + '/Health');


// Adds 2 flags to the game, and the concept of capturing the other team's flag

CTFMode = {
    playerPlugin: {
        activate: function() {
            this.connect('fragged', function() {
                GameManager.getSingleton().dropFlags(this);
            });
            this.connect('preDeactivate', function() {
                GameManager.getSingleton().dropFlags(this);
            });
        },
    },

    FlagState: {
        Based: -1,
        Free: -2,
        Score: -3,
    },

    managerPlugin: {
        flag0Team : new StateString(),
        flag0State: new StateInteger(),
        flag0Position: new StateArrayFloat(),

        flag1Team : new StateString(),
        flag1State: new StateInteger(),
        flag1Position: new StateArrayFloat(),

        // Parameters
        CTFMode: {
            score: 1,
        },

        activate: function() {
            this.connect('postRegisterTeams', function() {
                var teamNames = keys(this.teams);

                for (var i = 0; i < 2; i++) {
                    this['flag' + i + 'Team'] = teamNames[i];
                }

                this.resetFlags();
            });

            this.connect('startGame', function() {
                this.resetFlags();
            });

            this.resetFlags();

            this.connect('onModify_flag0Position', this.onFlagPlaced);
            this.connect('onModify_flag1Position', this.onFlagPlaced);

            this.connect('onModify_flag0State', partial(this.onFlagState, 0));
            this.connect('onModify_flag1State', partial(this.onFlagState, 1));

            this.freeFlagExpirationTimers = [null, null];
        },

        act: function(seconds) {
            for (var i = 0; i < 2; i++) {
                if (this.freeFlagExpirationTimers[i] && this.freeFlagExpirationTimers[i].tick(seconds)) {
                    Sound.play('0ad/alarmcreatemiltaryfoot_1.ogg'); // A player returned a flag to home
                    this.resetFlag(i);
                    this.freeFlagExpirationTimers[i] = null;
                }
            }
        },

        onFlagState: function(i, value, actorUniqueId) {
            if (value === CTFMode.FlagState.Score) {
                this.adjustScore(this.getFlagTeam(i), this.CTFMode.score);
                this.resetFlags();
                throw "CancelStateDataUpdate"; // Do not actually set the value - we reset it in resetFlags
            } else if (value === CTFMode.FlagState.Based) {
                if (actorUniqueId) { // A player returned a flag to home
                    Sound.play('0ad/alarmcreatemiltaryfoot_1.ogg');
                    this.freeFlagExpirationTimers[i] = null; // Stop the expiration timer
                }
            } else if (value === CTFMode.FlagState.Free) {
                // Start timer for flag expiration
                this.freeFlagExpirationTimers[i] = new RepeatingTimer(6.0);
            } else if (value >= 0) {
                var player = getEntity(value);
                if (player !== null && Health.isActiveEntity(player)) {
                    Sound.play('0ad/fs_sand7.ogg', player.position.copy());
                    this.freeFlagExpirationTimers[i] = null; // Cancel the timer, flag is safely held
                } else {
                    // Player may have disconnected, or died so cancel this

                    // Send an update with the old value. This resets the player
                    // so that they can operate on the flag again (otherwise isFlagDataPending
                    // would be an issue).
                    this['flag' + i + 'State'] = this['flag' + i + 'State'];

                    // Do not send this update, which has the player as the value, as it is wrong
                    throw "CancelStateDataUpdate";
                }
            }
        },

        resetFlags: function() {
            for (var i = 0; i < 2; i++) {
                this.resetFlag(i);
            }
        },

        resetFlag: function(i) {
            if (this.deactivated) return;

            if (this.isFlagDataPending(i)) return;

            this['flag' + i + 'State'] = CTFMode.FlagState.Based;
            var startTag = 'flag_start_' + this.getFlagTeam(i);
            var start = getEntityByTag(startTag);
            if (start !== null) {
                log(WARNING, 'flag placed successfully');
                this['flag' + i + 'Position'] = start.position.asArray();
            } else {
                log(WARNING, format('flag start not found ("{0}"), placing elsewhere...', startTag));
                this['flag' + i + 'Position'] = [600, 600, 600];
            }

            this.setFlagDataPending(i, true);
        },

        pickupFlag: function(i, player) {
            if (this.deactivated) return;

            if (this.isFlagDataPending(i)) return;

            var flagModelName = this.teamData[this.getFlagTeam(i)].flagModelName;
            if (!flagModelName) return; // No actual flag to pick up - just a dummy

            this['flag' + i + 'State'] = player.uniqueId;

            this.setFlagDataPending(i, true);
        },

        dropFlags: function(player) {
            if (this.deactivated) return;

            for (var i = 0; i < 2; i++) {
                if (this.getFlagState(i) === player.uniqueId) {
                    if (this.isFlagDataPending(i)) continue;

                    this['flag' + i + 'State'] = CTFMode.FlagState.Free;
                    this['flag' + i + 'Position'] = player.position.asArray();

                    this.setFlagDataPending(i, true);
                }
            }
        },

        onFlagPlaced: function() {
            return;
            // Drop to floor
            for (var i = 0; i < 2; i++) {
                var position = this.getFlagPosition(i);
                var fall = floorDistance(position, 1024);
                if (fall > 1.0) {
                    this['flag' + i + 'Position'].set(2, position.z - fall);
                }
            }
        },

        clientActivate: function(kwargs) {
            this.flagRenderingArgs = [];
            this.flagRenderingArgsTimestamp = -1;

            this.collisionTimer = new RepeatingTimer(0.1);

            this.connect('client_onModify_flag0State', partial(this.clientOnFlagState, 0));
            this.connect('client_onModify_flag1State', partial(this.clientOnFlagState, 1));

            this.flagFallingPhysics = [null, null];

            // Set to true for a flag when we try to do an action. Until we get a
            // server response for that flag, we can't do anything else to it. So,
            // by standing on a flag, we can only pick it up once.
            this.flagDataPending = [false, false];
        },

        isFlagDataPending: function(i) {
            if (Global.SERVER) return false;

            return this.flagDataPending[i];
        },

        setFlagDataPending: function(i, value) {
            if (Global.CLIENT) {
                this.flagDataPending[i] = value;
            }
        },

        clientOnFlagState: function(i, value) {
            this.setFlagDataPending(i, false);

            if (value === CTFMode.FlagState.Free) { // || value === CTFMode.FlagState.Based) {
                // Start flag falling physics
                var position = this.getFlagPosition(i);

                this.flagFallingPhysics[i] = {
                    targetZ: position.z - floorDistance(position, 1024),
                    currZ: position.z,
                    velocity: 0,
                };
            } else {
                this.flagFallingPhysics[i] = null;
            }
        },

        getFlagPosition: function(i) {
            var state = this.getFlagState(i);
            if (state < 0 || getEntity(state) === null) {
                var ret = new Vector3(this['flag' + i + 'Position'].asArray());
                var physics = this.flagFallingPhysics[i];
                if (physics) {
                    ret.z = physics.currZ;
                }
                return ret;
            } else {
                var holder = getEntity(state);
                var offset = (new Vector3()).fromYawPitch(holder.yaw, 0).mul(-2);
                offset.yaw = holder.yaw; // Patch it in
                return offset.add(holder.position);
            }
        },

        getFlagTeam: function(i) {
            return this['flag' + i + 'Team'];
        },

        getFlagState: function(i) {
            return this['flag' + i + 'State'];
        },

        clientAct: function(seconds) {
            for (var i = 0; i < 2; i++) {
                var physics = this.flagFallingPhysics[i];
                if (physics && physics.currZ > physics.targetZ) {
                    physics.velocity += seconds * World.gravity;
                    physics.currZ = Math.max(physics.targetZ, physics.currZ - seconds * physics.velocity);
                    if (physics.currZ <= 0) {
                        this.resetFlag(i); // fell off map
                    }
                }
            }

            var player = getPlayerEntity();

            // Check for pickup of enemy flag, and for arrival at home base with flag.
            if (this.collisionTimer.tick(seconds)) {
                if (player.health > 0) {
                    var playerPosition = player.position.copy(); // compare feet position of both
                    var that = this;
                    function collideWithFlag(i) {
                        return playerPosition.isCloseTo(that.getFlagPosition(i), 10.0); // 8.25
                    }
                    for (var i = 0; i < 2; i++) {
                        if (player.team === this.getFlagTeam(i)) {
                            // Collide with our flag, if free, reset it
                            if (this.getFlagState(i) === CTFMode.FlagState.Free && collideWithFlag(i)) {
                                this.resetFlag(i);
                            }

                            if (this.getFlagState(i) === CTFMode.FlagState.Based && 
                                this.getFlagState(1-i) === player.uniqueId && 
                                collideWithFlag(i)) {
                                this.scorePoint(player);
                            }
                        } else {
                            // Collide with enemy flag, if free, pick it up
                            if ((this.getFlagState(i) === CTFMode.FlagState.Free ||
                                 this.getFlagState(i) === CTFMode.FlagState.Based) && collideWithFlag(i)) {
                                this.pickupFlag(i, player);
                            }
                        }
                    }
                }
            }

            // HUD

            if (CAPI.showHUDRect) {
                // Radar
                var centerX = 0.90;
                var centerY = 0.15;
                var radius = 0.1;
//                CAPI.showHUDRect(centerX, centerY, -radius*2, -radius*2, 0x000000);
                CAPI.showHUDRect(centerX, centerY, -0.98*radius*2, -0.98*radius*2, 0xF0F0F0);
                CAPI.showHUDImage('packages/hud/radar/gk_level_2_rada_01_frame.png', centerX, centerY, radius*2, radius*2);
                CAPI.showHUDImage('packages/hud/radar/gk_level_2_rada_x.png', centerX, centerY, 0.015, 0.015);
                for (var i = 0; i < 2; i++) {
                    var team = this.getFlagTeam(i);
                    var icon = 'packages/hud/radar/gk_level_2_rada_f' + team[0] + '.png';
                    var direction = this.getFlagPosition(i).subNew(player.position);
                    direction.z = 0; // Don't care about up-down differences
                    var distance = Math.pow(direction.magnitude(), 0.66)/150;
                    var yaw = direction.toYawPitch().yaw - player.yaw;
                    yaw = Math.PI*(yaw - 90)/180;

                    CAPI.showHUDImage(icon,
                        centerX + clamp(distance*radius*Math.cos(yaw), -radius*0.9, radius*0.9)/Global.aspectRatio,
                        centerY + clamp(distance*radius*Math.sin(yaw), -radius*0.9, radius*0.9),
                        0.020, 0.02
                    );
                }
            }

            if (CAPI.showHUDImage) {
                for (var i = 0; i < 2; i++) {
                    if (this.getFlagState(i) === player.uniqueId) {
                        var flagIndex = this.getFlagTeam(i) === 'blue' ? 2 : 3;
                        var flag = 'packages/hud/gui_gk_Icon_f0' + flagIndex + '.png';
                        CAPI.showHUDImage(flag, 0.92, 0.5, 0.05, 0.05);
                    }
                }
            }
        },

        scorePoint: function(player) {
            for (var i = 0; i < 2; i++) {
                if (this.getFlagTeam(i) === player.team) {
                    if (this.isFlagDataPending(i)) continue;

                    this['flag' + i + 'State'] = CTFMode.FlagState.Score;

                    this.setFlagDataPending(i, true);
                }
            }
        },

        renderDynamic: function() {
            if (!this.initialized) {
                return;
            }

            for (var i = 0; i < 2; i++) {
                if (this.flagRenderingArgsTimestamp !== currTimestamp) {
                    var flagModelName = this.teamData[this.getFlagTeam(i)].flagModelName;
                    if (!flagModelName) {
                        this.flagRenderingArgs[i] = null;
                        continue;
                    }

                    var o = this.getFlagPosition(i);
                    var flags = MODEL.LIGHT | MODEL.CULL_VFC | MODEL.FULLBRIGHT | MODEL.CULL_DIST | MODEL.DYNSHADOW;
                    var yaw = o.yaw ? o.yaw : 0;
                    this.flagRenderingArgs[i] = [this, flagModelName, ANIM_IDLE|ANIM_LOOP, o.x, o.y, o.z, yaw, 0, flags, 0];
                }

                if (this.flagRenderingArgs[i]) {
                    CAPI.renderModel.apply(this, this.flagRenderingArgs[i]);
                }
            }

            this.flagRenderingArgsTimestamp = currTimestamp;
        },
    },
};

Map.preloadSound('0ad/fs_sand7.ogg');
Map.preloadSound('0ad/alarmcreatemiltaryfoot_1.ogg');
Map.preloadSound('0ad/alarmvictory_1.ogg');

