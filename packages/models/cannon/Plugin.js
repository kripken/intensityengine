
// Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
// This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

Library.include('library/AutoTargeting');
Library.include('library/MultipartRendering');

//! Example use: registerEntityClass( bakePlugins(Mapmodel, [AutoTargetingPlugin, MultipartRenderingPlugin, MultipartRenderingAutotargetingPlugin, CannonPlugin]) ); // FiringPlugin
CannonPlugin = {
    _class: "Cannon",

    upperHeight: 5,
    firingRadius: 12,

    activate: function(kwargs) {
        this.modelName = "invisiblegeneric/ellipse";
        this.collisionRadiusHeight = 4.5;
        this.collisionRadiusWidth = 6.66;
    },

    createRenderingArgs: function(yaw, pitch, anim, o, flags, basetime) {
        return [
            [this, "cannon/base", anim, o.x, o.y, o.z, yaw, 0, flags, basetime],
            [this, "cannon/barrel", anim, o.x, o.y, o.z+this.upperHeight, yaw, pitch, flags, basetime]
        ];
    }
};

//! Plugin for the cannon's guns, so they shoot from the right place
CannonGunPlugin = {
    getOrigin: function(shooter) {
        var ret = shooter.autoTargetDirection.copy().mul(shooter.firingRadius);
        ret.add(shooter.position);
        ret.z += shooter.upperHeight;
        return ret;
    },
};

