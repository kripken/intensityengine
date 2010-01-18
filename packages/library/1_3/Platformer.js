
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


//! Allows '2D' platform games, i.e., games inherently 2D in gameplay but
//! shown in nice 3D.
Platformer = {
    vector3FromAxis: function(axis) {
        var ret = new Vector3(0, 0, 0);
        if (axis === '+x') {
            ret.x = 1;
        } else if (axis === '-x') {
            ret.x = -1;
        } else if (axis === '+y') {
            ret.y = 1;
        } else {
            ret.y = -1;
        }
        return ret;
    },

    plugin: {
        //! '+x', '-x', '+y' or '-y': the axis of forward for the player
        platformAxis: new StateString({ clientSet: true }),

        //! The position along the fixed axis (the other one, orthogonal to forward)
        platformPosition: new StateString({ clientSet: true }),

        //! The axis for the camera: axis and a distance
        platformCamera: new StateJSON({ clientSet: true }),

        init: function() {
            this.platformAxis = '+x';
            this.platformPosition = 512;
            this.platformCamera = {
                axis: '+y',
                distance: 100,
            };

            this.movementSpeed = 100;
        },

        clientAct: function() {
            if (this === getPlayerEntity() && !isPlayerEditing(this)) {
                // Affix to the position
                if (this.platformAxis[1] == 'x') {
                    this.position.y = this.platformPosition;
                } else {
                    this.position.x = this.platformPosition;
                }

                var orientation = Platformer.vector3FromAxis(this.platformAxis).toYawPitch();
                this.yaw = orientation.yaw;

                var cameraPosition = this.position.copy().add(Platformer.vector3FromAxis(this.platformCamera.axis).mul(this.platformCamera.distance));
                cameraPosition.z += this.radius*3;
                var direction = this.position.subNew(cameraPosition);
                orientation = direction.toYawPitch();
                CAPI.forceCamera(
                    cameraPosition.x, cameraPosition.y, cameraPosition.z, orientation.yaw, orientation.pitch, 0
                );
            }
        },
    },
};

