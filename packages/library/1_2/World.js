
// Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
// This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

//! Add additional stuff to the CAPIExtras.js World object
World = merge(World, {
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
    getReflectedRay: function(ray, normal, elasticity) {
        elasticity = defaultValue(elasticity, 1.0);

        var bounceDirection = normal.mulNew(-normal.scalarProduct(ray));
        return ray.add(bounceDirection.mul(1 + elasticity));
    },

    //! Given a physical thing - an object with .position, .velocity, and .radius, we simulate
    //! it's movement for a given time, and make it bounce from world geometry.
    //! Thing can optionally have .ignore, which is an entity we will ignore.
    //! @param elasticity How elastic the bounces are - at 1, all the energy is conserved, at 0, all of it is lost.
    //! @param seconds How long, in seconds
    //! @return True if all is well, false if an unavoidable collision occurred that cannot be bounced from.
    bounce: function(thing, elasticity, seconds) {

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

        movement = World.getReflectedRay(movement, normal, elasticity);

        thing.position = oldPosition.add(movement);
        if (World.isColliding(thing.position, thing.radius, thing.ignore)) return fallback(); // Recover
        thing.velocity = movement.mul(1/seconds);

        return true;
    },
});

