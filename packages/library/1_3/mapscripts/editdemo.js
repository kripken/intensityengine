
// Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
// This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

Library.include('library/1_3/');

Library.include('library/' + Global.LIBRARY_VERSION + '/Plugins');
Library.include('library/' + Global.LIBRARY_VERSION + '/Health');
Library.include('library/' + Global.LIBRARY_VERSION + '/GameManager');
Library.include('library/' + Global.LIBRARY_VERSION + '/Editing');

// Default materials, etc.

Library.include('library/' + Global.LIBRARY_VERSION + '/MapDefaults');

// Textures

Library.include('yo_frankie/');

// Map settings

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
                init: function() { this.movementSpeed = 90; },
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

    actionKey: function(index, down) {
        if (down && isPlayerEditing()) {
            Editing.Tools.Slope.clientClick(CAPI.getTargetPosition());

/* // Simple test: push down on this point, in all adjacent cubes
            var gridSize = Editing.getGridSize();
            var c = Editing.snapToGrid(CAPI.getTargetPosition());
            Effect.splash(PARTICLE.SPARK, 55, 2.0, c, 0xFFE090, 1.0, 70, 1);
            for (var x = 0; x <= 1; x++) {
                for (var y = 0; y <= 1; y++) {
                    var d = new Vector3(c.x-x*gridSize, c.y-y*gridSize, c.z-gridSize);
                    Editing.pushCubeCorner(d.x, d.y, d.z, gridSize, Editing.FACE.TOP,
                        Editing.getFaceCorner(d, Editing.FACE.TOP, c),
                    1);
                }
            }
*/
        }
    },
}));

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
                player.defaultModelName = 'stromar_1_1/red';
            },
        },
    ]);
}

//Global.profiling = { interval: 10.0 };

