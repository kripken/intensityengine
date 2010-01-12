
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


Vehicles = {
    AIR_FRICTION: 0.4, // 0 means no friction with air
    SURFACE_FRICTION: 0.2, // Reduces inertia when on the floor
    WATER_FRICTION: 0.9, // Reduces inertia when on the floor

    plugin: {
        thrustPowerForward: new StateFloat(),
        thrustPowerBackward: new StateFloat(),

        init: function() {
            this.thrustPowerForward = 150;
            this.thrustPowerBackward = 50;

            this.movementSpeed = 50; // Only for edit mode
        },

        clientActivate: function(seconds) {
            this.trueVelocity = new Vector3(0, 0, 0);
            this.oldPosition = this.position.copy();
            this.oldYaw = 0;

            this.connect('clientRespawn', function() {
                this.trueVelocity = new Vector3(0, 0, 0);
            });
        },

        clientAct: function(seconds) {
            var thrust = this.move;

            if (thrust > 0 && this.clientState !== CLIENTSTATE.EDITING) {
                var smoke = 0xF0F0F0, flame = 0xBB8877;
                var exhaust = this.getExhaustPosition ? this.getExhaustPosition() : this.position.copy();
                var direction = new Vector3().fromYawPitch(this.yaw, 0);
                exhaust.add(direction.mul(-4*clamp(1-direction.scalarProduct(this.velocity.copy().normalize()), 0, 1)));

                Effect.splash(PARTICLE.SMOKE, 2, 2.0, exhaust, smoke, 2.25, 15, -100);
                Effect.flame(PARTICLE.FLAME, exhaust, 0.5, 0.5, flame, 3, 3.0, 100, 0.4, -6);
            }

            var floorDist = floorDistance(this.position.copy(), 30);
            this.onFloor = floorDist < 0.6 && floorDist >= 0;

            if (this !== getPlayerEntity()) return;

            if (this.clientState !== CLIENTSTATE.EDITING) {
//                if (this.canMove) this.canMove = false; // Move only through thrust

//                this.position = this.oldPosition; // Undo sauer movement
//                this.velocity = new Vector3(0, 0, 0);

                // If sauer changed our *direction* (and not just magnitude of velocity) then we hit something - apply that
                var saved = this.savedVelocity;
                var now = this.velocity.copy();
                if (saved && saved.magnitude() > 0 && now.magnitude() > 0 && saved.cosineAngleWith(now) < 0.95) {
                    if (saved.subNew(now).magnitude() > 30) {
                        this.onCollision();
                    }
                    this.trueVelocity = now;
                }

                var friction = Vehicles.AIR_FRICTION;
                if (Vehicles.WATER_FRICTION && this.inWater) {
                    friction = Vehicles.WATER_FRICTION;
                } else if (Vehicles.SURFACE_FRICTION && this.onFloor) {
                    friction = Vehicles.AIR_FRICTION + Vehicles.SURFACE_FRICTION;
                }
                friction = clamp(friction, 0, 1);

                this.trueVelocity = this.trueVelocity.mul(1 - friction*seconds); // Apply inertia slowing

// gravity                this.trueVelocity.z -= 100*seconds;

//                this.move = 0;
                this.strafe = 0;

                if (thrust) {
                    var power = thrust > 0 ? this.thrustPowerForward : -this.thrustPowerBackward;
                    if (this.inWater) {
                        power /= 4;
                    }
                    this.trueVelocity.add(this.getThrustDirection().mul(power*seconds));
                }

//                this.position.add(this.trueVelocity.mulNew(seconds));

                // Save position, to see if sauer moved us - since no velocity, must have been a collision
                this.oldPosition = this.position.copy();
                this.velocity = this.trueVelocity.copy();
                this.savedVelocity = this.trueVelocity.copy();
            } else {
                if (!this.canMove) this.canMove = true; // Move normally in edit mode

                this.oldPosition = this.position.copy();
                this.trueVelocity = new Vector3(0, 0, 0);
            }
        },

        onCollision: function() {
            Sound.play('olpc/AdamKeshen/kik.wav', this.oldPosition);
        },

        getThrustDirection: function() {
            return new Vector3().fromYawPitch(this.yaw, this.pitch).normalize();
        },

        decideVehicleAnimation: function(state, physstate, move, strafe, vel, falling, inwater, timeinair) {
            this.oldYaw = normalizeAngle(this.oldYaw, this.yaw);
            var diff = this.yaw - this.oldYaw
            strafe = -sign(diff) * (Math.abs(diff) > 0.5*Global.currTimeDelta*this.facingSpeed); // Since we override the strafe
            this.oldYaw = this.yaw;

            var anim = this.decideActionAnimation();

            if (state == CLIENTSTATE.EDITING || state == CLIENTSTATE.SPECTATOR) {
                anim = ANIM_EDIT | ANIM_LOOP;
            } else if (state == CLIENTSTATE.LAGGED) {
                anim = ANIM_LAG | ANIM_LOOP;
            } else {
                if(inwater && physstate<=PHYSICALSTATE.FALL) {
                    anim |= ANIM_JUMP << ANIM_SECONDARY;
                } else if (timeinair>250) {
                    anim |= (ANIM_JUMP|ANIM_END) << ANIM_SECONDARY;
                } else if (move || strafe) 
                {
                    /*if (move>0) {
                        anim |= (ANIM_FORWARD | ANIM_LOOP) << ANIM_SECONDARY;
                    } else*/ if (strafe) {
                        strafe = move >= 0 ? strafe : -strafe;
                        anim |= ((strafe>0 ? ANIM_LEFT : ANIM_RIGHT)) << ANIM_SECONDARY;
                    } else if (move<0) {
                        anim |= (ANIM_BACKWARD | ANIM_LOOP) << ANIM_SECONDARY;
                    }
                }

                if ( (anim & ANIM_INDEX) == ANIM_IDLE && ((anim >> ANIM_SECONDARY) & ANIM_INDEX)) {
                    anim >>= ANIM_SECONDARY;
                }
            }

            if(!((anim >> ANIM_SECONDARY) & ANIM_INDEX)) {
                anim |= (ANIM_IDLE | ANIM_LOOP) << ANIM_SECONDARY;
            }

            return anim;
        },
    },

    extraPlugins: {
        //! Calculates a floor orientation - that is caused by the current floor. Add it to the real yaw/pitch
        floorOrientation: {
            clientActivate: function() {
                this.floorOrientation = {
                    pitch: 0,
                };
            },

            clientAct: function(seconds) {
                function calcFloorStuff(yaw, old) {
                    var origin = this.position.copy();

                    var forward = new Vector3().fromYawPitch(this.yaw + yaw, 0).normalize().mul(1);
                    var forwardHeight = floorDistance(origin.addNew(forward), 2);
                    var backwardHeight = floorDistance(origin.subNew(forward), 2);

                    var curr;
                    var factor = seconds*4;

                    if (backwardHeight < 1.5 && forwardHeight < 1.5) {
                        var diff = forwardHeight - backwardHeight;
                        curr = -Math.atan2(diff, 2)*180/Math.PI;
                        factor *= 3;
                    } else {
                        // In the air
                        curr = clamp((this.velocity.z + this.falling.z)*1, -35, 35);
                    }

                    factor = clamp(factor, 0, 1);
                    return factor*curr + old*(1-factor);
                }

                this.floorOrientation.pitch = calcFloorStuff.apply(this, [0, this.floorOrientation.pitch]);
// TODO: When can show with roll                this.floorOrientation.roll = calcFloorStuff(90, this.floorOrientation.roll);
            },
        },

        limitYawing: {
            clientAct: function(seconds) {
                if (this === getPlayerEntity() && this.clientState !== CLIENTSTATE.EDITING) {
                    if (this.oldYaw === undefined) {
                        this.oldYaw = this.yaw;
                    }

                    this.oldYaw = normalizeAngle(this.oldYaw, this.yaw);
                    if (Math.abs(this.oldYaw - this.yaw) > this.facingSpeed*seconds) {
                        this.yaw = moveAngleTowards(this.oldYaw, this.yaw, this.facingSpeed*seconds);
                    }
                }
            },
        },
    },
};

