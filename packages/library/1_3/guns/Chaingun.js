
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



Chaingun = Gun.extend({
    _class: 'Chaingun',

    repeating: true,
    delay: 100, // We don't use this delay - we manage it ourselves
    originTag: 'tag_chaingun', // Works with 'stromar' character model; override for others
    damage: 5,
    scatter: 0.033,
    range: 150,

    pelletCache: {},
    pelletCacheTimestamp: -1,

    doShot: function(shooter, targetPosition, targetEntity) {
        shooter.chaingunFiringUpdate = true;
    },

    stopShooting: function(shooter) {
        shooter.chaingunFiringUpdate = false;
    },

    doActualShot: function(shooter) {
        if (shooter._controlledHere) {
            this.doRecoil(shooter, Math.random()*4);
        }

        var visualOrigin = this.getOrigin(shooter);
        var targetingOrigin = shooter.getTargetingOrigin(visualOrigin);

        // Targeting from the camera - where the player aimed the mouse
        var direction = new Vector3().fromYawPitch(shooter.yaw, shooter.pitch);
        direction.add(Random.normalizedVector3().mul(this.scatter)).normalize();
        var target = World.getRayCollisionWorld(targetingOrigin, direction, this.range);
        var temp = World.getRayCollisionEntities(targetingOrigin, target, shooter);
        var targetEntity;
        if (temp) {
            target = temp.collisionPosition;
            targetEntity = temp.entity;
        }

        // Check for hitting an entity from the gun source
        var temp = World.getRayCollisionEntities(visualOrigin, target, shooter);
        if (temp) {
            target = temp.collisionPosition;
            targetEntity = temp.entity;
        }

        // Check for hitting the scenery from the gun source
        direction = target.subNew(visualOrigin);
        var dist = direction.magnitude();
        var target2 = World.getRayCollisionWorld(visualOrigin, direction.normalize(), dist);
        if (target2.isCloseTo(visualOrigin, dist-2)) {
            target = target2;
            targetEntity = null;
        }

        if (targetEntity && targetEntity.sufferDamage) {
            targetEntity.sufferDamage({
                origin: target,
                damage: (shooter === getPlayerEntity()) ? this.damage : 0,
                nonControllerDamage: (shooter !== getPlayerEntity()) ? this.damage : 0,
            });
        }

        for (var i = 0; i < Random.randint(2, 4); i++) {
            Effect.flare(PARTICLE.STREAK, visualOrigin, target.subNew(Random.normalizedVector3().mul(1.5)), Chaingun.firingRate*1.5, 0xE49B4B);
        }
        Effect.lightning(visualOrigin, target, Chaingun.firingRate*1.5, 0xFF3333);
        if (Math.random() < 0.25) {
            Effect.splash(PARTICLE.SPARK, 1, Chaingun.firingRate*0.75, visualOrigin, 0xB49B4B, 1.0, 70, 1);
        }

        if (target.isCloseTo(targetingOrigin, this.range-0.25)) {
            Effect.splash(PARTICLE.SPARK, 15, 0.2, target, 0xF48877, 1.0, 70, 1);
            Effect.addDecal(DECAL.BULLET, target, visualOrigin.subNew(target).normalize(), 3.0);
            shooter.chaingunTarget = target;
        } else {
            shooter.chaingunTarget = null;
        }

//        if (Math.random() < Chaingun.firingRate*10) { // 10/sec
//            Effect.fireball(PARTICLE.EXPLOSION_NO_GLARE, origin, 1, 0.1, 0xFF775F, 1);
//        }

        Sound.play('gk/jump.ogg', visualOrigin);
    },
/*
        // Shoot pellets
        var pellet = this.makePellets(shooter, originPosition, targetPosition);

        // Collect damage per entity
        var entityDamages = {};

        forEach(pellets, function(pelletData) {
            var entity = pelletData[1];
            if (entity && entity.sufferDamage) {
                var current = [entity, 1];
                var prior = entityDamages[entity.uniqueId];
                if (prior) {
                    current[1] += prior[1];
                }
                entityDamages[entity.uniqueId] = current;
            }
        });

        var original = this.damage;
        forEach(values(entityDamages), function(entityDamage) {
            var entity = entityDamage[0];
            var pellets = entityDamage[1];

            this.damage = original * pellets;
            entity.sufferDamage(this);
        }, this);
        this.damage = original;
    },

    handleClientEffect: function(shooter, originPosition, targetPosition, targetEntity) {
        Effect.splash(PARTICLE.SMOKE, 6, 0.5, originPosition, 0xF0F0F0, 1.2, 30, -10);
        Effect.addDynamicLight(originPosition, 20, 0xFFFFFF, 0.8, 0.1, 0, 10);

        // Shoot pellets
        var pellets = this.makePellets(shooter, originPosition, targetPosition);

        forEach(pellets, function(pelletData) {
            var pellet = pelletData[0];
            var targetEntity = pelletData[1];
            var pelletTargetPosition = originPosition.addNew(pellet);

            Effect.flare(
                PARTICLE.STREAK,
                originPosition,
                pelletTargetPosition,
                0.1,
                0xFFE8D8
            );

            if (targetEntity && Health.isValidTarget(targetEntity)) {
                Effect.splash(PARTICLE.BLOOD, 13, 1.0, pelletTargetPosition, 0x60FFFF, 1.0, 70, 1);
            } else {
                Effect.addDecal(DECAL.BULLET, pelletTargetPosition, originPosition.subNew(pelletTargetPosition).normalize(), 3.0);
            }
        }, this);

        Sound.play('olpc/MichaelBierylo/sfx_DoorSlam.wav', originPosition);
        shooter.queueAction(new ParallelAction([
            new ShootAction3({ secondsLeft: this.delay }),
            new DelayedAction(function() {
                Sound.play('gk/slide', shooter.position.copy()); // may have moved
            }, { secondsLeft: 0.6 })
        ]));
    },
*/
    makePellets: function(shooter, origin, target) {
        if (this.pelletCacheTimestamp !== currTimestamp) {
            this.pelletCache = {};
            this.pelletCacheTimestamp = currTimestamp;
        }

        // Hash/cache, as we are called twice on the shooter (start logic and client effect)
        var hash = origin.toString();
        var ret = this.pelletCache[hash];
        if (!ret) {
            ret = [];

            var dir = target.subNew(origin).normalize().mul(this.pelletRange);
            var pellet, pelletTargetEntity;
            for (var i = 0; i < this.numPellets; i++) {
                pellet = dir.copy();
                pelletTargetEntity = null;

                pellet.x += (Math.random()-0.5)*this.pelletVariance;
                pellet.y += (Math.random()-0.5)*this.pelletVariance;
                pellet.z += (Math.random()-0.5)*this.pelletVariance;
                // Find collisions
                var worldDist = rayCollisionDistance(origin, pellet); // Collisitions with world
                var pelletDir = pellet.copy().normalize();
                var finalDist = worldDist;
                // Hackish way to do this, until add to API a nice way to do this sort of thing, collisition with dynents
                for (var dist = 0; dist < worldDist; dist += 6.5) {
                    var currPosition = origin.addNew( pelletDir.copy().mul(dist) );
                    var pairs = EntityQueries.byDistance(currPosition, {
                        maxDistance: 5,
                        _class: Player,
                        func: function(entity) { return entity !== getPlayerEntity() ? entity.getCenter() : new Vector3(-20, -20, -20); },
                    });
                    if (pairs.length > 0) {
                        pelletTargetEntity = pairs[0][0];
                        finalDist = dist;
                        break;
                    }
                }
                pellet.normalize().mul(finalDist);

                ret.push([pellet, pelletTargetEntity]);
            }

            this.pelletCache[hash] = ret;
        }

        return ret;
    },

});

// Protocol: controller sends updates at protocolRate when firing, of value true. Other clients
// keep firing once a true arrives, and continue for a while longer, before expiring it if no
// other true arrives. The controller sends a false to stop firing, but only once - we rely on
// expiration if it doesn't arrive.

Chaingun.protocolRate = 1/2;
Chaingun.firingRate = 1/20;

Chaingun.plugin = {
    chaingunFiringUpdate: new StateBoolean({ clientSet: true, reliable: false, hasHistory: false }),

    clientActivate: function() {
        this.chaingunFiring = false;

        this.connect('client_onModify_chaingunFiringUpdate', function(value) {
            value = value && Health.isActiveEntity(this); // Must be active

            if (!this.chaingunFiring && value) {
                // New firing
                this.chaingunFiringTimer = new RepeatingTimer(Chaingun.firingRate);
                this.chaingunFiringTimer.prime();

                if (this._controlledHere) {
                    this.chaingunProtocolTimer = new RepeatingTimer(Chaingun.protocolRate);
                }
            }

            if (!this._controlledHere) {
                this.chaingunFiringExpiration = 0;
            }

            this.chaingunFiring = value;
        });
    },

    clientAct: function(seconds) {
        if (this.chaingunFiring) {
            Effect.addDynamicLight(this.position, 30, 0xFFEECC, 0);//, 0.1, 0, 10);
            if (this.chaingunTarget) {
                Effect.addDynamicLight(this.chaingunTarget, 15, 0xFFEECC, 0);//, 0.1, 0, 10);
            }

            if (!this.chaingunFiringAction) {
                this.chaingunFiringAction = new RepeatingShootAction2({ secondsLeft: 10.0 })
                this.clearActions();
                this.queueAction(this.chaingunFiringAction);
            } else {
                this.chaingunFiringAction.secondsLeft = 10; // keep going
            }

            if (!Health.isActiveEntity(this)) { // Must be active
                this.chaingunFiring = false;
                return;
            }

            if (this.chaingunFiringTimer.tick(seconds)) {
                // Shoot (visual and maybe do damage, if controlled here)
                Chaingun.prototype.doActualShot(this);
            }

            if (this._controlledHere) {
                if (this.chaingunProtocolTimer.tick(seconds)) {
                    // Send protocol update
                    this.chaingunFiringUpdate = true;
                }
            } else {
                this.chaingunFiringExpiration += seconds;
                if (this.chaingunFiringExpiration > Chaingun.protocolRate*2.5) {
                    this.chaingunFiring = false;
                }
            }
        } else {
            if (this.chaingunFiringAction) {
                this.chaingunFiringAction.secondsLeft = -1;
                this.chaingunFiringAction = null;
            }
        }
    },
};

Map.preloadSound('olpc/MichaelBierylo/sfx_DoorSlam.wav');

