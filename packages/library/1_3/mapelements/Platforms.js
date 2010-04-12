
// Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
// This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

Platform = registerEntityClass(bakePlugins(Mapmodel, [{
    _class: 'Platform',
    shouldAct: true,

    init: function() {
        this.modelName = 'platform';
    },

    clientActivate: function() {
        this.z = this.position.z;
    },

    clientAct: function() {
        this.position.z = this.z + Math.sin(Global.time)*20;
    },
}]));

