
// Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
// This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

Library.include('library/Plugins');


JumpPadPlugin = {
    _class: "JumpPad",

    jumpVelocity: new StateArrayFloat(),

    shouldAct: true,

    init: function() {
        this.jumpVelocity = [0, 0, 500]; // Default
    },

    clientActivate: function() {
        this.playerDelay = -1;
    },

    clientAct: function(seconds) {
        if (this.playerDelay > 0) {
            this.playerDelay -= seconds;
        }
    },

    clientOnCollision: function(collider) {
        if (collider !== getPlayerEntity()) return; // Each player handles themselves

        if (this.playerDelay > 0) return; // Do not trigger many times each jump
        this.playerDelay = 0.5;
        collider.velocity = this.jumpVelocity; // Hurl collider up and away
        Sound.play('olpc/Berklee44BoulangerFX/rubberband.wav');
    },
};

registerEntityClass(bakePlugins(AreaTrigger, [JumpPadPlugin]));

Map.preloadSound('olpc/Berklee44BoulangerFX/rubberband.wav');

