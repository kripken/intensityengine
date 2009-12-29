
/*
 *=============================================================================

 * Copyright (C) 2008 Alon Zakai ('Kripken') kripkensteiner@gmail.com
 *
 * This file is part of the Intensity Engine project,
 *    http://www.intensityengine.com
 *
 * The Intensity Engine is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * The Intensity Engine is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with the Intensity Engine.  If not, see
 *     http://www.gnu.org/licenses/
 *     http://www.gnu.org/licenses/agpl-3.0.html
 *=============================================================================
 */


Library.include('library/' + Global.LIBRARY_VERSION + '/Plugins');


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

