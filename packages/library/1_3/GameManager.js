
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

        init: function() {
            this.team = ''; // empty until set
        },

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

        registerEntityClass(bakePlugins(LogicEntity, [{
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
                        flagModelName: team.flagModelName ? team.flagModelName : '',
                        kwargs: defaultValue(team.kwargs, {}),
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

                this.setPlayerTeam(player, smallest, sync);
            },

            setPlayerTeam: function(player, team, sync) {
                if (player.team) {
                    this.leaveTeam(player, sync);
                }

                player.team = team;
                var team = this.teams[team];
                team.playerList.push(player);
                team.playerSetup(player);
                player.respawn();

                if (sync) {
                    this.syncTeamData();
                }
            },

            leaveTeam: function(player, sync) {
                var sync = defaultValue(sync, true);

                var playerList = this.teams[player.team].playerList;
                var index = findIdentical(playerList, player);
                if (index >= 0) {
                    playerList.splice(
                        index,
                        1
                    );

                    if (sync) {
                        this.syncTeamData();
                    }
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

            clientActivate: function() {
                this.connect('client_onModify_teamData', function(value) {
                    if (this.teamData && value && getPlayerEntity()) {
                        var playerTeam = getPlayerEntity().team;
                        if (value[playerTeam].score > this.teamData[playerTeam].score) {
                            Sound.play('0ad/alarmvictory_1.ogg');
                        }
                    }
                });
            },

            setLocalAnimation: function() { }, // Just so it can fake being animated by actions
        }].concat(plugins)));

        if (Global.SERVER) {
            newEntity('GameManager');
        }
    },

    getSingleton: function() {
        if (this.singleton === undefined) {
            this.singleton = getEntitiesByClass('GameManager')[0];
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

            addHUDMessage: function(text, color, duration, size, x, y) {
                var kwargs;
                if (typeof text === 'object') {
                    kwargs = text;
                } else {
                    kwargs = { text: text, color: color, duration: duration, size: size, x: x, y: y };
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

                this.renderingHashHint = 0; // Used for rendering entities without fpsents
            },

            clientAct: function(seconds) {
                this.HUDMessages = filter(function(msg) {
                    var size = msg.size ? msg.size : 1.0;
                    size = msg.duration >= 0.5 ? size : size*Math.pow(msg.duration*2, 2);
                    CAPI.showHUDText(msg.text, msg.x ? msg.x : 0.5, msg.y ? msg.y : 0.2, size, msg.color);
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
                    var relevantTeams = filter(function(team) {
                        return !team.kwargs.ignoreForBalancing;
                    }, values(this.teams));

                    var numTeams = keys(relevantTeams).length;
                    var totalPlayers = sum(
                        map(
                            function(teamData) { return teamData.playerList.length; },
                            relevantTeams
                        )
                    );
                    var expectedPlayers = totalPlayers / numTeams;

                    var needsReduce = filter(
                        function(teamData) { return teamData.playerList.length > expectedPlayers + 1; },
                        relevantTeams
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

        highScoreTable: {
            plugin: {
                highScoreData: new StateJSON(),

                init: function() {
                    this.highScoreData = {
                        biggerScoresAreBetter: true,
                        maxScores: 6,
                        scores: [],
                        unit: 'points',
                        oneScorePerPlayer: true,
                    };
                },

                addPossibleHighScore: function(playerName, score) {
                    // Check if a new high score
                    var item = { playerName: playerName, score: score };
                    var biggerScoresAreBetter = this.highScoreData.biggerScoresAreBetter;
                    var getBetterScore = function(a, b) {
                        if (a === null) return parseFloat(b);
                        if (b === null) return parseFloat(a);
                        a = parseFloat(a);
                        b = parseFloat(b);
                        return biggerScoresAreBetter ? Math.max(a, b) : Math.min(a, b);
                    };
                    var add = this.highScoreData.scores.length < this.highScoreData.maxScores;
                    var bestOldScore = null; // Used if oneScorePerPlayer
                    if (!add || this.highScoreData.oneScorePerPlayer) {
                        forEach(this.highScoreData.scores, function (oldItem) {
                            if (getBetterScore(oldItem.score, item.score) == item.score) {
                                add = true;
                            }

                            if (oldItem.playerName === playerName) {
                                bestOldScore = bestOldScore === null ? oldItem.score : getBetterScore(bestOldScore, oldItem.score);
                            }
                        });
                    }
                    var scores = this.highScoreData.scores;
                    if (this.highScoreData.oneScorePerPlayer && bestOldScore !== null) {
                        add = add && (score == getBetterScore(score, bestOldScore));
                        if (add) { // Remove old score
                            scores = filter(function(oldItem) { return oldItem.playerName !== playerName; }, scores);
                        }
                    }
                    if (!add) return false;

                    // Add and update list
                    scores.push(item);
                    scores.sort(function(a, b) { return (getBetterScore(b.score, a.score) == a.score) ? -1 : 1; });
                    scores = scores.slice(0, this.highScoreData.maxScores);

                    // Push update
                    var data = this.highScoreData;
                    data.scores = scores;
                    this.highScoreData = data;

                    return true;
                },

                clientAct: function() {
                    if (this.showHighScores) {
                        CAPI.showHUDRect(0.5, 0.5, -0.8, -0.8, 0x225899);
                        CAPI.showHUDRect(0.5, 0.5, -0.75, -0.75, 0x000000);
                        CAPI.showHUDText('High Scores', 0.5, 0.19, 0.75, 0xFFEEDD);

                        var y = 0.3;
                        var spacing = Math.min(0.5/(this.highScoreData.maxScores-1), 0.1);
                        forEach(this.highScoreData.scores, function(item) {
                            CAPI.showHUDText(item.score + ' ' + this.highScoreData.unit + '  :  ' + item.playerName, 0.5, y, 0.5, 0xDDDDDD);
                            y += spacing;
                        }, this);
                    }
                },
            },

            actionKey: function(index, down) {
                if (index === 0) {
                    GameManager.getSingleton().showHighScores = down;
                }
            },
        },

        radar: {
            radar: {
                centerX: 0.90,
                centerY: 0.15,
                radius: 0.1,
                image: 'packages/hud/radar/gk_level_2_rada_01_frame.png',
                elements: [],
            },

            clientAct: function() {
                // Radar
                if (Global.gameHUD) {
                    this.radar = merge(this.radar, Global.gameHUD.getRadarParams());
                } else {
                    CAPI.showHUDRect(this.radar.centerX, this.radar.centerY, -0.98*this.radar.radius*2, -0.98*this.radar.radius*2, 0xF0F0F0);
                }
                CAPI.showHUDImage(this.radar.image, this.radar.centerX, this.radar.centerY, this.radar.radius*2, this.radar.radius*2);

                // Elements
                var player = getPlayerEntity();
                forEach(this.radar.elements, function(element) {
                    if (element.entity && element.entity.deactivated) return; // Added last frame, perhaps, and just removed
                    var direction = element.target.subNew(player.position);
                    direction.z = 0;
                    var distance = Math.pow(direction.magnitude(), 0.66)/150;
                    var yaw = direction.toYawPitch().yaw - player.yaw;
                    yaw = Math.PI*(yaw - 90)/180;
                    CAPI.showHUDImage(element.icon,
                        this.radar.centerX + clamp(distance*this.radar.radius*Math.cos(yaw), -this.radar.radius*0.9, this.radar.radius*0.9)/Global.aspectRatio,
                        this.radar.centerY + clamp(distance*this.radar.radius*Math.sin(yaw), -this.radar.radius*0.9, this.radar.radius*0.9),
                        0.020, 0.02
                    );
                }, this);
                this.radar.elements = [];
            },

            drawRadarElement: function(target, icon, entity) {
                this.radar.elements.push({ target: target, icon: icon, entity: entity });
            },
        },

        //! Manages a list of events that will happen in the future, and are optionally repeating.
        //! In general this is more efficient than using a RepeatingTimer in an act()ing entity.
        eventList: {
            activate: function() {
                this.eventManager = {
                    // A list of secondsBefore, secondsBetween, and a func, sorted by secondsBefore.
                    // func can return false, to not repeat (it never repeats anyhow without secondsBetween),
                    // or a number, which is added to the secondsBefore for the next round (i.e., an extra delay)
                    list: [],

                    sortList: function() {
                        this.list.sort(function(a, b) { return a.secondsBefore - b.secondsBefore; })
                    },

                    add: function(kwargs, toReplace) {
                        if (toReplace) {
                            toReplace.abort = true;
                        }
                        kwargs.secondsBefore = defaultValue(kwargs.secondsBefore, 0); // Not repeating, no time between
                        kwargs.secondsBetween = defaultValue(kwargs.secondsBetween, -1); // Not repeating, no time between
                        kwargs.abort = false;
                        this.list.push(kwargs);
                        this.sortList();
                        return kwargs;
                    },
                };
            },

            clientActivate: function() {
                GameManager.managerPlugins.eventList.activate.call(this);
            },

            act: function(seconds) {
                var changed = false;
                var newList = filter(function(item) {
                    if (item.abort || (item.entity && item.entity.deactivated)) {
                        changed = true;
                        return false;
                    }
                    item.secondsBefore -= seconds;
                    if (item.secondsBefore <= 0) {
                        changed = true;
                        var more = item.func();
                        if (item.secondsBetween >= 0 && more !== false) {
                            item.secondsBefore = item.secondsBetween + (more ? more : 0);
                            return true;
                        } else {
                            return false;
                        }
                    } else {
                        return true;
                    }
                }, this.eventManager.list, this);

                if (changed) {
                    this.eventManager.list = newList;
                    this.eventManager.sortList();
                }
            },

            clientAct: function() {
                GameManager.managerPlugins.eventList.act.apply(this, arguments);
            },
        },
    },
};

Map.preloadSound('0ad/thunder_10.ogg');
Map.preloadSound('0ad/windgust_11.ogg');

