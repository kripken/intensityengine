
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

    //! For each face, a list of corners, x+y*2 indexes
    CORNER: [
        {}, // TODO
        {},
        {},
        {},
        {},
        [ 0, 1, 2, 3 ],
    ],

    snapToGrid: function(position) {
        var gridSize = Editing.getGridSize();
        function snap(t) { return Math.round(t/gridSize)*gridSize; };
        return new Vector3(snap(position.x), snap(position.y), snap(position.z));
    },

    //! Gets a cube basis position, a face, and a position,
    //! and returns the corner index for that position on that face
    //! on that cube
    getFaceCorner: function(cubePosition, face, position) {
        if (face === Editing.FACE.TOP) {
            return Editing.CORNER[face][
                (position.x === cubePosition.x ? 0 : 1) +
                (position.y === cubePosition.y ? 0 : 1)*2
            ];
        } else {
            return null;
        }
    },

    Tools: {
        // Click once for top of slope, click again for bottom (opposite edge -
        // must differ in x, y and z).
        Slope: {
            stack: [],
            clientClick: function(position) {
                this.stack.push(Editing.snapToGrid(position));
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
                var smooth = true;
                var gridSize = Editing.getGridSize();
                var top = stack[0];
                var bottom = stack[1];
                var dir = bottom.subNew(top);
                var delta = new Vector3(sign(dir.x), sign(dir.y), sign(dir.z)).mul(gridSize);
                for (var x = top.x; x !== bottom.x; x += delta.x) {
                    for (var y = top.y; y !== bottom.y; y += delta.y) {
                        for (var z = top.z; z !== bottom.z; z += delta.z) {
                            var highHeight = bottom.z + (top.z-bottom.z)*(x-bottom.x)/(top.x-bottom.x);
                            var lowHeight = bottom.z + (top.z-bottom.z)*(x+delta.x-bottom.x)/(top.x-bottom.x);

                            // low-x,y,z coordinates, for the base of the cube in the octree
                            var cx = x + Math.min(delta.x, 0);
                            var cy = y + Math.min(delta.y, 0);
                            var cz = z + Math.min(delta.z, 0); // z is high, cz is low
                            var c = new Vector3(cx, cy, cz);

                            if (lowHeight >= z) {
                                Editing.createCube(cx, cy, cz, gridSize);
                            } else if (highHeight <= cz || !smooth) {
                                Editing.deleteCube(cx, cy, cz, gridSize);
                            } else {
                                // Slopey
                                Editing.createCube(cx, cy, cz, gridSize);
                                var highSteps = Math.floor(8*(highHeight - cz)/gridSize);
                                var lowSteps = Math.floor(8*(lowHeight - cz)/gridSize);
                                var d;
                                for (d = 0; d < highSteps; d++) {
                                    Editing.pushCubeCorner(cx, cy, cz, gridSize, Editing.FACE.TOP,
                                        Editing.getFaceCorner(c, Editing.FACE.TOP, new Vector3(x, y, z)),
                                    1);
                                    Editing.pushCubeCorner(cx, cy, cz, gridSize, Editing.FACE.TOP,
                                        Editing.getFaceCorner(c, Editing.FACE.TOP, new Vector3(x, y+delta.y, z)),
                                    1);
                                }
                                for (d = 0; d < lowSteps; d++) {
                                    Editing.pushCubeCorner(cx, cy, cz, gridSize, Editing.FACE.TOP,
                                        Editing.getFaceCorner(c, Editing.FACE.TOP, new Vector3(x+delta.x, y, z)),
                                    1);
                                    Editing.pushCubeCorner(cx, cy, cz, gridSize, Editing.FACE.TOP,
                                        Editing.getFaceCorner(c, Editing.FACE.TOP, new Vector3(x+delta.x, y+delta.y, z)),
                                    1);
                                }
                            }
                        }
                    }
                }
            },
        },
    },
};

