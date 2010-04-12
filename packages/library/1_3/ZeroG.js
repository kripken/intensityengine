
// Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
// This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

// Note: You can use this approach to do zero gravity for specific entities
// (like floating monsters). Set world gravity to 0, and manage gravity
// manually for the entities that need it

ZeroG = {
    AIR_FRICTION: 0.4, // 0 means no friction with air
    SURFACE_FRICTION: 0.2, // Reduces inertia when on the floor
    WATER_FRICTION: 0.9, // Reduces inertia when on the floor

    plugin: {
        clientAct: function(seconds) {
            var floorDist = rayCollisionDistance(this.position.copy(), World.gravityDirection);
            this.onFloor = floorDist < this.radius*1.5 && floorDist >= 0;
//log(ERROR, floorDist + ',' + this.onFloor);

            if (this !== getPlayerEntity()) return;

            if (!this.trueVelocity) this.trueVelocity = new Vector3(0, 0, 0);

            if (this.clientState !== CLIENTSTATE.EDITING) {
                // If sauer changed our *direction* (and not just magnitude of velocity) then we hit something - apply that
                var saved = this.savedVelocity;
                var now = this.velocity.copy();
                if (saved && saved.magnitude() > 0 && now.magnitude() > 0 && saved.cosineAngleWith(now) < 0.95) {
                    if (saved.subNew(now).magnitude() > 30) {
                        Sound.play('olpc/AdamKeshen/kik.wav', this.oldPosition);
                    }
                    this.trueVelocity = now;
                }

                var friction = ZeroG.AIR_FRICTION;
                if (ZeroG.WATER_FRICTION && this.inWater) {
                    friction = ZeroG.WATER_FRICTION;
                } else if (ZeroG.SURFACE_FRICTION && this.onFloor) {
                    friction = ZeroG.AIR_FRICTION + ZeroG.SURFACE_FRICTION;
                }
                friction = clamp(friction, 0, 1);

//                this.trueVelocity = this.trueVelocity.mul(1 - friction*seconds); // Apply inertia slowing

//                this.move = 0;
//                this.strafe = 0;

                if (this.move && this.onFloor) {
                    this.trueVelocity = this.trueVelocity.lerp(this.getThrustDirection().mul(this.movementSpeed), 1-seconds);
                } else if (this.onFloor) {
                    this.trueVelocity = this.trueVelocity.mul(1-seconds*5);
                }

//                this.position.add(this.trueVelocity.mulNew(seconds));

                // Save position, to see if sauer moved us - since no velocity, must have been a collision
                this.oldPosition = this.position.copy();
                this.velocity = this.trueVelocity.copy();
                this.savedVelocity = this.trueVelocity.copy();
            } else {
                if (!this.canMove) this.canMove = true; // Move normally in edit mode

                this.oldPosition = this.position.copy();
                this.trueVelocity = new Vector3(0, 0, 0);
            }
        },

        getThrustDirection: function() {
            return new Vector3().fromYawPitch(this.yaw, this.pitch).normalize();
        },
    },

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
World.gravityDirection = new Vector3(0, 0, 0);


/* Example map script

Library.include('library/1_3/');

Library.include('library/' + Global.LIBRARY_VERSION + '/__CorePatches');
Library.include('library/' + Global.LIBRARY_VERSION + '/Plugins');
Library.include('library/' + Global.LIBRARY_VERSION + '/ZeroG');

// Default materials, etc.

Library.include('library/MapDefaults');

// Textures

Library.include('yo_frankie/');

// Map settings

Map.fogColor(0, 0, 0);
Map.fog(9999);
Map.loadSky("skyboxes/philo/sky3");
Map.skylight(100, 100, 100);
Map.ambient(20);
Map.shadowmapAmbient("0x505050");
Map.shadowmapAngle(300);

//// Player class

registerEntityClass(bakePlugins(Player, [ZeroG.plugin, {
    _class: "GamePlayer",

    init: function() {
        this.movementSpeed = 100;
        this.position = [326, 348, 751];
    },

    clientAct: function(seconds) {
        World.gravityDirection = getEntity(2).position.subNew(this.position).normalize();
        if (this.trueVelocity) {
            this.trueVelocity.add(World.gravityDirection.mulNew(seconds*100));
        }
    },
}]));
*/

