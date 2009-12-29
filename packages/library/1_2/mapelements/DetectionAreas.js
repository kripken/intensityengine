
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

