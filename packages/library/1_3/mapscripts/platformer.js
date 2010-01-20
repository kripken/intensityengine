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

Tools.replaceFunction('Map.texture', function(type, _name, rot, xoffset, yoffset, scale, forceindex) {
    scale = scale * 0.5;
    arguments.callee._super.apply(this, [type, _name, rot, xoffset, yoffset, scale, forceindex]);
}, false);

//Library.include('yo_frankie/');
Library.include('textures/gk/swarm/');

// Map settings

Map.fogColor(45, 30, 10);
Map.fog(1500);
Map.loadSky("skyboxes/gk2/swarm/nnu_sb01");
Map.skylight(0, 0, 0);
Map.ambient(1);
Map.shadowmapAmbient("0x101010");
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
    performJump: Character.plugins.jumpWhilePressingSpace.performJump,
    performMousemove: Platformer.performMousemove,

//    clientClick: Platformer.clientClick,

    getCrosshair: function() { return isPlayerEditing(getPlayerEntity()) ? "data/crosshair.png" : '' },
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

