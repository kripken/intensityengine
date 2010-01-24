
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



RacingMode = {
    STATUS: {
        ready: 0,
        inProgress: 1,
    },

    managerPlugin: {
        playerFinishedRace: new StateInteger({ hasHistory: false }),

        racingMode: {
            maxTime: 1*60,
        },

        activate: function() {
            GameManager.getSingleton().eventManager.add({
                secondsBefore: 0,
                secondsBetween: 5,
                func: function() {
                    GameManager.getSingleton().refreshRace();
                },
            });

            this.raceStatus = 0;

            this.connect('onModify_playerFinishedRace', function(uniqueId) {
                var player = getEntity(uniqueId);
                if (!player) return;

                this.addHUDMessage({
                    text: "Your time: " + decimal2(Global.time - this.raceStartTime),
                    color: 0xFFEEFF,
                    duration: 5,
                    y: 0.333,
                    size: 0.66,
                    player: player,
                });

                GameManager.getSingleton().eventManager.add({
                    secondsBefore: 10,
                    func: function() {
                        player.respawn();
                    },
                });
            });
        },

        clientActivate: function() {
            this.connect('client_onModify_playerFinishedRace', function(uniqueId) {
                var player = getEntity(uniqueId);
                if (player !== getPlayerEntity()) return;

                Sound.play('0ad/alarmvictory_1.ogg');
            });
        },

        //! Override with stuff that starts the race, like opening the way to the start line
        startRace: function() { },

        refreshRace: function() {
            var players = filter(function(player) { return !isPlayerEditing(player); }, getClientEntities());
            if (players.length === 0) return;

            var statuses = {};
            forEach(values(RacingMode.STATUS), function(value) { statuses[value] = 0; });
            forEach(players, function(player) {
                statuses[player.raceStatus] += 1;
            });

            if (statuses[RacingMode.STATUS.inProgress] === 0 && this.raceStatus === 0) {
                // Can start new race
                this.raceStatus = 1;

                GameManager.getSingleton().eventManager.add({
                    secondsBefore: 0, // Next frame
                    func: bind(function() {
                        this.raceStatus = 2;

                        this.addHUDMessage({
                            text: "Race will start in a few seconds!",
                            color: 0xFFEEDD,
                            duration: 8,
                            y: 0.333,
                            size: 0.66,
                        });

                        this.eventManager.add({
                            secondsBefore: 10,
                            func: bind(function() {
                                this.raceStartTime = Global.time;

                                this.raceStatus = 3;

                                this.addHUDMessage({
                                    text: "GO!",
                                    color: 0xFFEEAA,
                                    duration: 2,
                                    y: 0.333,
                                    size: 1.0,
                                });

                                forEach(getClientEntities(), function(player) {
                                    player.raceStatus = RacingMode.STATUS.inProgress;
                                });

                                this.startRace();
                            }, this),
                        });
                    }, this),
                });
            } else {
                // Race in progress, check for expiration. Frag players that are too tardy
                // For fragged players, restart their status to ready
                if (Global.time - this.raceStartTime > this.racingMode.maxTime || statuses[RacingMode.STATUS.inProgress] === 0) {
                    forEach(getClientEntities(), function(player) {
                        if (player.raceStatus === RacingMode.STATUS.inProgress && Health.isValidTarget(player)) player.health = 0;
                    });

                    if (this.raceStatus >= 3) {
                        this.addHUDMessage({
                            text: "Race is over",
                            color: 0xCCEEFF,
                            duration: 3.33,
                            y: 0.333,
                            size: 0.66,
                        });

                        this.raceStatus = 0;
                    }
                }
            }
        },
    },

    playerPlugin: {
        raceStatus: new StateInteger(),

        clientActivate: function() {
            this.connect('clientRespawn', function() {
                this.raceStatus = RacingMode.STATUS.ready;
                if (this.resetWorldSequence) this.resetWorldSequence('racetrack');
            });

            this.connect('client_onModify_raceStatus', function(status) {
                if (status === RacingMode.STATUS.inProgress) {
                    this.raceStartTime = Global.time;
                }
            });
        },
    },

    highScores: {
        managerPlugin: {
            activate: function() {
                this.highScoreData = {
                    biggerScoresAreBetter: false, // lower seconds - better race
                    maxScores: 5,
                    scores: (this.highScoreData && this.highScoreData.scores) ? this.highScoreData.scores : [],
                    unit: 'seconds',
                    oneScorePerPlayer: true,
                };

                this.connect('onModify_playerFinishedRace', function(uniqueId) {
                    var player = getEntity(uniqueId);
                    if (!player) return;

                    var finalTime = decimal2(Global.time - this.raceStartTime);
                    if (GameManager.getSingleton().addPossibleHighScore(player._name, finalTime)) {
                        GameManager.getSingleton().addHUDMessage("New high score! " + finalTime + ' seconds', 0xFFDD99, 10.0, 0.8, 0, 0, player);
                        Sound.play('0ad/alarmvictory_1.ogg');
                    } else {
                        GameManager.getSingleton().addHUDMessage('You finished in ' + finalTime + ' seconds', 0xFFFFFF, 10.0, 0.8, 0, 0, player);
                        Sound.play('0ad/alarmcreatemiltaryfoot_1.ogg');
                    }
                });
            },
        },
    },
};

