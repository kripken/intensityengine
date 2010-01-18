
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
        //! '+x', '-x', '+y' or '-y': the axis on which we move. Always +.
        platformAxis: new StateString({ clientSet: true }),

        //! The position along the fixed axis (the other one, orthogonal to forward)
        platformPosition: new StateString({ clientSet: true }),

        //! The axis for the camera: axis and a distance
        platformCamera: new StateJSON({ clientSet: true }),

        //! Direction the player is facing, in the platform axis : left or right
        getPlatformDirection: function() {
            return this.xmapDefinedPositionData - 1;
        },

        setPlatformDirection: function(direction) {
            this.xmapDefinedPositionData = direction + 1;
        },

        init: function() {
            this.platformAxis = '+x';
            this.platformPosition = 512;
            this.platformCamera = {
                axis: '+y',
                distance: 100,
            };

            this.movementSpeed = 75;
        },

        clientActivate: function() {
            this.setPlatformDirection(1);
        },

        clientAct: function() {
            if (this === getPlayerEntity() && !isPlayerEditing(this)) {
                // Affix to the position
                var position = this.position.copy();
                if (this.platformAxis[1] == 'x') {
                    if (Math.abs(position.y - this.platformPosition) > 0.5) {
                        position.y = this.platformPosition;
                    } else position = null;
                } else {
                    if (Math.abs(position.x - this.platformPosition) > 0.5) {
                        position.x = this.platformPosition;
                    } else position = null;
                }
                if (position !== null) {
                    this.position = position;
                    log(DEBUG, "Fixed platform position");
                }

                var orientation = Platformer.vector3FromAxis(this.platformAxis).mul(this.getPlatformDirection()).toYawPitch();
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

        isOnFloor: function() {
            if (floorDistance(this.position, 1024) < 1) return true;
log(ERROR, "wha?" + this.velocity + ',' + this.falling);
            if (this.velocity.z < -1 || this.falling.z < -1) return false;
            var axis = Platformer.vector3FromAxis(this.platformAxis).mul(this.radius);
log(ERROR, axis);
            if (floorDistance(this.position.copy().add(axis), 1024) < 1) return true;
            if (floorDistance(this.position.copy().add(axis.mul(-1)), 1024) < 1) return true;
            return false;
        },
    },

    performMovement: function(move, down) {
        if (isPlayerEditing(getPlayerEntity())) return this._super.apply(this, arguments);

        if (move === 1) this.performJump(down);
    },

    //! Called when the left/right buttons are pressed. By default we do a normal strafe
    //! @param strafe Left or right
    //! @param down If the button press is down or not
    performStrafe: function(strafe, down) {
        if (isPlayerEditing(getPlayerEntity())) return this._super.apply(this, arguments);

        var player = getPlayerEntity();
        if (Platformer.vector3FromAxis(player.platformCamera.axis).crossProduct(Platformer.vector3FromAxis(player.platformAxis)).z < 0)
            strafe = -strafe;

        var old = player.getPlatformDirection(strafe);
        if (strafe !== 0) player.setPlatformDirection(strafe);
        getPlayerEntity().move = down;
    },

    performJump: function(down) {
        var player = getPlayerEntity();
        if (down && player.isOnFloor()) {
            player.velocity.z += 200;
        }
    },
};

// mouse wheel zooming out etc.

// jump while pressing

// fov control

