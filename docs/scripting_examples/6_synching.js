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

//// Player class

GamePlayer = registerEntityClass(
    bakePlugins(
        Player,
        [
            Health.plugin,
            GameManager.playerPlugin,
            {
                _class: "GamePlayer",
            },
        ]
    )
);

////

MyTree = registerEntityClass(
    bakePlugins(Mapmodel, [{
        _class: 'MyTree',

        effectColor: new StateInteger(),

        init: function() {
            this.effectColor = 0xFFFFFF;
            this.modelName = 'tree';
        },

        activate: function() {
            GameManager.getSingleton().eventManager.add({
                secondsBefore: 0,
                secondsBetween: 0.5,
                func: bind(function() {
                    var players = getEntitiesByClass('GamePlayer');
                    var minDist = 10240;
                    forEach(players, function(player) {
                        minDist = Math.min(minDist, player.position.subNew(this.position).magnitude());
                    }, this);
                    var newColor = Math.floor(clamp((minDist-20)/128, 0, 1)*255);
                    newColor = 255-newColor + (newColor << 16) + 0x001000;
                    if (this.effectColor !== newColor) this.effectColor = newColor;
                }, this),
                entity: this,
            });
        },

        clientActivate: function() {
            GameManager.getSingleton().eventManager.add({
                secondsBefore: 0,
                secondsBetween: 0.1,
                func: bind(function() {
                    var effectPosition = this.position.copy();
                    effectPosition.z += 32;
                    Effect.splash(PARTICLE.SPARK, 50, 0.25, effectPosition, this.effectColor, 1.0, 70, 1);
                }, this),
                entity: this,
            });
        },
    }])
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

    var tree = newEntity('MyTree');
    tree.position = [512, 512, 512];
}

