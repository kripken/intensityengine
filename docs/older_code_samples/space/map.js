// (C) 2009 Alon 'Kripken' Zakai

Library.include('library/1_2/__CorePatches');

Library.include('library/1_2/Plugins');
Library.include('library/1_1/Health');
Library.include('library/1_1/GameManager');
Library.include('library/1_2/ZeroG');
Library.include('library/1_2/WorldSignals');
Library.include('library/1_2/MultipartRendering');
Library.include('library/1_2/guns/Rocket');
Library.include('library/1_2/mapelements/Cannons');


// Default materials, etc.

Library.include('library/MapDefaults');

// Textures

Library.include('yo_frankie/');

// Map settings

Map.fogColor(0, 0, 0);
Map.fog(9999);
Map.loadSky("skyboxes/philo/sky3");
Map.skylight(100, 100, 100);
Map.ambient(20);
Map.shadowmapAmbient("0x000000");
Map.shadowmapAngle(300);

//// NPCs

function makeCannon(_name, gunClass, additionalPlugins) {
    additionalPlugins = defaultValue(additionalPlugins, []);

    var CannonGun = bakePlugins(gunClass, [CannonGunPlugin]);
    var cannonGun = new CannonGun();
    Firing.registerGun(cannonGun);
    var plugins = [AutoTargetingPlugin, MultipartRenderingPlugin, MultipartRenderingAutotargetingPlugin, CannonPlugin, Firing.plugins.protocol, BotFiringPlugin, CannonHealthPlugin, { _class: _name, gun: cannonGun }];
    return registerEntityClass( bakePlugins(Mapmodel, plugins.concat(additionalPlugins)) );
}

SIGNAL = {
    LOUD_NOISE: 'LoudNoise',
};

StartledAction = Action.extend({
    canMultiplyQueue: false,

    secondsLeft: 3.0,

    doStart: function() {
        this.originalYaw = this.actor.autoTargetYaw;
        this.originalPitch = this.actor.autoTargetPitch;
    },

    doExecute: function(seconds) {
//        this.actor.autoTargetEntity = null; // Cannot focus

        var timer = this.secondsLeft*3.5;
        this.actor.autoTargetYaw = this.originalYaw + 20*Math.sin(timer*1.5);
        this.actor.autoTargetPitch = this.originalPitch + 10*Math.cos(timer);

        return this._super.apply(this, arguments);
    },

//    doFinish: function() {
//    },
});

makeCannon('RocketCannon', SeekingRocketGun, [Projectiles.plugin, ParallelActionsPlugin, {
    activate: function() {
        this.autoTargetParams = {
            rotateSpeed: 33,
            minPitch: -45,
            maxPitch: 45,
            eyeHeight: 7, // Where we shoot from
            searchRadius: 250,
            effectiveSearchRadius: function(actor, target, distance) {
                var direction = target.position.subNew(actor.position).toYawPitch();
                var yawDiff = Math.abs( normalizeAngle(direction.yaw, actor.autoTargetYaw) - actor.autoTargetYaw );
                var pitchDiff = Math.abs( normalizeAngle(direction.pitch, actor.autoTargetPitch) - actor.autoTargetPitch );
                if (yawDiff + pitchDiff > 45) return 500;
                return distance;
            },
            seekDelay: 1.0, // How often to search for an entity to target
            fullSyncRate: 1.0/10,
        };
    },

    clientActivate: function() {
        // TODO: re-connect when position changes, during editing it is useful. For now, upload the map.
        this.connectedSignal = WorldSignals.connect(SIGNAL.LOUD_NOISE, this.position.copy(), 250, bind(
            function(origin, kwargs) {
                if (!this.autoTargetEntity) {
                    this.addParallelAction(new StartledAction());
                }
            },
            this
        ));
    },

    clientDeactivate: function() {
        WorldSignals.disconnect(this.connectedSignal);
    },
}]);

//// Player class

GamePlayer = registerEntityClass(
    bakePlugins(
        Player,
        [
            ZeroG.plugin,
            Health.plugin,
            GameManager.playerPlugin,
            MultipartRenderingPlugin,
            {
                _class: "GamePlayer",

                accessories: new StateJSON(),

                init: function() {
                    this.accessories = {};
                    this.eyeHeight = 0;
                },

               
                createRenderingArgs: function(yaw, pitch, anim, o, flags, basetime) {
                    var ret = [
                        [this, 'space/ships/1', anim, o.x, o.y, o.z, yaw, pitch, flags, basetime]
                    ];

                    forEach(keys(this.accessories), function(accessory) {
                        ret.push(
                            [this, 'space/ships/1/' + accessory, anim, o.x, o.y, o.z, yaw, pitch, flags, basetime]
                        );
                    }, this);

                    return ret;
                },

                getMultipartFlags: function() {
                    return MODEL.LIGHT | MODEL.CULL_VFC | MODEL.FULLBRIGHT | MODEL.CULL_DIST | MODEL.DYNSHADOW;
                },
            },
        ]
    )
);

//// Application

ApplicationManager.setApplicationClass(Application.extend({
    _class: "GameApplication",

    getPcClass: function() {
        return "GamePlayer";
    },

    // Replace this with appropriate behaviour for when a player falls of the map
    clientOnEntityOffMap: function(entity) {
        entity.position = [600,600,600];
    },

    getScoreboardText: GameManager.getScoreboardText,

    actionKey: function(index, down) {
        if (!down) return;

        var key = '';
        if (index === 0) {
            key = 'weapon_launcher/left';
        } else if (index <= 6) {
            key = 'weapon_' + index + '/left';
        } else if (index === 7) {
            key = 'armor_1';
        } else if (index === 8) {
            key = 'armor_2';
        }

        var temp = getPlayerEntity().accessories;
        if (temp[key]) {
            delete temp[key];
        } else {
            temp[key] = true;
        }
        getPlayerEntity().accessories = temp;
    },

    clientClick: function(button, down) {
        if (down) {
            log(ERROR, "Bararum");
            WorldSignals.emit(SIGNAL.LOUD_NOISE, getPlayerEntity().getCenter());
        }
    },
}));

// Setup game

GameManager.setup([
    GameManager.managerPlugins.messages,
    AutoTargeting.managerPlugin,
    {
        clientActivate: function() {
            if (!this.shownWelcome) {
//                this.addHUDMessage("Press 'H' for help", 0xCCDDFF, 8.0);
                this.shownWelcome = true;
            }
        },
    },
]);

if (Global.SERVER) { // Run this only on the server - not the clients
    var entities = CAPI.readFile("./entities.json");
    loadEntities(entities);

    GameManager.getSingleton().registerTeams([
        {
            _name: 'players',
            setup: function(player) {
                player.defaultModelName = 'space/ships/1';
            },
        },
    ]);
}

