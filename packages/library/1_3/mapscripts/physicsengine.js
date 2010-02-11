// (C) 2009 Alon 'Kripken' Zakai

Library.include('library/1_3/');

Library.include('library/' + Global.LIBRARY_VERSION + '/__CorePatches');
Library.include('library/' + Global.LIBRARY_VERSION + '/Plugins');
Library.include('library/' + Global.LIBRARY_VERSION + '/Health');
Library.include('library/' + Global.LIBRARY_VERSION + '/GameManager');
Library.include('library/' + Global.LIBRARY_VERSION + '/Physics');
Library.include('library/' + Global.LIBRARY_VERSION + '/World');

Global.physicsEngineType = 'bullet';

// Textures

Library.include('yo_frankie/');

// Map settings

Map.fogColor(0, 0, 0);
Map.fog(9999);
Map.loadSky("skyboxes/philo/sky3");
Map.skylight(100, 100, 100);
Map.ambient(20);
Map.shadowmapAmbient("0x505050");
Map.shadowmapAngle(300);

//// Player class

GamePlayer = registerEntityClass(
    bakePlugins(
        Player,
        [
            Health.plugin,
            GameManager.playerPlugin,
            Physics.Engines.playerPlugin,
            {
                _class: "GamePlayer",
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
    }
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
}

