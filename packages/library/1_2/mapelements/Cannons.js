
// Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
// This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

Library.include('library/1_2/AutoTargeting');
Library.include('library/1_2/MultipartRendering');

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
    },

    getCenter: function() {
        var ret = this.position.copy();
        ret.z += this.upperHeight;
        return ret;
    },
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

//! Plugin to make the cannon shootable
CannonHealthPlugin = {
    health: new StateInteger({ reliable: false }),

    activate: function(kwargs) {
        this._health = 30; // Server-side only, used for incrementing without frequent updates
        this.health = this._health;
        this.connect('onModify__health', function(value) {
            this._health = Math.max(0, value);
        });
    },

    act: function(seconds) {
        this._health += seconds*4;
        this._health = Math.min(this._health, 30);
        if (Math.abs(this._health - this.health) > 4) {
            this.health = this._health; // send update
        }
        if (this._health <= 25 && this.canAutoTarget) {
            this.canAutoTarget = false;
        } else if (this._health > 25 && !this.canAutoTarget) {
            this.canAutoTarget = true;
        }
    },

    clientAct: function(seconds) {
        if (!this.canAutoTarget) {
            Effect.flame(PARTICLE.SMOKE, this.getCenter(), 0.5, 1.5, 0x000000, 1, 7.0, 100, Math.max(seconds*50, 0.5), -15);
        }
    },

    sufferDamage: function(kwargs) {
        this._health = Math.max(this._health - kwargs.damage, 0);
    },
};

