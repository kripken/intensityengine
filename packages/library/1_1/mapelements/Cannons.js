
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


Library.include('library/1_1/AutoTargeting');
Library.include('library/1_1/MultipartRendering');

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

    sufferDamage: function(kwargs) {
        this._health = Math.max(this._health - kwargs.damage, 0);
    },
};

