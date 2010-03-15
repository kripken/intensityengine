
//=============================================================================
// Copyright (C) 2008 Alon Zakai ('Kripken') kripkensteiner@gmail.com
//
// This file is part of the Intensity Engine project,
//    http://www.intensityengine.com
//
// The Intensity Engine is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published by
// the Free Software Foundation, version 3.
//
// The Intensity Engine is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with the Intensity Engine.  If not, see
//     http://www.gnu.org/licenses/
//     http://www.gnu.org/licenses/agpl-3.0.html
//=============================================================================


//! Add additional stuff to the CAPIExtras.js World object
World2 = {
    //! Finds the normal of a surface with respect to an outside (reference)
    //! point. This is useful, for example, to calculate how objects bounce off of walls.
    //! @param reference A point outside of the surface, our reference point
    //! @param surface A point on the surface (not the surface itself).
    //! @param resolution If given, what 'resolution' or relevant scale to use. Finer
    //!                   detail levels will be ignored. If not given, guessed.
    //! @return The surface normal, a Vector3, or null if we cannot
    //!         calculate it.
    getSurfaceNormal: function(reference, surface, resolution) {
        var direction = surface.subNew(reference);
        var distance = direction.magnitude();
        if (distance === 0) return null;

        resolution = defaultValue(resolution, distance/20);
        function randomResolutional() {
            return (Math.random()-0.5)*2*resolution;
        }

        var pointDirection, points, ret, temp;
        for (var attempt = 0; attempt < 3; attempt++) { // try 2 times, then give up
            // Pick three random closeby points, cast rays and find where they land on the surface. Calc normal from that
            points = [];
            for (var i = 0; i < 3; i++) {
                pointDirection = surface.addNew(new Vector3(randomResolutional(), randomResolutional(), randomResolutional())).sub(reference);
                if (pointDirection.magnitude() === 0) {
                    pointDirection.z += resolution; // simple hack
                }

                temp = rayCollisionDistance(reference, pointDirection.normalize().mul(distance*3 +resolution*3 + 3))
                points.push(pointDirection.normalize().mul(temp));
            }

            ret = points[1].sub(points[0]).crossProduct(points[2].sub(points[0]));
            if (ret.magnitude() > 0) {
                // This is good, just pick the direction - want reference to be on the right side
                if (ret.scalarProduct(reference.subNew(surface)) < 0) {
                    ret.mul(-1);
                }
                return ret.normalize();
            }
        }

        return null;
    },

    //! Calculates the reflected ray off a surface normal
    //! @return The reflected ray
    getReflectedRay: function(ray, normal, elasticity, friction) {
        elasticity = defaultValue(elasticity, 1.0);
        friction = defaultValue(friction, 1.0);

        var bounceDirection = normal.mulNew(-normal.scalarProduct(ray));
        if (friction === 1.0) {
            return ray.add(bounceDirection.mul(1 + elasticity));
        } else {
            var surfaceDirection = ray.addNew(bounceDirection);
            return surfaceDirection.mul(friction).add(bounceDirection.mul(elasticity));
        }
    },

    //! Given a physical thing - an object with .position, .velocity, and .radius, we simulate
    //! it's movement for a given time, and make it bounce from world geometry.
    //! Thing can optionally have .ignore, which is an entity we will ignore.
    //! @param elasticity How elastic the bounces are - at 1, all the energy is conserved, at 0, all of it is lost.
    //! @param seconds How long, in seconds
    //! @return True if all is well, false if an unavoidable collision occurred that cannot be bounced from.
    bounce: function(thing, elasticity, friction, seconds) {
        if (seconds === undefined) { // backwards compatibility for when we didn't have friction
            seconds = friction;
            friction = 1.0;
        }

        function fallback() {
            // We failed to bounce. Just go in the reverse direction from the last 'ok' spot - better than something more embarassing
            if (thing.lastSafe && thing.lastSafe[1]) {
                thing.position = thing.lastSafe[1].position;
                thing.velocity = thing.lastSafe[1].velocity;
            } else if (thing.lastSafe) {
                thing.position = thing.lastSafe[0].position;
                thing.velocity = thing.lastSafe[0].velocity;
            }
            thing.velocity.mul(-1);
            return true;
        }

        elasticity = defaultValue(elasticity, 0.9);

        if (seconds === 0 || thing.velocity.magnitude() === 0) return true;

        if (World.isColliding(thing.position, thing.radius, thing.ignore)) return fallback();

        // Save 2 backwards - [0] is the latest, and will have constraints applied. [1] is two back, so it was safe even WITH
        // constraints applied to it
        thing.lastSafe = [{
            position: thing.position.copy(),
            velocity: thing.velocity.copy(),
        }, (thing.lastSafe ? thing.lastSafe[0] : null)];

        var oldPosition = thing.position.copy();
        var movement = thing.velocity.mulNew(seconds);
        thing.position.add(movement);

        if (!World.isColliding(thing.position, thing.radius, thing.ignore)) return true; // All done

        // Try actual bounce

        var direction = movement.copy().normalize();
        var surfaceDist = rayCollisionDistance(oldPosition, direction.mulNew(3*movement.magnitude() + 3*thing.radius + 1.5))
        if (surfaceDist < 0) return fallback();
        var surface = oldPosition.addNew(direction.mulNew(surfaceDist));
        var normal = World.getSurfaceNormal(oldPosition, surface);//, thing.radius*1.1);
        if (normal === null) return fallback();

        movement = World.getReflectedRay(movement, normal, elasticity, friction);

        thing.position = oldPosition.add(movement);
        if (World.isColliding(thing.position, thing.radius, thing.ignore)) return fallback(); // Recover
        thing.velocity = movement.mul(1/seconds);

        return true;
    },

    getMaterial: function(position) {
        return CAPI.getMaterial(position.x, position.y, position.z);
    },

    //! Given an origin and a direction, finds the collision point with the world
    //! @param direction Assumed normalized
    getRayCollisionWorld: function(origin, direction, maxDist) {
        maxDist = defaultValue(maxDist, 2048);
        var dist = rayCollisionDistance(origin, direction.mulNew(maxDist));
        return origin.addNew(direction.mulNew(Math.min(dist, maxDist)));
    },

    //! Given an origin and a target (*NOT* a direction as with getRayCollisionWorld!), finds whether
    //! any entities intersect the ray, and if so, returns {entity, collisionPosition}.
    getRayCollisionEntities: function(origin, target, ignore) {
        var entities = World.getCollidableEntities();
        var direction = target.subNew(origin);
        var dist2 = direction.magnitude();
        if (dist2 === 0) return null;
        dist2 = dist2*dist2;

        var best = null;
        function consider(entity, alpha, collisionPosition) {
            if (best === null || distance < best.distance) {
                best = {
                    entity: entity,
                    alpha: alpha,
                    collisionPosition: collisionPosition,
                };
            }
        }

        forEach(entities, function(entity) {
            if (entity === ignore) return;

            // Find distance
            var entityDirection = entity.center.subNew(origin);
            var entityRadius = entity.radius ? entity.radius : Math.max(entity.collisionRadiusWidth, entity.collisionRadiusHeight);
            var alpha = direction.scalarProduct(entityDirection) / dist2;
            var collisionPosition = origin.addNew(direction.mulNew(alpha));
            var distance = entity.center.subNew(collisionPosition).magnitude();
            if (alpha < 0 || alpha > 1 || distance > entityRadius) return; // XXX Alpha check ignores radius
            consider(entity, alpha, collisionPosition);
        });

        return best;
    },

    isCollidingEntities: function(position, radius, ignore) {
        var entities = World.getCollidableEntities();

        for (var i = 0; i < entities.length; i++) {
            var entity = entities[i];

            if (entity === ignore || entity.deactivated) continue;

            var entityRadius = entity.radius ? entity.radius : Math.max(entity.collisionRadiusWidth, entity.collisionRadiusHeight);
            if (position.isCloseTo(entity.position, radius + entityRadius)) return true;
        }

        return false;
    },

    //! By default, we return characters. Static entities have their collisions as part of the world anyhow.
    //! Override this to add any other entities, like swarms etc.
    //! All the entities returned must have a center and a radius.
    getCollidableEntities: function() {
        return getEntitiesByClass(getEntityClass('Character'));
    },

    //!
    isPlayerCollidingEntity: function(player, entity) {
        if (entity.collisionRadiusWidth) {
            // z
            if (player.position.z >= entity.position.z + 2*entity.collisionRadiusHeight ||
                player.position.z + player.eyeHeight + player.aboveEye <= entity.position.z) return false;

            // x
            if (player.position.x - player.radius >= entity.position.x + entity.collisionRadiusWidth ||
                player.position.x + player.radius <= entity.position.x - entity.collisionRadiusWidth) return false;

            // y
            if (player.position.y - player.radius >= entity.position.y + entity.collisionRadiusWidth ||
                player.position.y + player.radius <= entity.position.y - entity.collisionRadiusWidth) return false;

            return true;
        } else {
            // z
            if (player.position.z >= entity.position.z + entity.eyeHeight + entity.aboveEye ||
                player.position.z + player.eyeHeight + player.aboveEye <= entity.position.z) return false;

            // x
            if (player.position.x - player.radius >= entity.position.x + entity.radius ||
                player.position.x + player.radius <= entity.position.x - entity.radius) return false;

            // y
            if (player.position.y - player.radius >= entity.position.y + entity.radius ||
                player.position.y + player.radius <= entity.position.y - entity.radius) return false;

            return true;
        }
    },
};

//! Keep World getters/setters (MochiKit merge fails on that)
for (item in World2) {
    World[item] = World2[item];
}

var oldIsColliding = World.isColliding;
World.isColliding = function(position, radius, ignore) {
    if (oldIsColliding.apply(World, arguments)) {
        return true;
    }

    return World.isCollidingEntities(position, radius, ignore);
}

// Should reflect iengine.h

MATF_CLIP_SHIFT = 3;

MATERIAL = {
    AIR: 0,
    WATER: 1,
    LAVA: 2,
    GLASS: 3,
    NOCLIP: 1 << MATF_CLIP_SHIFT,  // collisions always treat cube as empty
    CLIP: 2 << MATF_CLIP_SHIFT,  // collisions always treat cube as solid
};

