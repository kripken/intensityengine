//// Library

Library.include('library/1_3/');

Library.include('library/' + Global.LIBRARY_VERSION + '/Plugins');
Library.include('library/' + Global.LIBRARY_VERSION + '/Health');
Library.include('library/' + Global.LIBRARY_VERSION + '/GameManager');

//// Default materials, etc.

Library.include('library/' + Global.LIBRARY_VERSION + '/MapDefaults');

//// Textures

Library.include('yo_frankie/');

//// Map settings

Map.fogColor(0, 0, 0);
Map.fog(9999);
Map.loadSky("skyboxes/philo/sky3");
Map.skylight(100, 100, 100);
Map.ambient(20);
Map.shadowmapAmbient("0x010101");
Map.shadowmapAngle(300);

//// Plugin example

RunOutOfBreathPlugin = {
    maxBreath: 1,

    clientActivate: function() {
        this.breath = this.maxBreath;
    },

    clientAct: function(seconds) {
        if (this.move || this.strafe) {
            this.breath -= seconds;
            if (this.breath <= 0) {
                this.health -= 10;
                this.breath = this.maxBreath;
            }
        } else {
            this.breath = this.maxBreath;
        }
    },
};

//// Player class

GamePlayer = registerEntityClass(
    bakePlugins(
        Player,
        [
            Health.plugin,
            RunOutOfBreathPlugin,
            GameManager.playerPlugin,
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

    clientOnEntityOffMap: Health.dieIfOffMap,

    getScoreboardText: GameManager.getScoreboardText,
}));

//// Game manager

GameManager.setup([
    GameManager.managerPlugins.messages,
    GameManager.managerPlugins.eventList,
]);

//// Load permanent entities

if (Global.SERVER) { // Run this only on the server - not the clients
    var entities = CAPI.readFile("./entities.json");
    loadEntities(entities);

    GameManager.getSingleton().registerTeams([
        {
            _name: 'players',
            setup: function(player) {
                player.defaultModelName = 'stromar';
            },
        },
    ]);
}

