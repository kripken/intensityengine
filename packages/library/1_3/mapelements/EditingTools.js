
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


EditingTools = {
    showDebugSphere: function(center, radius, color) {
        var speed = 10;
        if (Math.random() < Global.currTimeDelta*speed) {
            var player = getPlayerEntity();
            var normal = player.position.subNew(center).normalize();
            var dir = Random.normalizedVector3().projectAlongSurface(normal).normalize();
            var up = new Vector3(0, 0, 1);
            var x = new Vector3(1, 0, 0);
            var y = new Vector3(0, 1, 0);
            forEach([
                dir, normal.crossProduct(up), normal.crossProduct(up).mul(-1), up, up.mulNew(-1), x, x.mulNew(-1), y, y.mulNew(-1)
            ], function(where) {
                where.mul(radius);
                Effect.lightning(center, center.addNew(where), 1.5/speed, color, 1.0);
            });
        }
    },
};


//! A plugin that lets an entity have a parent, and display nice debug effects
//! in edit mode so the relationship is clear
ChildEntityPlugin = {
    shouldAct: true,

    debugDisplay: {
        color: 0x22BBFF,
        radius: 1.0,
    },

    parentEntityClass: '',
    parentSearchRadius: 256,

    parentEntity: new StateInteger(),

    init: function() {
        this.parentEntity = -1;
    },

    activate: function() {
        if (this.parentEntity === -1) {
            // Start with the closest relevant parent, to make map builders' lives easy. That's what we're all about.
            var possibles = getCloseEntities(this.position, this.parentSearchRadius, getEntityClass(this.parentEntityClass));
            if (possibles.length > 0) {
                if (this !== possibles[0][0]) {
                    this.parentEntity = possibles[0][0].uniqueId;
                } else {
                    if (possibles.length > 1) {
                        this.parentEntity = possibles[1][0].uniqueId;
                    }
                }
            }
        }
    },

    clientActivate: function() {
        this.debugDisplay.currPosition = 0;
    },

    clientAct: function(seconds) {
        if (isPlayerEditing() && this.parentEntity >= 0) {
            // Debug printout, to make map builders' lives easy. That's what we're all about.
            var parent = getEntity(this.parentEntity);
            if (parent) {
                Effect.lightning(parent.position, this.position, 0.0, this.debugDisplay.color, this.debugDisplay.radius);

                var direction = parent.position.subNew(this.position)
                this.debugDisplay.currPosition = clamp(this.debugDisplay.currPosition + seconds/4, 0, 1);
                if (this.debugDisplay.currPosition === 1) this.debugDisplay.currPosition = 0;
                var nextPosition = clamp(this.debugDisplay.currPosition + 0.2, 0, 1);
                Effect.flare(PARTICLE.STREAK,
                    this.position.addNew(direction.mulNew(this.debugDisplay.currPosition)),
                    this.position.addNew(direction.mulNew(nextPosition)),
                    0,
                    this.debugDisplay.color,
                    this.debugDisplay.radius
                )
            }
        }
    },
};

