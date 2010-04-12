
// Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
// This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

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

            leaveTeam: function(player) {
                var playerList = this.teams[player.team].playerList;
                playerList.splice(
                    findIdentical(playerList, player),
                    1
                );

                this.syncTeamData();
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
                            } else {
                                msg = 'You lost...';
                                sound = '0ad/windgust_11.ogg';
                            }
                        } else {
                            msg = 'The game is a tie.';
                            sound = '0ad/windgust_11.ogg';
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
    },
};

Map.preloadSound('0ad/thunder_10.ogg');
Map.preloadSound('0ad/windgust_11.ogg');

