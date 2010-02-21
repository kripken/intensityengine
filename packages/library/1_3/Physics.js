
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
        create: function(type) {
            CAPI.physicsCreateEngine(type);
        },

        setupPhysicalEntity: function(entity) {
            entity.physicsHandle = entity.createPhysicalObject();

            if (entity instanceof Character) {
                entity.connect((Global.CLIENT ? 'client_' : '') + 'onModify_position', function(position) {
                    position = new Vector3(position);
                    CAPI.setDynentO(this, position);
                });

                entity.connect((Global.CLIENT ? 'client_' : '') + 'onModify_velocity', function(velocity) {
                    velocity = new Vector3(velocity);
                    CAPI.setDynentVel(this, velocity);
                });
            }
        },

        teardownPhysicalEntity: function(entity) {
            CAPI.physicsRemoveBody(entity.physicsHandle);
        },

        getPosition: function(entity) { return CAPI.physicsGetBodyPosition(entity.physicsHandle); },
        getRotation: function(entity) { return CAPI.physicsGetBodyRotation(entity.physicsHandle); },
        getVelocity: function(entity) { return CAPI.physicsGetBodyVelocity(entity.physicsHandle); },

        setPosition: function(entity, v) { // TODO: queue this and other setX things, if no handle yet
            if (entity.physicsHandle !== undefined) CAPI.physicsSetBodyPosition(entity.physicsHandle, v[0], v[1], v[2]);
        },
        setRotation: function(entity, v) {
            if (entity.physicsHandle !== undefined) CAPI.physicsSetBodyRotation(entity.physicsHandle, v[0], v[1], v[2], v[3]);
        },
        setVelocity: function(entity, v) {
            if (entity.physicsHandle !== undefined) CAPI.physicsSetBodyVelocity(entity.physicsHandle, v[0], v[1], v[2]);
        },

        objectPlugin: {
            position: new WrappedCVector3({ cGetter: 'Physics.Engine.getPosition', cSetter: 'Physics.Engine.setPosition', customSynch: true }),
            rotation: new WrappedCVector4({ cGetter: 'Physics.Engine.getRotation', cSetter: 'Physics.Engine.setRotation', customSynch: true }),
            velocity: new WrappedCVector3({ cGetter: 'Physics.Engine.getVelocity', cSetter: 'Physics.Engine.setVelocity', customSynch: true }),

            createPhysicalObject: function() {
                return CAPI.physicsAddBox(1, 20, 20, 20);
            },
            activate: function() { Physics.Engine.setupPhysicalEntity(this); },
            deactivate: function() { Physics.Engine.teardownPhysicalEntity(this); },
            clientActivate: function() { Physics.Engine.setupPhysicalEntity(this); },
            clientDeactivate: function() { Physics.Engine.teardownPhysicalEntity(this); },

            renderPhysical: function(modelName, offset) {
                var flags = MODEL.LIGHT | MODEL.DYNSHADOW;

                // Optimized version
                var o = CAPI.physicsGetBodyPosition(this.physicsHandle);
                if (offset) { o[0] += offset.x; o[1] += offset.y; o[2] += offset.z; }
                var r = CAPI.physicsGetBodyRotation(this.physicsHandle);
                var args = [this, modelName, ANIM_IDLE|ANIM_LOOP, o[0], o[1], o[2], 0, 0, 0, flags, 0, r[0], r[1], r[2], r[3]];

//                // Normal version
//                var o = this.position;
//                var r = this.rotation;
//                var args = [this, modelName, ANIM_IDLE|ANIM_LOOP, o.x, o.y, o.z, 0, 0, 0, flags, 0, r.x, r.y, r.z, r.w];
                CAPI.renderModel3.apply(this, args);
            },
        },

        playerPlugin: {
            createPhysicalObject: function() {
                // Typical character setup
                var ret = CAPI.physicsAddCapsule(10, this.radius, this.aboveEye+this.eyeHeight-this.radius*2);
                CAPI.physicsSetAngularFactor(ret, 0, 0, 1);

//                var ret = CAPI.physicsAddBox(10, 25, 15, 10);
////                var ret = CAPI.physicsAddSphere(10, 10);
//
//var other;
//other = CAPI.physicsAddSphere(1, 5);
////CAPI.physicsSetAngularFactor(other, 0, 0, 0);
//CAPI.physicsAddConstraintP2P(ret, other, 15, 15, -5, 0, 0, 0);
//other = CAPI.physicsAddSphere(1, 5);
////CAPI.physicsSetAngularFactor(other, 0, 0, 0);
//CAPI.physicsAddConstraintP2P(ret, other, 15, -15, -5, 0, 0, 0);
//other = CAPI.physicsAddSphere(1, 5);
////CAPI.physicsSetAngularFactor(other, 0, 0, 0);
//CAPI.physicsAddConstraintP2P(ret, other, -15, 15, -5, 0, 0, 0);
//other = CAPI.physicsAddSphere(1, 5);
////CAPI.physicsSetAngularFactor(other, 0, 0, 0);
//CAPI.physicsAddConstraintP2P(ret, other, -15, -15, -5, 0, 0, 0);

                return ret;
            },
            clientActivate: function() {
                this.lastPosition = new Vector3(0, 0, 0);
            },

            clientAct: function(seconds) {
                var position = this.position.copy();
                var velocity = this.velocity.copy();

                if (this === getPlayerEntity()) {
                    var speed = this.movementSpeed*2;
                    var editing = isPlayerEditing(this);

                    if (editing) {
                        position = this.lastPosition ? this.lastPosition : this.position;
                        velocity.mul(0);
                    }

                    var targetVelocity = new Vector3(0,0,0);

                    if (this.move) {
                        targetVelocity.add(new Vector3().fromYawPitch(this.yaw, !editing ? 0 : this.pitch).mul(speed*this.move));
                    }
                    if (this.strafe) {
                        targetVelocity.add(new Vector3().fromYawPitch(this.yaw-90, 0).mul(speed*this.strafe));
                    }
                    if (this.move || this.strafe) {
                        if (editing) {
                            position.add(targetVelocity.mulNew(seconds));
                        }
                    }

                    if (!editing) {
                        var flatVelocity = velocity.copy();
                        flatVelocity.z = 0;
                        var cap = speed;
                        targetVelocity.sub(flatVelocity).cap(cap).mul(seconds*30);
                        CAPI.physicsAddBodyImpulse(this.physicsHandle, targetVelocity.x, targetVelocity.y, targetVelocity.z);
                    }

                    this.lastPosition = this.position.copy();
                }

                this.position = position;
                this.rotation = new Vector4().quatFromAxisAngle(new Vector3(0,0,1), this.yaw-90);
            },

            jump: function() {
//                if (this.isOnFloor() || World.getMaterial(this.position) === MATERIAL.WATER) {
                    this.velocity.z += this.movementSpeed*2;
//                }
            },

            renderDynamic: function() {
                // Appropriate for a capsule
                this.renderPhysical('stromar', new Vector3(0, 0, - (this.aboveEye+this.eyeHeight)/2));
            },
        },
    },
};

Physics.Engine.Entity = registerEntityClass(bakePlugins(LogicEntity, [
    Physics.Engine.objectPlugin,
    {
        _class: 'PhysicsEngineEntity',
        _sauerType: '',

        radius: new StateFloat(),

        init: function(uniqueId, kwargs) {
            this.position = new Vector3(kwargs.position.x, kwargs.position.y, kwargs.position.z);
            this.radius = 5;
        },

        renderDynamic: function() {
            this.renderPhysical('box');
        },
    },
]));

