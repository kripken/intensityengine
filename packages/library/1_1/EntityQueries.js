
// Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
// This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

EntityQueries = {
    // Replaces some functions from LogicEntityStore

    //! Returns the Logic Entities close to a particular entity (the entity itself is ignored).
    //! @param origin The logic entity or position around which we look.
    //! @param max_distance How far to look.
    //! @param _class If given (default is None), then consider only LogicEntities that are instances of this class or its subclasses.
    //! Useful for example to find all close-by doors, and not characters, etc.
    //! @param with_tag If provided, then only entities having this tag will be taken into consideration.
    //! @param unsorted By default we sort the output; this can disable that.
    //! @return A list, from close to far, of tuples of the form (entity, distance)
    byDistance: function(origin, kwargs) {
        kwargs = defaultValue(kwargs, {});
        var maxDistance = kwargs.maxDistance;
        var _class = kwargs._class;
        var withTag = kwargs.withTag;
        var unsorted = kwargs.unsorted;
        var func = defaultValue(kwargs.func, function(entity) { return entity.position.copy(); });

        var ret = [];

        forEach(values(__entitiesStore), function(otherEntity) {
            if ( _class && !(otherEntity instanceof _class) ) {
                return;
            }

            if ( withTag && !entity.hasTag(withTag) ) {
                return;
            }

            var distance = origin.subNew(func(otherEntity)).magnitude();

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
};

// XXX Backport bug fix
rayCollisionDistance = function(origin, ray) {
    var rayMag = ray.magnitude();
    return CAPI.rayPos( origin.x, origin.y, origin.z, ray.x/rayMag, ray.y/rayMag, ray.z/rayMag, rayMag)
}
// XXX

