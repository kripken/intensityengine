
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

