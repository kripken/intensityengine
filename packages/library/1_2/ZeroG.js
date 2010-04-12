
// Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
// This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

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

