// Copyright (C) 2009 Alon 'Kripken' Zakai

Global.triggeringCollisionsDelay = 0;

Library.include('library/1_3/');

Library.include('library/' + Global.LIBRARY_VERSION + '/__CorePatches');
Library.include('library/' + Global.LIBRARY_VERSION + '/Plugins');
Library.include('library/' + Global.LIBRARY_VERSION + '/Health');
Library.include('library/' + Global.LIBRARY_VERSION + '/GameManager');
Library.include('library/' + Global.LIBRARY_VERSION + '/Chat');
Library.include('library/' + Global.LIBRARY_VERSION + '/World');
Library.include('library/' + Global.LIBRARY_VERSION + '/Platformer');
Library.include('library/' + Global.LIBRARY_VERSION + '/Firing');
Library.include('library/' + Global.LIBRARY_VERSION + '/guns/Rocket');
Library.include('library/' + Global.LIBRARY_VERSION + '/guns/Chaingun');
Library.include('library/' + Global.LIBRARY_VERSION + '/mapelements/Cannons');

// Default materials, etc.

Library.include('library/' + Global.LIBRARY_VERSION + '/MapDefaults');

Tools.replaceFunction('Map.texture', function(type, _name, rot, xoffset, yoffset, scale, forceindex) {
    scale = scale * 0.5;
    arguments.callee._super.apply(this, [type, _name, rot, xoffset, yoffset, scale, forceindex]);
}, false);

//Library.include('yo_frankie/');
//Library.include('library/textures/gk/swarm/');
Library.include('library/' + Global.LIBRARY_VERSION + '/mapscripts/platformer_textures');

// Map settings

Map.fogColor(45, 30, 10);
Map.fog(1500);
Map.loadSky("skyboxes/gk2/swarm/nnu_sb01");
Map.skylight(0, 0, 0);
Map.ambient(1);
Map.shadowmapAmbient("0x101010");
Map.shadowmapAngle(300);

//// NPCs

CannonGunPlugin = {
    getOrigin: function(shooter) {
        var ret = shooter.autoTargetDirection.copy().mul(shooter.firingRadius);
        ret.add(shooter.position);
        ret.z += shooter.upperHeight*1.5;
        return ret;
    },
};

makeCannon('RocketCannon', RocketGun.extend({
    projectileClass: Rocket.extend({
        explosionPower: 25,
        speed: 150,
        timeLeft: 8.0,
        gravityFactor: 0,
    }),
}), [Projectiles.plugin, {
    firingRadius: 10,
    healthSystem: 'destructible',
}]);

//// Player class

playerRocketLauncher = Firing.registerGun(new RocketGun(), 'Rocket Launcher');
playerChaingun = Firing.registerGun(new (Chaingun.extend({ range: 25, }))(), 'Chaingun');

GamePlayer = registerEntityClass(
    bakePlugins(
        Player,
        [
            Health.plugin,
            GameManager.playerPlugin,
            Chat.playerPlugin,
            Chat.extraPlugins.skype,
            Platformer.plugin,
            Character.plugins.jumpWhilePressingSpace.plugin,
            Firing.plugins.protocol,
            Firing.plugins.player,
            Projectiles.plugin,
            Chaingun.plugin,
            {
                _class: "GamePlayer",

                init: function() {
                    this.modelName = '';
                    this.HUDModelName = '';
                    this.gunIndexes = [playerRocketLauncher, playerChaingun];
                    this.currGunIndex = playerRocketLauncher;
                },

                clientActivate: function() {
                    this.gunAmmos[playerRocketLauncher] = 20;
                    this.gunAmmos[playerChaingun] = null;
                },

                clientAct: function() {
                    if (isPlayerEditing(this)) return;
                    var lightPosition = this.center.copy();
                    lightPosition.z += 10;
                    lightPosition.add(Platformer.vector3FromAxis(this.platformCameraAxis).mul(10));
                    Effect.addDynamicLight(lightPosition, 50, 0x333333);
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

    clientClick: callAll(
        bind(Firing.clientClick, Firing)
        , function(button, down) { if (down && button === 2) Chat.voice.callTargetEntity() }
    ),

    getCrosshair: function() { return isPlayerEditing(getPlayerEntity()) ? "data/crosshair.png" : '' },
}));

// Setup game

Projectiles.serverside = false;

GameManager.setup([
    GameManager.managerPlugins.messages,
    GameManager.managerPlugins.eventList,
    ParallelActionsPlugin,
    Projectiles.plugin,
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

