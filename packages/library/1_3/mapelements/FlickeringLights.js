
// Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
// This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

registerEntityClass(bakePlugins(WorldMarker, [{
    _class: 'FlickeringLight',

    shouldAct: { client: true },

    radius: new StateFloat(),
    color: new StateInteger(),
    lightProbability: new StateFloat(),
    minDelay: new StateFloat(),
    maxDelay: new StateFloat(),

    init: function() {
        this.radius = 20;
        this.color = 0xFFFFFF;
        this.lightProbability = 0.5;
        this.minDelay = 0.1; // Do not cause epileptic seizures
        this.maxDelay = 0.333;
    },

    clientActivate: function() {
        this.delay = 0;
    },

    clientAct: function(seconds) {
        this.delay -= seconds;
        if (this.delay <= 0) {
            this.delay = Math.max(Math.random()*this.maxDelay, this.minDelay)*2;
            if (Math.random() < this.lightProbability) {
                Effect.addDynamicLight(this.position, this.radius, this.color, this.delay, 0, 1<<2, this.radius);
            }
        }
    },
}]));

