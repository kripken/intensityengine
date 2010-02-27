// (C) 2009 Alon 'Kripken' Zakai

Library.include('library/1_3/');

Library.include('library/' + Global.LIBRARY_VERSION + '/Plugins');
Library.include('library/' + Global.LIBRARY_VERSION + '/Health');
Library.include('library/' + Global.LIBRARY_VERSION + '/GameManager');
Library.include('library/' + Global.LIBRARY_VERSION + '/Physics');
Library.include('library/' + Global.LIBRARY_VERSION + '/World');
Library.include('library/' + Global.LIBRARY_VERSION + '/Projectiles');
Library.include('library/' + Global.LIBRARY_VERSION + '/Firing');
Library.include('library/' + Global.LIBRARY_VERSION + '/guns/Rocket');

//// Setup physics

Physics.Engine.create('bullet');

// Default materials, etc.

Library.include('library/' + Global.LIBRARY_VERSION + '/MapDefaults');

// Textures

Library.include('yo_frankie/');

Library.include('textures/gk/brick3/');
Library.include('textures/gk/concrete3/');
Library.include('textures/gk/deco3/');
Library.include('textures/gk/ground3/');
Library.include('textures/gk/metal3/');
Library.include('textures/gk/morter3/');
Library.include('textures/gk/rock3/');
Library.include('textures/gk/walls3/');
Library.include('textures/gk/wood3/');
Library.include('textures/gk/set4/');
Library.include('textures/gk/set5/');

// Map settings

Map.fogColor(0, 0, 0);
Map.fog(9999);
Map.loadSky("skyboxes/philo/sky3");
Map.skylight(100, 100, 100);
Map.ambient(20);
Map.shadowmapAmbient("0x010101");
Map.shadowmapAngle(300);

//// Player class

playerRocketLauncher = Firing.registerGun(new RocketGun(), 'Rocket Launcher', 'packages/hud/gui_gk_Icon_w02.png');

GamePlayer = registerEntityClass(
    bakePlugins(
        Player,
        [
            Health.plugin,
            GameManager.playerPlugin,
            Physics.Engine.objectPlugin,
            Physics.Engine.playerPlugin,
            Firing.plugins.protocol,
            Firing.plugins.player,
            Projectiles.plugin,
            {
                _class: "GamePlayer",
                init: function() {
                    this.movementSpeed = 75;
                    this.gunIndexes = [playerRocketLauncher];
                    this.currGunIndex = playerRocketLauncher;
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
log(ERROR, "off map!");
        entity.position = [600,600,600];
    },

    clientClick: Firing.clientClick,
}));

GameManager.setup([
    GameManager.managerPlugins.messages,
    GameManager.managerPlugins.eventList,
//    ParallelActionsPlugin,
    {
        clientActivate: function() {
            if (!this.shownWelcome) {
//                this.addHUDMessage("Press 'H' for help", 0xCCDDFF, 8.0);
                this.shownWelcome = true;
            }
        },
    },
]);

//// Load permanent entities

if (Global.SERVER) { // Run this only on the server - not the clients
    var entities = CAPI.readFile("./entities.json");
    loadEntities(entities);

    GameManager.getSingleton().registerTeams([
        {
            _name: 'players',
            setup: function(player) {
                player.defaultModelName = 'stromar/red';
                player.defaultHUDModelName = '';
            },
        },
    ]);

    Global.queuedActions.push(function() {
        var SIZE = 20.5;

        for (var i = 0; i < 5; i++) {
            for (var j = 1; j < 6; j++) {
                newEntity('PhysicsEngineEntity', { position: new Vector3(600+i*SIZE, 600, 520+j*SIZE) });
            }
        }
    });
}

