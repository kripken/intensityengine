
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

PlatformNamespace = {
    timer: 0,

    frequency: 1,
    range: 10,
    thickness: 1,

    activate: function() {
        this.PlatformNamespace = clone(PlatformNamespace);
        this.PlatformNamespace.parent = this;

        this.PlatformNamespace.absoluteHeight = this.position.z;
        this.PlatformNamespace.colliders = [];
    },

    act: function(seconds) {
        this.timer += seconds;
        if (Global.CLIENT) {
            var height = 0.5*(1+Math.sin(this.timer*this.frequency));
            height = 0.75*height + 0.25*0.5;
            height = height*this.parent.collisionRadiusHeight*2;

            this.absoluteHeight = this.parent.position.z + height;

            var oldColliders = this.colliders;
            this.colliders = [];
            forEach(oldColliders, function(collider) {
                this.placeCollider(collider);
            }, this);
        }
    },

    addCollider: function(collider) {
        this.colliders.push(collider);
    },

    placeCollider: function(collider) {
        if (collider.velocity.z + collider.falling.z > 0) {
            return; // If jumping or such, do not grab
        };

        if (collider.position.z > this.absoluteHeight + 2*this.thickness ||
            collider.position.z + collider.eyeHeight < this.absoluteHeight - 2*this.thickness) {
            return; // Client is too far above or below to matter
        };

/*
        if (collider.position.z < this.absoluteHeight + 0.1*this.thickness &&
            collider.position.z + collider.eyeHeight > this.absoluteHeight - 0.1*this.thickness) {
            // Colliding with the *side* of the platform - move away

            if ( Math.abs(this.parent.position.x + this.parent.collisionRadiusWidth - collider.position.x) <
                 Math.abs(this.parent.position.x - this.parent.collisionRadiusWidth - collider.position.x) ) {
                collider.position.x = this.parent.position.x + this.parent.collisionRadiusWidth + collider.radius;
            } else {
                collider.position.x = this.parent.position.x - this.parent.collisionRadiusWidth - collider.radius;
            }

            if ( Math.abs(this.parent.position.y + this.parent.collisionRadiusWidth - collider.position.y) <
                 Math.abs(this.parent.position.y - this.parent.collisionRadiusWidth - collider.position.y) ) {
                collider.position.y = this.parent.position.y + this.parent.collisionRadiusWidth + collider.radius;
            } else {
                collider.position.y = this.parent.position.y - this.parent.collisionRadiusWidth - collider.radius;
            }

            return;
        }
*/

        // Standing on the platform

        var temp = collider.position.copy();
        temp.z = this.absoluteHeight + this.thickness;
        CAPI.setDynentO(collider, temp);

        var temp2 = collider.falling.copy();
        temp2.z = 0;
        CAPI.setDynentFalling(collider, temp2);

        var temp3 = collider.velocity.copy();
        temp3.z = 0;
        CAPI.setDynentVel(collider, temp3);

        collider.timeInAir = 0;
        collider.physicalState = 4;
    },

    renderDynamic: function() {
        if (!this.parent.initialized) {
            return;
        }

        var o = this.parent.position.copy();
        var flags = MODEL.LIGHT | MODEL.CULL_VFC | MODEL.CULL_OCCLUDED | MODEL.CULL_QUERY | MODEL.DYNSHADOW;
//            flags |= MODEL.FULLBRIGHT; // TODO: For non-characters, use: flags |= MODEL.CULL_DIST;

        var renderingArgs = [this.parent, 'platform', ANIM_IDLE, o.x, o.y, this.absoluteHeight, 0, 0, flags, 0];

        CAPI.renderModel.apply(this.parent, renderingArgs);
    },
};

PlatformPlugin = {
    _class: "Platform",

    shouldAct: true,

    serverTimer: new StateFloat(),

    init: function() {
        this.serverTimer = 0;
    },

    activate: PlatformNamespace.activate,
    clientActivate: PlatformNamespace.activate,

    act: function(seconds) { this.PlatformNamespace.act(seconds); },
    clientAct: function(seconds) { this.PlatformNamespace.act(seconds); },

    clientOnCollision: function(collider) {
        this.PlatformNamespace.addCollider(collider);
    },

    renderDynamic: function() { this.PlatformNamespace.renderDynamic(); },
};

registerEntityClass( bakePlugins(AreaTrigger, [PlatformPlugin]) );

