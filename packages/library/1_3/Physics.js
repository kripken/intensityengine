
//=============================================================================
// Copyright (C) 2008 Alon Zakai ('Kripken') kripkensteiner@gmail.com
//
// This file is part of the Intensity Engine project,
//    http://www.intensityengine.com
//
// The Intensity Engine is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published by
// the Free Software Foundation, version 3.
//
// The Intensity Engine is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with the Intensity Engine.  If not, see
//     http://www.gnu.org/licenses/
//     http://www.gnu.org/licenses/agpl-3.0.html
//=============================================================================


Physics = {
    EPSILON: 0.1,

    constraints: {
        Distance: Class.extend({
            create: function(position, distance) {
                this.position = position;
                this.distance = distance;
            },

            tick: function(projectile, seconds) {
                var direction = projectile.position.subNew(this.position);
                var diff = direction.magnitude() - this.distance;
                if (Math.abs(diff) < Physics.EPSILON) return false;

                direction.normalize();
                projectile.position = this.position.addNew(direction.mulNew(this.distance));
                projectile.velocity.add(direction.mulNew(-diff/Math.max(seconds, 0.01))).mul(clamp(1 - seconds, 0, 1)); // Final mul - add dampening

                return false;
            },
        }),
    },

    plugins: {
        ConstrainedProjectile: {
            create: function() {
                this._super.apply(this, arguments);

                this.constraints = [];
            },

            tick: function(seconds) {
                var ret = this._super.apply(this, arguments);

                // Apply constraints in a loop, until none of them act - then we are done
                var acted = true;
                var times = 0;
                var MAX_TIMES = 5;
                while (acted && times < MAX_TIMES) {
                    acted = false;
                    forEach(this.constraints, function(constraint) {
                        acted |= constraint.tick(this, seconds);
                    }, this);
                    times += 1;
                }
                if (times === MAX_TIMES) {
                    log(WARNING, 'Physics constraints did not work themselves out.');
                }

                return ret;
            },
        },
    },

    Engine: {
        setupPhysicalEntity: function(entity) {
            entity.physicsHandle = entity.createPhysicalObject();

            entity.connect((Global.CLIENT ? 'client_' : '') + 'onModify_position', function(position) {
                position = new Vector3(position);
                CAPI.physicsSetDynamicPosition(entity.physicsHandle, position.x, position.y, position.z);
            });

            entity.connect((Global.CLIENT ? 'client_' : '') + 'onModify_velocity', function(velocity) {
                velocity = new Vector3(velocity);
//log(ERROR, "mod vel: " + velocity);
                CAPI.physicsSetDynamicVelocity(entity.physicsHandle, velocity.x, velocity.y, velocity.z);
            });
        },

        teardownPhysicalEntity: function(entity) {
            CAPI.physicsRemoveDynamic(entity.physicsHandle);
        },

        objectPlugin: {
            createPhysicalObject: function() {
                return CAPI.physicsAddDynamicBox(1, 10, 10, 10);
            },
            activate: function() { Physics.Engine.setupPhysicalEntity(this); },
            deactivate: function() { Physics.Engine.teardownPhysicalEntity(this); },
            clientActivate: function() { Physics.Engine.setupPhysicalEntity(this); },
            clientDeactivate: function() { Physics.Engine.teardownPhysicalEntity(this); },
        },

        playerPlugin: {
            createPhysicalObject: function() {
                return CAPI.physicsAddDynamicSphere(1, 10);
            },
            clientAct: function(seconds) {
                var data = CAPI.physicsGetDynamic(this.physicsHandle);
                this.position = data.position;
                this.velocity = data.velocity;

                if (this === getPlayerEntity()) {
                    var speed = this.movementSpeed*2;
                    var editing = isPlayerEditing(this);

                    if (editing) {
                        this.position = this.lastPosition ? this.lastPosition : this.position;
                        this.velocity.mul(0);
                    }

                    if (this.move) {
                        this.velocity.add(new Vector3().fromYawPitch(this.yaw, !editing ? 0 : this.pitch).mul(seconds*speed*this.move));
                    }
                    if (this.strafe) {
                        this.velocity.add(new Vector3().fromYawPitch(this.yaw-90, 0).mul(seconds*speed*this.strafe));
                    }
                    if (this.move || this.strafe) {
                        if (!editing) {
                            CAPI.physicsSetDynamicVelocity(this.physicsHandle, this.velocity.x, this.velocity.y, this.velocity.z);
                        } else {
                            this.position.add(this.velocity.mulNew(speed*seconds));
                        }
                    }

                    if (editing) { this.lastPosition = this.position.copy(); }
                }
            },

            jump: function() {
//                if (this.isOnFloor() || World.getMaterial(this.position) === MATERIAL.WATER) {
                    this.velocity.z += this.movementSpeed;
//                }
            },
        },
    },
};

Physics.Engine.Entity = registerEntityClass(bakePlugins(LogicEntity, [
    Physics.Engine.objectPlugin,
    {
        _class: 'PhysicsEngineEntity',
        _sauerType: '',

        position: new StateVector3(),
        radius: new StateFloat(),

        init: function(uniqueId, kwargs) {
log(ERROR, "init:" + serializeJSON(kwargs));
            this.position = new Vector3(kwargs.position.x, kwargs.position.y, kwargs.position.z);
log(ERROR, "pos:" + this.position + ',' + kwargs.position + ',' + new Vector3(kwargs.position.x, kwargs.position.y, kwargs.position.z));
            this.radius = 5;
        },

        activate: function(kwargs) {
log(ERROR, "activate");
        },
    },
]));

