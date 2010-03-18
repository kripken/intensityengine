// (C) 2009 Alon 'Kripken' Zakai

Library.include('library/1_2/__CorePatches');
Library.include('library/1_2/Plugins');
Library.include('library/1_2/GameManager');
Library.include('library/1_2/Health');
Library.include('library/1_2/Chat');
Library.include('library/1_2/Projectiles');
Library.include('library/1_2/World');
Library.include('library/1_2/ai/Steering');
Library.include('library/1_2/mapelements/Portals');
Library.include('library/1_2/mapelements/WorldNotices');

Library.include('./Tutorial');

Library.include('./KarmicKoala');

// Default materials, etc.

Library.include('library/MapDefaults');

// Textures

Library.include('textures/gk/brick3/');
Library.include('textures/gk/concrete3/');
Library.include('textures/gk/deco3/');
Library.include('textures/gk/ground3/');
Library.include('textures/gk/metal3/');
Library.include('textures/gk/morter3/');
Library.include('textures/gk/rock3/');
Library.include('textures/gk/walls3/');
Library.include('textures/gk/wood3/');

Library.include('yo_frankie/');

Library.include('textures/gk/set4/');

// Map settings

Map.fogColor(0, 0, 0);
Map.fog(2500);
//Map.loadSky("skyboxes/blue_orange/1");
Map.loadSky("skyboxes/gk1/GK_CM_SK006");
Map.skylight(50, 50, 50);
Map.ambient(40);
Map.shadowmapAmbient("0x101010");
Map.shadowmapAngle(300);

//// Player class

registerEntityClass(
    bakePlugins(
        Player,
        [
            GameManager.playerPlugin,
            Health.plugin,
            Chat.playerPlugin,
            Tutorial.playerPlugin,
            {
                _class: "GamePlayer",

                init: function() {
                    this.modelName = ''; // start invisible
                },

                activate: function() {
                    this.movementSpeed = 75;
                },
            }
        ]
    )
);

//// NPC

Ball = Projectiles.Projectile.extend({
    radius: 3,
    color: 0xDCBBAA,
    explosionPower: 0,
    speed: 75.0,
    timeLeft: 3.5,

    elasticity: 0.66,

    create: function() {
        this._super.apply(this, arguments);

        this.ignore = this.owner;
        this.bounceFunc = partial(World.bounce, this, this.elasticity);
    },

    tick: function(seconds) {
        this.velocity.z -= World.gravity*seconds;
        return this._super.apply(this, arguments);
    },

    render: function() {
//        Effect.splash(PARTICLE.SMOKE, 2, 0.3, this.position, 0xF0F0F0, 1.2, 50, -20);
        Effect.flame(PARTICLE.FLAME, this.position, 0.5, 0.5, 0xBB8877, 2, 3.0, 100, 0.4, -6);
        Effect.addDynamicLight(this.position, this.radius*4, 0x555555);
    },
});

registerEntityClass(
    bakePlugins(
        Character,
        [
            GameManager.playerPlugin,
            Health.plugin,
            Chat.playerPlugin,
            Projectiles.plugin,
            {
                preferredDist: 25,

                activate: function() {
                    this.targetingTimer = new RepeatingTimer(0.5);
                    this.health = 100; // Never respawned, so need to do it manually, or else bot will look 'dead'
                },

                act: function(seconds) {
                    _name = 'testBOT <<NPC>>';
                    if (this._name !== _name) this._name = _name;

                    if (this.targetingTimer.tick(seconds) && this.actionSystem.isEmpty()) {
                        if (!this.home.isCloseTo(this.position, 80)) {
                            Sound.play("yo_frankie/DeathFlash.wav", this.position);
                            this.position = this.home;
                        }
                        
                        this.targetEntity = null;
                        var pairs = getCloseEntities(this.position.copy(), this.preferredDist*3, Player);
                        if (pairs.length > 0 && this.isValidTarget(pairs[0][0])) {
                            this.targetEntity = pairs[0][0];
                            this.queueAction(new FaceTowardsAction(this.targetEntity));
                        }
                    }

                    this.move = 0;
                    if (this.targetEntity) {
                        if (!this.isValidTarget(this.targetEntity)) {
                            this.targetEntity = null;
                            this.actionSystem.clear();
                            return;
                        }
                        var dist = this.position.subNew(this.targetEntity.position).magnitude();
                        var relDist = Math.abs(dist - this.preferredDist);
                        if (relDist > this.preferredDist/4) {
                            var needCloser = (dist > this.preferredDist);
                            var targetYaw = yawTo(this.position, this.targetEntity.position);
                            targetYaw = normalizeAngle(targetYaw, this.yaw);
                            var diff = Math.abs(targetYaw - this.yaw);
                            if (diff < 30) {
                                this.move = needCloser ? 1 : -1;
                            }
                        }
                    }
                },

                isValidTarget: function(target) {
                    if (target.deactivated) return false;
                    if (!hasLineOfSight(this.getCenter(), target.getCenter())) return false;
                    var dist = this.getCenter().subNew(target.getCenter()).magnitude();
                    return (dist < this.preferredDist * 3)
                },
            },
            {
                _class: "BotPlayer",

                init: function() {
                    this.modelName = 'stromar/yellow';
                },

                activate: function() {
                    this.movementSpeed = 50;
                },
            },
            {
                clientActivate: function() {
                    this.ballTimer = new RepeatingTimer(2.5);
                },

                clientAct: function(seconds) {
                    if (this.ballTimer.tick(seconds)) {
                        this.projectileManager.add( new Ball(
                            this.getCenter().add(new Vector3(0, 0, 2)),
                            new Vector3(Math.random()-0.5, Math.random()-0.5, 3).normalize(),
                            this
                        ));
                    }
                },
            }
        ]
    )
);

//// Application

ApplicationManager.setApplicationClass(Application.extend({
    _class: "GameApplication",

    getPcClass: function() {
        return "GamePlayer";
    },

    clientOnEntityOffMap: Health.dieIfOffMap,

    getScoreboardText: GameManager.getScoreboardText,

    clientClick: function() {
        Tutorial.clientClick.apply(Tutorial, arguments);
        DynaPortals.clientClick.apply(Tutorial, arguments);
    },

    actionKey: Tutorial.actionKey,

    handleTextMessage: Chat.handleTextMessage,
}));

//// Load permanent entities

GameManager.setup([
    GameManager.managerPlugins.messages,
    Tutorial.managerPlugin,
    DynaPortals.managerPlugin,
//    AutoTargeting.managerPlugin,
    {
        clientActivate: function() {
            this.addHUDMessage({
                text: "Welcome to Syntensity!",
                color: 0xDDEEFF,
                duration: 4.0,
                y: 0.25,
            });
        },
    },
]);

if (Global.SERVER) { // Run this only on the server - not the clients
    var entities = CAPI.readFile("./entities.json");
    loadEntities(entities);

    GameManager.getSingleton().registerTeams([
        {
            _name: 'people',
            setup: function(player) {
                player.defaultModelName = 'stromar_1_1/red';
            },
        },
    ]);

    // NPCs
    var npc = newNPC("BotPlayer");
    npc.position = getEntityByTag('bot').position;
    npc.home = npc.position.copy();
}

Map.preloadModel('stromar/red');

