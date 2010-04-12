
// Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
// This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

rayCollisionDistance = function(origin, ray) {
    var rayMag = ray.magnitude();
    return CAPI.rayPos( origin.x, origin.y, origin.z, ray.x/rayMag, ray.y/rayMag, ray.z/rayMag, rayMag)
}


Global.currTimestamp = currTimestamp;

startFrame = function() {
    currTimestamp += 1; // backwards compatibility
    Global.currTimestamp = currTimestamp;
}


Global.time = 0; //!< Total time passed
Global.currTimeDelta = 1.0; //!< Current frame time. Initialized to 1.0 just to give a valid value if anyone reads it.

manageActions = function(seconds) {
    Global.time += seconds;
    Global.currTimeDelta = seconds;

    log(INFO, "manageActions: " + seconds);

    forEach(values(__entitiesStore), function(entity) {
//        log(INFO, "manageActions for: " + entity.uniqueId);
        if (entity.deactivated) {
            return;
        }

        if (!entity.shouldAct) {
            return;
        }

        if (Global.CLIENT) {
            entity.clientAct(seconds);
        } else {
            entity.act(seconds);
        }
    });
}


//! Returns the Logic Entities close to a particular entity (the entity itself is ignored).
//! @param origin The logic entity or position around which we look.
//! @param max_distance How far to look.
//! @param _class If given (default is None), then consider only LogicEntities that are instances of this class or its subclasses.
//! Useful for example to find all close-by doors, and not characters, etc.
//! @param with_tag If provided, then only entities having this tag will be taken into consideration.
//! @param unsorted By default we sort the output; this can disable that.
//! @return A list, from close to far, of tuples of the form (entity, distance)
getCloseEntities = function(origin, maxDistance, _class, withTag, unsorted) {
    var ret = [];

    forEach(values(__entitiesStore), function(otherEntity) {
        if ( _class && !(otherEntity instanceof _class) ) {
            return;
        }

        if ( withTag && !entity.hasTag(withTag) ) {
            return;
        }

        if (!otherEntity.position) return;

        var distance = origin.subNew(otherEntity.position).magnitude();

        if (distance <= maxDistance) {
            ret.push( [otherEntity, distance] );
        }
    });

    // Sort results by distance
    if (!unsorted) {
        ret.sort(function(a, b) { return (b[1] - a[1]); });
    }

    return ret;
}


Vector3.prototype.scalarProduct = function(other) {
    return this.x*other.x + this.y*other.y + this.z*other.z;
};

Vector3.prototype.cosineAngleWith = function(other) {
    return this.scalarProduct(other) / (this.magnitude() * other.magnitude());
};

