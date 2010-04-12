
// Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
// This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

//! A flexible and fast octree for collisions and so forth
Octree = {
    root: null,
    entities: {}, //!< uniqueId-> {uniqueId, position, collisionRadiusWidth and height}

    refresh: cacheByTimeDelay(function() {
        var entities = getEntitiesByClass('AreaTrigger');

        // Compare to Octree entities, check for changes in position, new/removed entities, etc.
        forEach(values(entities), function(entity) {
            var octreeEntity = Octree.entities[entity.uniqueId];
            if (!octreeEntity) {
                Octree.insertEntity(entity);
            } else {
                if (entity.position.subNew(octreeEntity.position).magnitude() > 0.5 ||
                    entity.collisionRadiusWidth != octreeEntity.collisionRadiusWidth ||
                    entity.collisionRadiusHeight != octreeEntity.collisionRadiusHeight) {
                    Octree.removeEntity(entity);
                    Octree.insertEntity(entity);
                }
            }
        });

        forEach(values(Octree.entities), function(octreeEntity) {
            var entity = getEntity(octreeEntity.uniqueId)
            if (!entity) {
                Octree.removeEntity(entity);
            }
        });
    }, 1/5),

    OctreeNode
    insertEntity: function(entity) {
    },

    removeEntity: function(entity) {
    },

    //! Loops into the octree, calling func with octree nodes for every octree node that
    //! that position+radius intersect
    loopInto: function(position, radius, func) {
        Octree.root = Octree.refresh();
    },

    manageTriggeringCollisions: function() {
        var time;
        if (Global.profiling && Global.profiling.data) {
            time = CAPI.currTime();
        }

        forEach(getClientEntities(), function(player) {
            if (isPlayerEditing(player)) return;

            var position = player.position.copy();
            position.z += (player.eyeHeight+player.aboveEye)/2;
            Octree.loopInto(position, Math.max(player.radius, (player.eyeHeight+player.aboveEye)/2), function(node){
                for (var i = 0; i < node.entities.length; i++) {
                    var entity = node.entities[i];

                    if (World.isPlayerCollidingEntity(player, entity)) {
                        if (Global.CLIENT) {
                            entity.clientOnCollision(player);
                        } else {
                            entity.onCollision(player);
                        }
                    }
                }
            });
        });

        if (Global.profiling && Global.profiling.data) {
            var _class = '__TriggeringCollisions__';
            time = CAPI.currTime() - time;
            if (Global.profiling.data[_class] === undefined) Global.profiling.data[_class] = 0;
            Global.profiling.data[_class] += time;
        }
    },
};


// Replace default system, which is 'flat'
manageTriggeringCollisions = Octree.manageTriggeringCollisions;

