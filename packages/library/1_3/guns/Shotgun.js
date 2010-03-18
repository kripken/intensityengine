
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


Library.include('library/' + Global.LIBRARY_VERSION + '/__CorePatches'); // For rayCollisionDistance
Library.include('library/' + Global.LIBRARY_VERSION + '/Firing');
Library.include('library/' + Global.LIBRARY_VERSION + '/EntityQueries');
Library.include('library/' + Global.LIBRARY_VERSION + '/Events');


Shotgun = Gun.extend({
    delay: 2.0,
    repeating: false,
    originTag: 'tag_chaingun', // Works with 'stromar' character model; override for others
    damage: 30, // The damage for __1__ pellet from the shot, so in theory can do this times numPellets each shot
    numPellets: 10, // Damage is split into these
    pelletRange: 55,
    pelletVariance: 20,

    pelletCache: {},
    pelletCacheTimestamp: -1,

    handleStartLogic: function(shooter, originPosition, targetPosition, targetEntity) {
        this.doRecoil(shooter, 100);

        // Shoot pellets
        var pellets = this.makePellets(shooter, originPosition, targetPosition);

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

Map.preloadSound('olpc/MichaelBierylo/sfx_DoorSlam.wav');

