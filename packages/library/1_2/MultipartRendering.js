
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


//! Allows very dynamic rendering of multipart custom entities, that is,
//! entities for which we render multiple models (for example, a cannon
//! with a fixed base and turning barrel).
//! Override getMultipartAnimation, getMultipartYaw, getMultipartPitch,
//! getMultipartFlags as necessary. The defaults are suitable for
//! a simple mapmodel.
MultipartRenderingPlugin = {
    clientActivate: function() {
        this.renderingArgsTimestamp = -2;

        // Override existing function (do not operating alongside it, replace it)
        this.renderDynamic = function() {
            if (!this.initialized) {
                return;
            }
            if (this.renderingArgsTimestamp !== currTimestamp) {
                var anim = this.getMultipartAnimation();
                var o = this.position.copy();
                var yaw = this.getMultipartYaw();
                var pitch = this.getMultipartPitch();
                var flags = this.getMultipartFlags();
                var basetime = 0;

                this.renderingArgs = this.createRenderingArgs(yaw, pitch, anim, o, flags, basetime);
                this.renderingArgsTimestamp = currTimestamp;
            }
            forEach(this.renderingArgs, function(renderingArg) {
                CAPI.renderModel.apply(this, renderingArg);
            }, this);
        };
    },

    getMultipartAnimation: function() {
        return ANIM_IDLE | ANIM_LOOP;
    },

    getMultipartFlags: function() {
        return MODEL.LIGHT | MODEL.CULL_VFC | MODEL.CULL_OCCLUDED | MODEL.CULL_QUERY | MODEL.FULLBRIGHT | MODEL.CULL_DIST | MODEL.DYNSHADOW;
    },

    getMultipartYaw: function() {
        return this.yaw;
    },

    getMultipartPitch: function() {
        return this.pitch;
    },
};

