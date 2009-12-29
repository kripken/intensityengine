// (C) 2009 Alon 'Kripken' Zakai

Library.include('library/Plugins');
Library.include('library/Firing');
Library.include('library/Health');
Library.include('library/GameManager');
Library.include('library/modes/CTF');
Library.include('library/AutoTargeting');
Library.include('library/MultipartRendering');
Library.include('library/guns/Stunball');
Library.include('models/cannon/Plugin');

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
Map.shadowmapAmbient("0x101010");
Map.shadowmapAngle(300);

//// Player class

registerEntityClass(
    bakePlugins(
        Player,
        [
            Firing.plugins.protocol,
            Firing.plugins.player,
            Health.plugin,
            GameManager.playerPlugin,
            CTFMode.playerPlugin,
            StunballVictimPlugin,
            {
                _class: "GamePlayer",

                init: function() {
                    this.modelName = ''; // Start invisible, until spawned
                    this.canMove = false; // Start immobile, until spawned
                },

                activate: function() {
                    this.movementSpeed = 150;
                }
            }
        ]
    )
);

Firing.registerGun(1, new SniperRifle(), true);

// Autocannons

gunCounter = 9

function makeCannon(_name, gunClass, additionalPlugins) {
    additionalPlugins = defaultValue(additionalPlugins, []);

    var CannonGun = bakePlugins(gunClass, [CannonGunPlugin]);
    var cannonGun = new CannonGun();
    Firing.registerGun(gunCounter, cannonGun);
    gunCounter += 1;
    var plugins = [AutoTargetingPlugin, MultipartRenderingPlugin, MultipartRenderingAutotargetingPlugin, CannonPlugin, Firing.plugins.protocol, BotFiringPlugin, { _class: _name, gun: cannonGun }];
    return registerEntityClass( bakePlugins(Mapmodel, plugins.concat(additionalPlugins)) );
}

makeCannon('InstaCannon', SniperRifle);

makeCannon('StunCannon', StunballGun, [Projectiles.plugin, StunballBotPlugin]);


//// Application

ApplicationManager.setApplicationClass(Application.extend({
    _class: "GameApplication",

    getPcClass: function() {
        return "GamePlayer";
    },

    // Replace this with appropriate behaviour for when a player falls of the map
    clientOnEntityOffMap: Health.dieIfOffMap,

    getScoreboardText: GameManager.getScoreboardText,

    clientClick: Firing.clientClick,
}));

//// Load permanent entities

GameManager.setup([
    CTFMode.managerPlugin,
    merge(GameManager.managerPlugins.limitGameTime, { MAX_TIME: 10*60 }),
    merge(GameManager.managerPlugins.limitGameScore, { MAX_SCORE: 1 }),
    GameManager.managerPlugins.intermission,
]);

if (Global.SERVER) { // Run this only on the server - not the clients
    var entities = CAPI.readFile("./entities.json");
    loadEntities(entities);

    GameManager.getSingleton().registerTeams([
        {
            _name: 'marines',
            setup: function(player) {
                player.defaultModelName = 'stromar';
            },
        },
        {
            _name: 'squirrels',
            setup: function(player) {
                player.defaultModelName = 'frankie';
            },
        },
    ]);
}

Map.preloadModel('stromar');
Map.preloadModel('frankie');


