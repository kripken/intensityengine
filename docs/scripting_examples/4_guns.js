//// Library

Library.include('library/1_3/');

Library.include('library/' + Global.LIBRARY_VERSION + '/Plugins');
Library.include('library/' + Global.LIBRARY_VERSION + '/Health');
Library.include('library/' + Global.LIBRARY_VERSION + '/GameManager');
Library.include('library/' + Global.LIBRARY_VERSION + '/Firing');
Library.include('library/' + Global.LIBRARY_VERSION + '/guns/Insta');
Library.include('library/' + Global.LIBRARY_VERSION + '/guns/Shotgun');

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

//// Player class

playerInstaGun = Firing.registerGun(new InstaGun(), 'Sniper Rifle', 'packages/icons/info.jpg');
playerShotgun = Firing.registerGun(new Shotgun(), 'Shotgun', 'packages/icons/menu.jpg');

//// Nicer icons in the syntensity assets:
//playerInstaGun = Firing.registerGun(new InstaGun(), 'Sniper Rifle', 'packages/hud/gui_gk_Icon_w03.png');
//playerShotgun = Firing.registerGun(new Shotgun(), 'Shotgun', 'packages/hud/gui_gk_Icon_w04.png');

GamePlayer = registerEntityClass(
    bakePlugins(
        Player,
        [
            Firing.plugins.protocol,
            Firing.plugins.player,
            Health.plugin,
            GameManager.playerPlugin,
            {
                _class: "GamePlayer",

                init: function() {
                    this.gunIndexes = [playerInstaGun, playerShotgun];
                    this.currGunIndex = playerInstaGun;
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

    clientOnEntityOffMap: Health.dieIfOffMap,

    getScoreboardText: GameManager.getScoreboardText,

    clientClick: Firing.clientClick,
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

