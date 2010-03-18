// Default materials, etc.

Library.include('library/MapDefaults');

// Textures

Library.include('yo_frankie/');

//// Player class

registerEntityClass(Player.extend({
    _class: "GamePlayer",
}));

//// Application

ApplicationManager.setApplicationClass(Application.extend({
    getPcClass: function() {
        return "GamePlayer";
    },
}));

//// Load permanent entities

if (Global.SERVER) { // Run this only on the server - not the clients
    var entities = CAPI.readFile("./entities.json");
    loadEntities(entities);
}

