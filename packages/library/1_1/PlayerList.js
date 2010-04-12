
// Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
// This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

// Usage: Bake PlayerList.plugin into the player class, and
//        call PlayerList.getScoreboardText from the Application
//        function of the same name

PlayerList = {
    list: {},

    getScoreboardText: function() {
        var data = [];
        forEach(values(PlayerList.list), function (player) {
            data.push([player.uniqueId, player._name + " -"]);
        });
        return data;
    },

    plugin: {
        clientActivate: function() {
            PlayerList.list[this.uniqueId] = this;
        },

        clientDeactivate: function() {
            delete PlayerList.list[this.uniqueId];
        },
    },
};

