
// Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
// This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

Library.include('library/1_1/Plugins');


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

    renderDynamic: function() {
        var o = this.position;
        var flags = MODEL.LIGHT | MODEL.CULL_VFC | MODEL.CULL_OCCLUDED | MODEL.CULL_QUERY | MODEL.FULLBRIGHT | MODEL.CULL_DIST | MODEL.DYNSHADOW;
        var yaw = -(Global.time*120) % 360;
//log(ERROR, "yaw:" + yaw + ',' + Global.time + ":" + Global.time % 360);
        var args = [this, 'jumppad', ANIM_IDLE|ANIM_LOOP, o.x, o.y, o.z, yaw, 90, flags, 0];
        CAPI.renderModel.apply(this, args);
    },
};

registerEntityClass(bakePlugins(AreaTrigger, [JumpPadPlugin]));

Map.preloadSound('olpc/Berklee44BoulangerFX/rubberband.wav');

