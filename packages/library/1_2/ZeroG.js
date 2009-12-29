
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


// Note: You can use this approach to do zero gravity for specific entities
// (like floating monsters). Set world gravity to 0, and manage gravity
// manually for the entities that need it

ZeroG = {
    entities: {
        GravityWell: registerEntityClass(bakePlugins(
            WorldMarker,
            [
                {
                    _class: 'GravityWell',
                    shouldAct: true,

                    power: new StateFloat(),

                    init: function() {
                        this.power = 150;
                    },

                    clientAct: function(seconds) {
                        var entities = [getPlayerEntity()];
                        var power = this.power;
                        var origin = this.position;

                        Effect.splash(PARTICLE.SPARK, 15, seconds*10, origin, 0x55FF90, 1.0, 70, 1);

                        forEach(entities, function(entity) {
                            if (!entity.trueVelocity) return;

                            var direction = origin.subNew(entity.getCenter());
                            var distance = direction.magnitude();
                            if (distance < 0.5 || distance > power) return;
                            direction.normalize();
                            var effect = power*seconds;// / Math.sqrt(distance); // Not really 'Newtonian' physics

                            if (effect > 0) {
                                // Note - apply to old, for our custom gravity system
                                entity.trueVelocity.add(direction.mul(effect));
                            }
                        });
                    },
                },
            ]
        )),
    },
};


// Setup

World.gravity = 0;

