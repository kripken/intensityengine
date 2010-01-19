// Copyright (C) 2009 Alon 'Kripken' Zakai

Library.include('library/1_3/');

Library.include('library/' + Global.LIBRARY_VERSION + '/__CorePatches');
Library.include('library/' + Global.LIBRARY_VERSION + '/Plugins');
Library.include('library/' + Global.LIBRARY_VERSION + '/Health');
Library.include('library/' + Global.LIBRARY_VERSION + '/GameManager');
Library.include('library/' + Global.LIBRARY_VERSION + '/Chat');
Library.include('library/' + Global.LIBRARY_VERSION + '/World');
Library.include('library/' + Global.LIBRARY_VERSION + '/Platformer');

// Default materials, etc.

Library.include('library/' + Global.LIBRARY_VERSION + '/MapDefaults');

Library.include('yo_frankie/');

// Map settings

Map.fogColor(0, 0, 0);
Map.fog(9999);
Map.loadSky("skyboxes/philo/sky3");
Map.skylight(100, 100, 100);
Map.ambient(20);
Map.shadowmapAmbient("0x000000");
Map.shadowmapAngle(300);

//// Player class

GamePlayer = registerEntityClass(
    bakePlugins(
        Player,
        [
            Health.plugin,
            GameManager.playerPlugin,
            Chat.playerPlugin,
            Platformer.plugin,
            Character.plugins.jumpWhilePressingSpace.plugin,
            {
                _class: "GamePlayer",

                init: function() {
                    this.modelName = '';
                    this.HUDModelName = '';
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

    handleTextMessage: Chat.handleTextMessage,

    performMovement: Platformer.performMovement,
    performStrafe: Platformer.performStrafe,
    performJump: Platformer.performJump,

//    clientClick: Platformer.clientClick,

    getCrosshair: function() { return '' },
}));

// Setup game

GameManager.setup([
    GameManager.managerPlugins.messages,
    GameManager.managerPlugins.eventList,
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
                player.defaultModelName = 'stromar/red';
                player.defaultHUDModelName = '';
            },
        },
    ]);
}

if (Global.CLIENT) {
    if (CAPI.setDefaultThirdpersonMode) {
        CAPI.setDefaultThirdpersonMode(1);
    }
}

