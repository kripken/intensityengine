
/*
 *=============================================================================
 * Copyright (C) 2010 Alon Zakai ('Kripken') kripkensteiner@gmail.com
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


Editing = {
    getWorldSize: CAPI.editing_getWorldSize,
    getGridSize: CAPI.editing_getGridSize,
    eraseGeometry: CAPI.editing_eraseGeometry,
    createCube: CAPI.editing_createCube,
    deleteCube: CAPI.editing_deleteCube,
    setCubeTexture: CAPI.editing_setCubeTexture,
    setCubeMaterial: CAPI.editing_setCubeMaterial,
    pushCubeCorner: CAPI.editing_pushCubeCorner,
    getSelectedEntity: CAPI.editing_getSelectedEntity,

    FACE: {
        LEFT: 0,
        RIGHT: 1,
        BACK: 2,
        FRONT: 3,
        BOTTOM: 4,
        TOP: 5,
        LOW_X: 0,
        HIGH_X: 1,
        LOW_Y: 2,
        HIGH_Y: 3,
        LOW_Z: 4,
        HIGH_Z: 5,
    },

    Tools: {
        snapToGrid: function(position) {
            var gridSize = Editing.getGridSize();
            function snap(t) { return Math.round(t/gridSize)*gridSize; };
            return new Vector3(snap(position.x), snap(position.y), snap(position.z));
        },

        // Click once for top of slope, click again for bottom (opposite edge -
        // must differ in x, y and z).
        Slope: {
            stack: [],
            clientClick: function(position) {
                this.stack.push(Editing.Tools.snapToGrid(position));
                if (this.stack.length === 2) {
                    this.draw(this.stack);
                    this.stack = [];
                }

                this.visualEffectEvent = GameManager.getSingleton().eventManager.add({
                    secondsBefore: 0,
                    secondsBetween: 0.1,
                    func: bind(function(position) {
                        if (!isPlayerEditing() || this.stack.length === 0) return false;
                        forEach(this.stack, function(position) {
                            Effect.splash(PARTICLE.SPARK, 15, 0.5, position, 0xFFE090, 1.0, 70, 1);
                        });
                    }, this),
                    entity: getPlayerEntity(),
                }, this.visualEffectEvent);

            },
            draw: function(stack) {
                var gridSize = Editing.getGridSize();
                var top = stack[0];
                var bottom = stack[1];
                var dir = bottom.subNew(top);
                var delta = new Vector3(sign(dir.x), sign(dir.y), sign(dir.z)).mul(gridSize);
                for (var x = top.x; x !== bottom.x; x += delta.x) {
                    for (var y = top.y; y !== bottom.y; y += delta.y) {
                        for (var z = top.z; z !== bottom.z; z += delta.z) {
                            var height = bottom.z + (top.z-bottom.z)*(x-bottom.x)/(top.x-bottom.x);
                            if (height >= z) {
                                Editing.createCube(x + Math.min(delta.x, 0), y + Math.min(delta.y, 0), z, gridSize);
                            } else {
                                Editing.deleteCube(x + Math.min(delta.x, 0), y + Math.min(delta.y, 0), z, gridSize);
                            }
                        }
                    }
                }
            },
        },
    },
};

