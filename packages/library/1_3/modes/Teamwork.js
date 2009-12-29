
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



// Forces the teammates to stick together

TeamworkMode = {
    managerPlugin: {
        teamworkWarning: new StateString({ reliable: false, hasHistory: false }),

        // Override this if you want
        teamworkMode: {
            interval: 2.0,
            distanceFunc: function(dist) {
                return Math.exp(-dist*dist/6400) - 0.5;
            },
            applyEffect: function(player, effect) {
                effect = Math.ceil(effect*5);
                var newHealth = clamp(player.health + effect, 0, player.maxHealth);
                if (newHealth !== player.health) {
                    player.health = newHealth;
                }
            },
        },

        activate: function() {
            this.connect('startGame', function() {
                this.teamworkMode.timer = new RepeatingTimer(this.teamworkMode.interval);
            });
        },

        act: function(seconds) {
            if (!this.gameRunning) return;

            if (this.teamworkMode.timer.tick(seconds)) {
                forEach(values(this.teams), function(team) {
                    var allGood = true;
                    var players = team.playerList;
                    if (players.length <= 1) return;
                    forEach(players, function(player) {
                        if (!Health.isValidTarget(player)) return;
                        var effect = 0;
                        forEach(players, function(otherPlayer) {
                            if (!Health.isValidTarget(otherPlayer)) return;
                            if (player === otherPlayer) return;
                            var dist = player.position.subNew(otherPlayer.position).magnitude();
                            effect += this.teamworkMode.distanceFunc(dist);
                        }, this);
                        allGood = allGood && (effect >= 0);
                        this.teamworkMode.applyEffect(player, effect/(players.length-1));
                    }, this);
                    if (!allGood) {
                        this.teamworkWarning = team._name;
                    }
                }, this);
            }
        },

        clientActivate: function() {
            this.lastWarning = Global.time;

            this.connect('client_onModify_teamworkWarning', function(team) {
                if (getPlayerEntity() && getPlayerEntity().team === team && Global.time - this.lastWarning > 12.0) {
                    this.addHUDMessage({
                        text: 'Warning: Do not split up the team, or you will weaken!',
                        color: 0xFFAA30,
                        duration: 4,
                        size: 0.5,
                        x: 0.5,
                        y: 0.25
                    });
                    this.lastWarning = Global.time;
                }
            });
        },
    },
};

