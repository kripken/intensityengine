
// Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
// This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

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

