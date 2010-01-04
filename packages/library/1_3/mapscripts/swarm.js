// (C) 2009 Alon 'Kripken' Zakai

Library.include('library/1_3/');

Library.include('library/' + Global.LIBRARY_VERSION + '/__CorePatches');
Library.include('library/' + Global.LIBRARY_VERSION + '/Plugins');
Library.include('library/' + Global.LIBRARY_VERSION + '/Firing');
Library.include('library/' + Global.LIBRARY_VERSION + '/Health');
Library.include('library/' + Global.LIBRARY_VERSION + '/GameManager');
Library.include('library/' + Global.LIBRARY_VERSION + '/modes/Teamwork');
Library.include('library/' + Global.LIBRARY_VERSION + '/World');
Library.include('library/' + Global.LIBRARY_VERSION + '/CutScenes');
Library.include('library/' + Global.LIBRARY_VERSION + '/Swarm');
Library.include('library/' + Global.LIBRARY_VERSION + '/CustomEffect');
Library.include('library/' + Global.LIBRARY_VERSION + '/guns/Rocket');
Library.include('library/' + Global.LIBRARY_VERSION + '/guns/Chaingun');
Library.include('library/' + Global.LIBRARY_VERSION + '/guns/Insta');
Library.include('library/' + Global.LIBRARY_VERSION + '/guns/Stunball');
Library.include('library/' + Global.LIBRARY_VERSION + '/mapelements/FlickeringLights');
Library.include('library/' + Global.LIBRARY_VERSION + '/mapelements/Cannons');
Library.include('library/' + Global.LIBRARY_VERSION + '/mapelements/PlotTriggers');
Library.include('library/' + Global.LIBRARY_VERSION + '/mapelements/Pickups');
//Library.include('library/' + Global.LIBRARY_VERSION + '/Octree');

// Default materials, etc.

Library.include('library/MapDefaults');

// Textures

Library.include('textures/gk/swarm/');

// Map settings

Map.fogColor(45, 30, 10);
Map.fog(1500);
Map.loadSky("skyboxes/gk2/nnu_sb01");
Map.skylight(0, 0, 0);
Map.ambient(1);
Map.shadowmapAmbient("0x101010");
Map.shadowmapAngle(300);

//// Cannons

SwarmRocketCannonPlugin = {
    init: function() {
        this.autoTargetSearchRadius = 465;
    },

    activate: function() {
        this.autoTargetParams = {
            rotateSpeed: 60,
            minPitch: -45,
            maxPitch: 45,
            eyeHeight: 7, // Where we shoot from
/*
            effectiveSearchRadius: function(actor, target, distance) {
                var direction = target.position.subNew(actor.position).toYawPitch();
                var yawDiff = Math.abs( normalizeAngle(direction.yaw, actor.autoTargetYaw) - actor.autoTargetYaw );
                var pitchDiff = Math.abs( normalizeAngle(direction.pitch, actor.autoTargetPitch) - actor.autoTargetPitch );
                if (yawDiff + pitchDiff > 45) return 500;
                return distance;
            },
*/
            seekDelay: 3.0, // How often to search for an entity to target
            fullSyncRate: 1.0/5,
            interpolateRate: 1/30,
        };
        this.botFiringParams = {
            firingDelay: 3,
            triggerFingerDelay: 0.33,
        };
    },
};

makeCannon('RocketCannon', RocketGun.extend({ projectileClass: Rocket.extend({ explosionPower: 25, speed: 150, accuracy: 0.5, timeLeft: 8.0, gravityFactor: 0 }) }), [Projectiles.plugin, SwarmRocketCannonPlugin]);

makeCannon('UpsidedownRocketCannon', RocketGun.extend({ projectileClass: Rocket.extend({ explosionPower: 25, speed: 150, accuracy: 0.5, timeLeft: 8.0, gravityFactor: 0 }) }), [Projectiles.plugin, UpsidedownCannonPlugin, SwarmRocketCannonPlugin]);

SwarmInstaCannonPlugin = {
    init: function() {
        this.autoTargetSearchRadius = 150;
    },

    activate: function() {
        this.autoTargetParams = {
            rotateSpeed: 60,
            minPitch: -45,
            maxPitch: 45,
            eyeHeight: 7, // Where we shoot from
            seekDelay: 3.0, // How often to search for an entity to target
            fullSyncRate: 1.0/5,
            interpolateRate: 1/30,
        };
    },
};
makeCannon('InstaCannon', InstaGun.extend({ damage: 25 }), [SwarmInstaCannonPlugin]);

makeCannon('UpsidedownInstaCannon', InstaGun.extend({ damage: 25 }), [UpsidedownCannonPlugin, SwarmInstaCannonPlugin]);
makeCannon('UpsidedownCannon', InstaGun.extend({ damage: 25 }), [UpsidedownCannonPlugin, SwarmInstaCannonPlugin]);


//// Player class

PowerfulRocket = Rocket.extend({
    explosionPower: 66.0,
    gravityFactor: 0.0,
});

PowerfulRocketGun = RocketGun.extend({
    projectileClass: PowerfulRocket,
});

playerChaingun = Firing.registerGun(new Chaingun(), 'Chaingun');
playerRocketLauncher = Firing.registerGun(new PowerfulRocketGun(), 'Rocket Launcher');

registerEntityClass(
    bakePlugins(
        Player,
        [
            Firing.plugins.protocol,
            Firing.plugins.player,
            Health.plugin,
            GameManager.playerPlugin,
            Projectiles.plugin,
            Chaingun.plugin,
            Character.plugins.effectiveCameraHeight,
            Character.plugins.footsteps,
            StunballVictimPlugin,
            {
                _class: "GamePlayer",
                HUDModelOffset: new Vector3(0, 0, 1.5), // Adjust for higher eyeheight&aboveeye with 115% model

                init: function() {
                    this.modelName = ''; // Start invisible, until spawned
                    this.canMove = false; // Start immobile, until spawned
                    this.gunIndexes = [playerChaingun, playerRocketLauncher];
                    this.currGunIndex = playerChaingun;
                    this.maxHealth = 250;

                    // 115% size model
                    this.eyeHeight = 15.35;
                    this.aboveEye = 1.15;
                    this.radius = 3.5;

                    this.position = [1631.68,326.32,425.07]; // Place there immediately, for nicer spawning the first time
                },

                activate: function() {
                    this.movementSpeed = 75;
                },

                getColor: function() {
                    return this.HUDModelName.split('/').pop();
                },

                clientActivate: function() {
                    this.gunAmmos[playerChaingun] = null;
                    this.gunAmmos[playerRocketLauncher] = 10;
                },
//                clientAct: function() {
//                    this.pitch = clamp(this.pitch, -45, 90);
//                },
            }
        ]
    )
);

//// Game props etc.

//! Shows a particle effect until triggered (so players know what is left)
BigCavernDoorTrigger = registerEntityClass(bakePlugins(PlotTrigger, [{
    _class: 'BigCavernTrigger',
    shouldAct: true,

    renderDynamic: null,

    clientActivate: function(seconds) {
        this.timer = new RepeatingTimer(1/10);

        this.connect('client_onModify_state', function(state) {
            if (state === 'open') {
                var player = getPlayerEntity();
                if (!player) return;
                if (player.position.isCloseTo(this.position, 50)) {
                    player.sufferStun(100);
                    Sound.play('0ad/thunder_10.ogg', player.center);
                    Effect.lightning(player.center, this.center, 0.5, 0xFF9933, 1.0);
                }
            }
        });
    },

    clientAct: function(seconds) {
        if (this.state === 'closed' && this.timer.tick(seconds)) {
            var position = this.position.copy();
            position.z += this.collisionRadiusHeight*1.5;
            Effect.splash(PARTICLE.SPARK, 10, 1/5, position, 0x4477EE, 1.0, 70, 1);
        }
    },
}]));

KeycardBase = registerEntityClass(bakePlugins(RecursivePlotTrigger, [{
    _class: 'KeycardBase',

    useRenderDynamicTest: true,

    init: function() {
        this.stateModelNames = ['gk/swarm/keycard/base', 'gk/swarm/keycard/base'];
    },

    renderDynamic: function() {
        if (this.state === 'open') {
            if (!this.childModelName) {
                var child = filter(
                    function(trigger) {
                        return trigger.parentEntity === this.uniqueId;
                    },
                    getEntitiesByClass(this.childEntityClass),
                    this
                )[0];
                this.childModelName = child.stateModelNames.get(0);
            }

            var o = this.position;
            var flags = MODEL.LIGHT;
            var args = [GameManager.getSingleton(), this.childModelName, ANIM_IDLE, o.x, o.y-1.5, o.z+18, 180, 0, 180, flags, 0];
            GameManager.getSingleton().renderingHashHint = 0;
            CAPI.renderModel2.apply(null, args);
        }
    },
}]));

//! Final boss battle trigger
BigBossTrigger = registerEntityClass(bakePlugins(PlotTrigger, [{
    _class: 'BigBossTrigger',

    renderDynamic: null,

    activate: function(seconds) {
        this.openTime = -1;

        this.connect('onModify_state', function(state) {
            if (state === 'open') {
                this.openTime = Global.time;
            }
        });

        GameManager.getSingleton().eventManager.add({
            secondsBefore: 0,
            secondsBetween: 1,
            func: bind(function() {
                if (this.state === 'open' && Global.time - this.openTime > 30.0) {
                    this.state = 'closed'; // Players must reopen!
                }
            }, this),
            entity: this,
        });
    },

    clientActivate: function(seconds) {
        GameManager.getSingleton().eventManager.add({
            secondsBefore: 0,
            secondsBetween: 1/10,
            func: bind(function() {
                if (this.state === 'closed') {
                    Effect.splash(PARTICLE.SPARK, 10, 1/5, this.center, 0xEE7744, 1.0, 70, 1);
                }
            }, this),
            entity: this,
        });

        this.connect('client_onModify_state', function(state) {
            if (state === 'open' && this.state === 'closed') {
                Sound.play('0ad/alarmcreatemiltaryfoot_1.ogg', this.position.copy());
            }
        });
    },
}]));

BigBossHeart = registerEntityClass(bakePlugins(RecursivePlotTrigger, [{
    _class: 'BigBossHeart',
    shouldAct: true,

    maxHealth: 100,
    health: new StateInteger(),

    init: function() {
        this.states = ['closed'];
        this.stateModelNames = [''];
        this.radius = 0;
    },

    activate: function() {
        this.health = this.maxHealth;

        GameManager.getSingleton().eventManager.add({
            secondsBefore: 0,
            secondsBetween: 1,
            func: bind(function() {
                if (this.health < this.maxHealth) {
                    log(ERROR, this.health);
                    var children = filter(function(child) { return child.state === 'closed'; }, getEntitiesByClass('BigBossTrigger'));
                    this.health = Math.min(this.health + 10*children.length, this.maxHealth);
                }
            }, this),
            entity: this,
        });

        this.connect('onModify_health', function(health) {
            if (health <= 0) {
                // Stop attackers in last room
                GameManager.getSingleton().disableAutoTargeting = true;
                forEach(getEntitiesByClass('SwarmBugSpawner'), function(spawner) {
                    spawner.reactivateDelay = 10000;
                    spawner.deactivateSpawner();
                });

                // Restart map / TODO: Move to next map
                GameManager.getSingleton().eventManager.add({
                    secondsBefore: 21, // XXX: Coordinate with player cutscene!
                    func: function() {
                        CAPI.signalComponent('MapControl', 'restart');
                    },
                    entity: this,
                });

            }
        });
    },

    clientActivate: function(seconds) {
        GameManager.getSingleton().eventManager.add({
            secondsBefore: 0,
            secondsBetween: 0,
            func: bind(function() {
                var factor = this.health/this.maxHealth;
                if (factor < 0.9) {
                    var color = 255;//clamp(255*factor, 0, 255);
                    Effect.splash(PARTICLE.SPARK, 10, factor*1/2, this.center, color + (color << 8) + (color << 16), 1.0, 70, 1);
                    return factor*1/3;
                } else {
                    return 1/2;
                }
            }, this),
            entity: this,
        });

        this.connect('client_onModify_health', function(health) {
            if (health <= 0) {
                GameManager.getSingleton().showEndTitles(this.position.addNew(new Vector3(0, 0, this.collisionRadiusHeight)));
            }
        });
    },

    clientAct: function() {
        if (getPlayerEntity() && this.position.isCloseTo(getPlayerEntity().position, 512)) {
            var factors = Global.gameHUD.calcFactors();
            CAPI.showHUDImage(
                'packages/gamehud/gk/swarm/boss/GK_Boss_IMG_Overlay_256x256.png',
                (1/Global.aspectRatio)*factors.x*128/Global.screenWidth, factors.y*128/Global.screenHeight,
                factors.x*256/Global.screenWidth, factors.y*256/Global.screenHeight
            );

            var raw = Math.floor(10*this.health/this.maxHealth);

            CAPI.showHUDImage(
                'packages/gamehud/gk/swarm/boss/GK_Boss_IMG_Overlay_64x256_HP_0' + raw + '.png',
                factors.x*((1/Global.aspectRatio)*128+32)/Global.screenWidth, factors.y*128/Global.screenHeight,
                factors.x*64/Global.screenWidth, factors.y*256/Global.screenHeight
            );
        }
    },

    sufferDamage: function(kwargs) {
        if (kwargs.damage >= 0) {
            this.health -= Math.max(kwargs.damage, 10);
        }
    },
}]));

// Monkeypatch World.isColliding so we can chaingun the heart
var getCollidableEntitiesOld = World.getCollidableEntities;
World.getCollidableEntities = function() {
    return getCollidableEntitiesOld().concat(getEntitiesByClass('BigBossHeart'));
}

//// Application

ApplicationManager.setApplicationClass(Application.extend({
    _class: "GameApplication",

    getPcClass: function() {
        return "GamePlayer";
    },

    // Replace this with appropriate behaviour for when a player falls of the map
    clientOnEntityOffMap: Health.dieIfOffMap,

    getScoreboardText: GameManager.getScoreboardText,

    clientClick: function(button, down, position) {
//        log(ERROR, "MAT: " + World.getMaterial(position));
        Firing.clientClick.apply(Firing, arguments);
    },

    actionKey: function(index, down) {
        if (!down) return;
        if (index === 0) { // action key 0: Help
            UserInterface.showMessage("Middle mouse: Cycle weapons");
        } else if (index >= 1 && index <= 4) { // Or: bind MOUSE2 actionkey1
            var gunIndexes = getPlayerEntity().gunIndexes.asArray();
            if (findIdentical(gunIndexes, index-1) >= 0) {
                getPlayerEntity().currGunIndex = index-1;
            }
        } else if (index === 7) { // die
            getPlayerEntity().health = 0;
        }
    },

    performJump: function(down) {
        if (down) {
            var player = getPlayerEntity();
            if (player.physicalState >= PHYSICALSTATE.SLOPE || player.inWater) {
                Sound.play('gk/jump2.ogg', player.position);
                player.velocity.z += 125;
            }
        }
    },
}));

//// Game manager

SwarmPickups = {
    base: {
        collisionRadiusWidth: 3,
        collisionRadiusHeight: 3,
        secondsLeft: 10.0,
        useRenderDynamicTest: true,
    },
};

SwarmPickups['h'] = merge(SwarmPickups['base'], {
    renderDynamic: function() {
        var o = this.position;
        var flags = MODEL.LIGHT | MODEL.CULL_VFC | MODEL.CULL_DIST;
        var args = [GameManager.getSingleton(), 'gk/swarm/health', ANIM_IDLE, o.x, o.y, o.z, 0, 0, 0, flags, 0];
        GameManager.getSingleton().renderingHashHint = 0;
        CAPI.renderModel2.apply(null, args);
    },
    doPickup: function(player) {
        player.health = Math.min(player.maxHealth, player.health + 50);
    },
});

SwarmPickups['r'] = merge(SwarmPickups['base'], {
    renderDynamic: function() {
        var o = this.position;
        var flags = MODEL.LIGHT | MODEL.CULL_VFC | MODEL.CULL_DIST;
        var args = [GameManager.getSingleton(), 'gk/swarm/munition', ANIM_IDLE, o.x, o.y, o.z, 0, 0, 0, flags, 0];
        GameManager.getSingleton().renderingHashHint = 0;
        CAPI.renderModel2.apply(null, args);
    },
    doPickup: function(player) {
        player.gunAmmos[playerRocketLauncher] += 5;
    },
});

SwarmPickups['hp'] = merge(SwarmPickups['h'], {
    secondsLeft: 60.0,
});

SwarmPickups['rp'] = merge(SwarmPickups['r'], {
    secondsLeft: 60.0,
});

CutSceneWithBackgroundText = CutScenes.BaseAction.extend({
    doExecute: function() {
        this.oldShowHUDImage('packages/gamehud/gk/swarm/cutscene_overlay.png', 0.5, 0.5, Math.max(Global.aspectRatio, 1), Math.min(Global.aspectRatio, 1));

        return this._super.apply(this, arguments);
    },

/*
    showSubtitleBackground: function() {
        var factors = Global.gameHUD.calcFactors();
        this.oldShowHUDImage('packages/gamehud/gk/swarm/textbox.png', 0.5, 0.9, factors.x*800/Global.screenWidth, factors.y*128/Global.screenHeight);
    },
*/
});

Projectiles.serverside = false;

GameManager.setup([
    GameManager.managerPlugins.messages,
    GameManager.managerPlugins.radar,
    GameManager.managerPlugins.eventList,
//    TeamworkMode.managerPlugin,
    Pickups.managerPlugin,
    Projectiles.plugin,
    ParallelActionsPlugin,
    {
        pickupTypes: {
            'h': SwarmPickups['h'],
            'r': SwarmPickups['r'],
            'hp': SwarmPickups['hp'],
            'rp': SwarmPickups['rp'],
        },

        activate: function() {
            this.playerColors = { 'red': null, 'yellow': null, 'blue': null };
        },

        clientAct: function() {
            if (!this.shownIntro && getPlayerEntity()) {
                this.showIntro();
                this.shownIntro = true;
            }

            forEach(getEntitiesByClass(Player), function(player) {
                if (player !== getPlayerEntity()) {
                    this.drawRadarElement(player.position, 'packages/hud/radar/' + player.getColor() + '_dot.png', player);
                }
            }, this);
        },

        showIntro: function() {
            if (Global.noCutscenes) return;

            if (isIntensityVersionAtLeast('1.1.3')) {
                Sound.playMusic('music/Pal_Zoltan_Illes__Rolling_Militia.ogg');
            } else {
                log(WARNING, "Cannot play music, need to upgrade the engine");
            }

            var factor = 4/3;

            function renderBug(o, yaw, pitch, roll, startTime) {
                var anim = 50|ANIM_LOOP;
                var flags = MODEL.LIGHT | MODEL.CULL_VFC | MODEL.CULL_DIST | MODEL.DYNSHADOW;
                var args = [GameManager.getSingleton(), 'gk/botter/1', anim, o.x, o.y, o.z, yaw, pitch, roll, flags, startTime];
                GameManager.getSingleton().renderingHashHint = 1;
                CAPI.renderModel2.apply(null, args);
            }

            getPlayerEntity().queueAction(new (CutSceneWithBackgroundText.extend({
                canBeCancelled: true,

                subtitles: [
                    new (CutScenes.Subtitle.extend({
                        start: 0.5*factor, end: 9.0*factor, text: 'You have landed on a hostile planet...',
                    }))(), 
                    new (CutScenes.Subtitle.extend({
                        start: 11.0*factor, end: 19.0*factor, text: 'Others have failed at this mission...',
                    }))(), 
                    new (CutScenes.Subtitle.extend({
                        start: 22.0*factor, end: 29.0*factor, text: 'Now you must go forth and explore...',
                    }))(), 
                ],
            }))(
                [
                    new (CutScenes.SmoothAction.extend({
                        markers: [
                            { position: new Vector3(1379.26,  577.65,  505.52), yaw: 31.09, pitch: -10.90 },
                            { position: new Vector3(1852.22,  545.97,  568.96), yaw: 307.90, pitch: -15.18 },
                            { position: new Vector3(1840.70,  34.74,  527.08), yaw: 230.81, pitch: -12.54 },
                        ],
                        delayAfter: 1.0,
                    }))(),
                    new (CutScenes.SmoothAction.extend(merge(
                        {
                            delayBefore: 1.0,
                            markers: [
                                { position: new Vector3(1021.78,  1305.85,  594.17), yaw: 157.08, pitch: 4.63 },
                                { position: new Vector3(858.32,  1516.53,  676.01), yaw: 143.36, pitch: 18.00 },
                                { position: new Vector3(988.48,  1874.73,  601.13), yaw: 60.72, pitch: 1.54 },
                            ],
                            delayAfter: 1.0,
                        },
                        RenderingCaptureActionPlugin,
                        {
                            renderDynamic: function() {
                                renderBug(new Vector3(1074.87,  1819.10,  572.01), 120, 0, 0, 50);
                            }
                        }
                    )))(),
                    new (CutScenes.SmoothAction.extend(merge(
                        {
                            delayBefore: 1.0,
                            markers: [
                                { position: new Vector3(522.99,  717.87,  577.47), yaw: 19.63, pitch: 1.90 },
                                { position: new Vector3(520.65,  706.02,  586.36), yaw: 185.90, pitch: -11.45 },
                                { position: new Vector3(491.87,  917.46,  576.36), yaw: 116.72, pitch: 1.09 },
                            ],
                        },
                        RenderingCaptureActionPlugin,
                        {
                            renderDynamic: function() {
                                renderBug(new Vector3(570.11,  1005.64,  544.08), 166, 0, 0, 0);
                                renderBug(new Vector3(618.83,  988.90,  557.63), 267, -30, 0, 25);
                                renderBug(new Vector3(474.58,  898.20,  566.14), 269, 0, -45, 50);
                                renderBug(new Vector3(548.00,  630.06,  544.09), 175, 0, 0, 75);
                                renderBug(new Vector3(536.99,  926.66,  544.08), 223, 0, 0, 100);
                            }
                        }
                    )))(),
                ]
            ));

            getPlayerEntity().queueAction(new (Action.extend({
                doStart: function() {
                    GameManager.getSingleton().addHUDMessage({
                        text: "Note: This is a *PREVIEW* version! :)", // "Tip: Press '9' for firstperson"
                        color: 0xFFDDCC,
                        duration: 5,
                        y: 0.333,
                        size: 0.66,
                    });
                },
            }))());
        },

        showEndTitles: function(fireworksOrigin) {
            getPlayerEntity().health = getPlayerEntity().maxHealth; // don't die from final rockets!

            // Fireworks

            var startTime = Global.time;
            for (var i = 0; i < 5; i++) {
                GameManager.getSingleton().eventManager.add({
                    secondsBefore: Math.random(),
                    secondsBetween: 0, // Dynamic
                    func: bind(function() {
                        Sound.play('olpc/MichaelBierylo/sfx_DoorSlam.wav', fireworksOrigin);

                        this.addParallelAction(new CustomEffect.Fireworks({
                            shots: [{
                                position: fireworksOrigin.copy(),
                                velocity: new Vector3(30*(Math.random()-0.5), 30*(Math.random()-0.5), 20 + 30*(Math.random()-0.5)),
                                secondsLeft: 3.0,
                                childMinZ: fireworksOrigin.z - 50,
                                minVelocityZ: -10,
                                childSecondsLeft: 0.6,
                                getColor: function() {
                                    if (this.size === 1.0) return 0xFFEECC;
                                    return Math.floor(Math.random()*255) + (Math.floor(Math.random()*255) << 8) + (Math.floor(Math.random()*255) << 16);
                                },
                                size: 1.0,
                            }],
                        }));

                        if (Global.time - startTime > 25) {
                            return false;
                        } else {
                            return -1;
                        }
                    }, this),
                    entity: this,
                });
            }

            // Cutscene

            getPlayerEntity().queueAction(new (CutSceneWithBackgroundText.extend({
                subtitles: [
                    new (CutScenes.Subtitle.extend({
                        start: 2/3, end: 25.0, text: 'You defeated the computer!',
                    }))(),
                ],
            }))(
                [
                    new (CutScenes.SmoothAction.extend({
                        markers: [
                            { position: new Vector3(2568.11, 1109.05, 856.76), yaw: 59.81, pitch: 4.72 },
                            { position: new Vector3(2521.69, 1161.13, 858.84), yaw: 44.90, pitch: -2.27 },
                            { position: new Vector3(2598.12, 1222.86, 859.70), yaw: 19.18, pitch: -7.27 },
                            { position: new Vector3(2738.24, 1256.46, 862.41), yaw: 335.99, pitch: -3.36 },
                            { position: new Vector3(2767.91, 1167.78, 859.01), yaw: 312.99, pitch: -1.45 },
                        ],
                    }))(),
                ]
            ));
        },
    },
]);

//// Load permanent entities

if (Global.SERVER) { // Run this only on the server - not the clients
    var entities = CAPI.readFile(Global.entitiesFile ? Global.entitiesFile : "base/GK_Swarm_data/entities.json");
    loadEntities(entities);

    GameManager.getSingleton().registerTeams([
        {
            _name: 'red',
            setup: function(player) {
                var manager = GameManager.getSingleton();
                var possibles = filter(function(color) {
                    return !manager.playerColors[color] || manager.playerColors[color].deactivated;
                }, keys(manager.playerColors));
                if (possibles.length === 0) {
                    possibles = keys(manager.playerColors); // All taken, so pick any of them
                }

                var color = Random.choice(possibles);
                manager.playerColors[color] = player;

                player.defaultModelName = 'stromar_1_1/' + color + '/130';
                player.defaultHUDModelName = 'stromar_1_1/hud/' + color;
            },
            flagModelName: 'flag/red',
        },
    ]);
}

//Map.preloadModel('stromar_1_1/red/130');
//Map.preloadModel('stromar/hud/red');
for (var i = 0; i <= 8; i++) {
    Map.preloadModel('gk/botter/' + i);
}

Global.gameHUD = {
    // TODO: handle low resolutions
    getHealthParams: function() {
        var factors = this.calcFactors();
        var w = 42, h = 273;
        var ret = {
            x: (Global.screenWidth-factors.x*w/2)/Global.screenWidth,
            y: factors.y*(282+h/2)/Global.screenHeight,
            w: factors.x*w/Global.screenWidth,
            h: factors.y*h/Global.screenHeight,
        };
        return ret;
    },

    getRadarParams: function() {
        var factors = this.calcFactors();
        var w = 282;
        return {
            centerX: (Global.screenWidth-factors.y*w/2)/Global.screenWidth,
            centerY: factors.y*(w/2)/Global.screenHeight,
            radius: factors.y*(w/2)/Global.screenHeight,
            image: 'packages/gamehud/gk/swarm/gk_pc_rada_bg_image_1.png',
        };
    },

    getFiringParams: function(gun) {
        var factors = this.calcFactors();
        var ammo = getPlayerEntity().gunAmmos[gun.gunIndex];
        var ammo1 = ammo !== null ? Math.floor(ammo/10) : '+';
        var ammo2 = ammo !== null ? Math.floor(ammo%10) : 'N';
        return {
            gun: {
                icon: gun instanceof Chaingun ?
                    'packages/gamehud/gk/swarm/gk_wepon_gun_0.png' :
                    'packages/gamehud/gk/swarm/gk_wepon_missile_0.png',
                x: factors.x*(Global.screenWidth+factors.y*(-282-64-64-64/2))/Global.screenWidth,
                y: factors.y*(5 + 128/2)/Global.screenHeight,
                w: factors.x*64/Global.screenWidth,
                h: factors.y*128/Global.screenHeight,
            },
            ammo1: {
                icon: 'packages/gamehud/gk/swarm/gk_ammo_e1_' + ammo1 + '.png',
                x: factors.x*(Global.screenWidth+factors.y*(-282-64-64/2))/Global.screenWidth,
                y: factors.y*(5 + 128/2)/Global.screenHeight,
                w: factors.x*64/Global.screenWidth,
                h: factors.y*128/Global.screenHeight,
            },
            ammo2: {
                icon: 'packages/gamehud/gk/swarm/gk_ammo_e2_' + ammo2 + '.png',
                x: factors.x*(Global.screenWidth+factors.y*(-282-64/2))/Global.screenWidth,
                y: factors.y*(5 + 128/2)/Global.screenHeight,
                w: factors.x*64/Global.screenWidth,
                h: factors.y*128/Global.screenHeight,
            },
        };
    },

    calcFactors: function() {
        return {
            x: Global.screenWidth/Math.max(Global.screenWidth,Global.screenHeight),
            y: Global.screenHeight/Math.max(Global.screenWidth,Global.screenHeight),
        };
    },

};

//Global.profiling = { interval: 10.0 };

