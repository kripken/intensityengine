
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


GameManager = {
    playerPlugin: {
        team: new StateString(),

        activate: function() {
            GameManager.getSingleton().pickTeam(this);

            this.connect('preDeactivate', function() {
                GameManager.getSingleton().leaveTeam(this);
            });

            this.respawn();
        },

        clientActivate: function() {
            this.connect('clientRespawn', function() {
                GameManager.getSingleton().placePlayer(this);
            });
        },
    },

    setup: function(plugins) {
        plugins = defaultValue(plugins, []);

        registerEntityClass(bakePlugins(LogicEntity, plugins.concat([{
            _class: 'GameManager',

            teamData: new StateJSON(),

            activate: function() {
                this.addTag('game_manager');

                this.teams = {};
            },

            getPlayers: function() {
                var players = [];
                forEach(values(this.teams), function(team) {
                    players = players.concat(team.playerList);
                });
                return players;
            },

            startGame: function() {
                var players = this.getPlayers();

                // Clear teams
                forEach(values(this.teams), function(team) {
                    team.score = 0;
                    team.playerList = [];
                });

                // Place players in teams randomly
                while (players.length > 0) {
                    var player = players.splice(Math.floor(Math.random()*players.length), 1)[0];
                    this.pickTeam(player, false); // Pick teams with no syncing until the end
                };

                this.syncTeamData();

                forEach(this.getPlayers(), function(player) {
                    player.respawn();
                });

                this.emit('startGame');

                this.gameRunning = true;
            },

            endGame: function() {
                this.gameRunning = false;

                this.emit('endGame'); // Usually you want to connect something here to run this.startGame, but see intermission plugin
            },

            //! @param _name The name of the team
            //! @param setup Additional setup for players on this team. Done once when they join
            //!              the team. For example, you can set the player model here.
            registerTeams: function(data) {
                forEach(data, function(team) {
                    this.teams[team._name] = {
                        _name: team._name,
                        playerList: [],
                        playerSetup: team.setup,
                        score: 0,
                        flagModelName: team.flagModelName ? team.flagModelName : 'tree',
                    };
                }, this);

                this.emit('postRegisterTeams');

                this.startGame();
            },

            syncTeamData: function() {
                if (!this.deactivated) { // We are called during deactivation process, as players leave
                    this.teamData = this.teams;
                }

                this.emit('teamDataModified');
            },

            //! Adds player to smallest team
            pickTeam: function(player, sync) {
                var sync = defaultValue(sync, true);

                var smallest = '';
                forEach(items(this.teams), function(pair) {
                    if (smallest === '' || pair[1].playerList.length < this.teams[smallest].playerList.length) {
                        smallest = pair[0];
                    }
                }, this);
                player.team = smallest;
                var team = this.teams[smallest];
                team.playerList.push(player);
                team.playerSetup(player);

                if (sync) {
                    this.syncTeamData();
                }
            },

            leaveTeam: function(player, sync) {
                var sync = defaultValue(sync, true);

                var playerList = this.teams[player.team].playerList;
                playerList.splice(
                    findIdentical(playerList, player),
                    1
                );

                if (sync) {
                    this.syncTeamData();
                }
            },

            //! Assumes the existence of start tags, which are worldmarkers with tags 'start_' + team name.
            placePlayer: function(player) {
                var startTag = 'start_' + player.team;
                var possibles = getEntitiesByTag(startTag);
                if (possibles !== null && possibles.length > 0) {
                    var start = possibles[Math.floor(Math.random()*possibles.length)];
                    if (start) {
                        start.placeEntity(player);
                        return;
                    }
                }
                log(WARNING, format('player start not found ("{0}"), placing player elsewhere...', startTag));
                player.position = [512, 512, 571];
            },

            adjustScore: function(teamName, diff) {
                this.teams[teamName].score += diff;

                this.syncTeamData();
            },

            getScoreboardText: function() {
                var data = [];
                forEach(keys(this.teamData), function (teamName) {
                    data.push([-1, ' << ' + teamName + ' >>  ' + this.teamData[teamName].score + ' points']);
                    forEach(values(this.teamData[teamName].playerList), function (uniqueId) {
                        data.push([uniqueId, getEntity(uniqueId)._name + " -"]);
                    });
                }, this);
                return data;
            },
        }])));

        if (Global.SERVER) {
            newEntity('GameManager');
        }
    },

    getSingleton: function() {
        if (this.singleton === undefined) {
            this.singleton = getEntityByTag('game_manager');
        }

        return this.singleton;
    },

    getScoreboardText: function() {
        return GameManager.getSingleton().getScoreboardText();
    },

    managerPlugins: {
        messages: {
            serverMessage: new StateJSON({ hasHistory: false }),

            HUDMessages: [],

            addHUDMessage: function(text, color, duration, size) {
                size = defaultValue(size, 1);

                var kwargs;
                if (typeof text === 'object') {
                    kwargs = text;
                } else {
                    kwargs = { text: text, color: color, duration: duration, size: size };
                }

                if (Global.SERVER) {
                    this.serverMessage = kwargs;
                } else {
                    this.clearHUDMessages(); // XXX: only 1 for now
                    this.HUDMessages.push(kwargs);
                }
            },

            clearHUDMessages: function() {
                this.HUDMessages = [];
            },

            clientActivate: function() {
                this.connect('client_onModify_serverMessage', function(kwargs) {
                    this.addHUDMessage(kwargs);
                });
            },

            clientAct: function(seconds) {
                this.HUDMessages = filter(function(msg) {
                    var size = msg.duration >= 0.5 ? msg.size : msg.size*Math.pow(msg.duration*2, 2);
                    CAPI.showHUDText(msg.text, 0.5, 0.1, size, msg.color);
                    msg.duration -= seconds;
                    return msg.duration > 0;
                }, this.HUDMessages);
            },
        },

        limitGameTime: {
            MAX_TIME: 10*60, // 10 minutes
            activate: function() {
                this.connect('startGame', function() {
                    this.timeLeft = this.MAX_TIME;
                });
            },

            act: function(seconds) {
                if (!this.gameRunning) return;

                this.timeLeft -= seconds;
                if (this.timeLeft <= 0) {
                    this.endGame();
                }
            },
        },

        limitGameScore: {
            MAX_SCORE: 10,
            activate: function() {
                this.connect('teamDataModified', function () {
                    if (!this.gameRunning) return;
                    forEach(values(this.teams), function(team) {
                        if (team.score >= this.MAX_SCORE) {
                            this.endGame();
                        }
                    }, this);
                });
            },
        },

        intermission: {
            activate: function() {
                this.connect('endGame', function() {
                    // Decide winner
                    var maxScore = undefined;
                    var minScore = undefined;
                    forEach(values(this.teams), function(team) {
                        maxScore = defaultValue(maxScore, team.score);
                        minScore = defaultValue(minScore, team.score);
                        maxScore = Math.max(team.score, maxScore);
                        minScore = Math.min(team.score, minScore);
                    }, this);

                    var tie = (maxScore === minScore);

                    var players = this.getPlayers();
                    forEach(players, function(player) {
                        player.canMove = false;
                        var msg, sound;
                        if (!tie) {
                            if (this.teams[player.team].score === maxScore) {
                                msg = 'You won!';
                                sound = '0ad/thunder_10.ogg';
                                player.animation = ANIM_WIN|ANIM_LOOP;
                            } else {
                                msg = 'You lost...';
                                sound = '0ad/windgust_11.ogg';
                                player.animation = ANIM_LOSE|ANIM_LOOP;
                            }
                        } else {
                            msg = 'The game is a tie.';
                            sound = '0ad/windgust_11.ogg';
                            player.animation = ANIM_IDLE|ANIM_LOOP;
                        }
                        MessageSystem.showClientMessage(player, 'Game finished', msg);
                        Sound.play(sound, new Vector3(0, 0, 0), 0, player.clientNumber);
                    }, this);

                    this.intermissionDelayLeft = 10.0;
                });
            },

            act: function(seconds) {
                if (this.intermissionDelayLeft) {
                    this.intermissionDelayLeft -= seconds;
                    if (this.intermissionDelayLeft <= 0) {
                        this.intermissionDelayLeft = null;

                        // Unfreeze players
                        var players = this.getPlayers();
                        forEach(players, function(player) {
                            player.canMove = true;
                        });

                        this.startGame();
                    }
                }
            }
        },

        //! Balance the teams.
        //! Once per second, move one player. May take several seconds to completely balance if large disparities exist
        balancer: {
            activate: function() {
                this.balancerTimer = new RepeatingTimer(1.0);
            },

            act: function(seconds) {
                if (this.balancerTimer.tick(seconds)) {
                    var numTeams = keys(this.teams).length;
                    var totalPlayers = sum(
                        map(
                            function(teamData) { return teamData.playerList.length; },
                            values(this.teams)
                        )
                    );
                    var expectedPlayers = totalPlayers / numTeams;

                    var needsReduce = filter(
                        function(teamData) { return teamData.playerList.length > expectedPlayers + 1.1/numTeams; }, // 1.1: float safety
                        values(this.teams)
                    );

                    var changed = false;
                    forEach(needsReduce, function(teamData) {
                        var player = teamData.playerList[0];
                        this.leaveTeam(player, false);
                        this.pickTeam(player, false);
                        player.respawn();
                        changed = true;
                    }, this);
                    this.syncTeamData(); // Do all syncing at the end
                    if (changed && this.addHUDMessage) {
                        this.addHUDMessage('Balanced the teams', 0xFFFFFF, 2.0, 0.66);
                    }
                }
            },
        },
    },
};

Map.preloadSound('0ad/thunder_10.ogg');
Map.preloadSound('0ad/windgust_11.ogg');

