
// Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
// This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

DetectionAreas = {
    plugin: {
        _class: 'DetectionArea',

        detectionArea: true,

        check: function(position) {
            return position.z >= this.position.z && position.z <= (this.position.z + 2*this.collisionRadiusHeight)
                && position.x >= (this.position.x - this.collisionRadiusWidth)
                && position.x <= (this.position.x + this.collisionRadiusWidth)
                && position.y >= (this.position.y - this.collisionRadiusWidth)
                && position.y <= (this.position.y + this.collisionRadiusWidth);
        },
    },

    //! Goes over all DetectionAreas with that tag, and checks if position is in any of them
    check: function(position, tag) {
        return reduce(function(curr, entity) {
            return curr || (
                entity.detectionArea &&
                entity.check(position)
            );
        }, getEntitiesByTag(tag), false);
    },
};

registerEntityClass(bakePlugins(AreaTrigger, [DetectionAreas.plugin]));

