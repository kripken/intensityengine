
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
        setRotation: function(entity, v) { log(ERROR, "TODO: Set rotation"); /* CAPI.physicsSetBodyRotation(entity.physicsHandle, v[0], v[1], v[2], [3]); */ },
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

            renderPhysical: function() {
                var flags = MODEL.LIGHT | MODEL.DYNSHADOW;

                // Optimized version
                var o = CAPI.physicsGetBodyPosition(this.physicsHandle);
                var r = CAPI.physicsGetBodyRotation(this.physicsHandle);
                var args = [this, 'box', ANIM_IDLE|ANIM_LOOP, o[0], o[1], o[2], 0, 0, 0, flags, 0, r[0], r[1], r[2], r[3]];

//                // Normal version
//                var o = this.position;
//                var r = this.rotation;
//                var args = [this, 'box', ANIM_IDLE|ANIM_LOOP, o.x, o.y, o.z, 0, 0, 0, flags, 0, r.x, r.y, r.z, r.w];
                CAPI.renderModel3.apply(this, args);
            },
        },

        playerPlugin: {
            createPhysicalObject: function() {
                return CAPI.physicsAddBox(10, 20, 20, 20);
//                return CAPI.physicsAddSphere(10, 10);
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

                    if (this.move) {
                        velocity.add(new Vector3().fromYawPitch(this.yaw, !editing ? 0 : this.pitch).mul(seconds*speed*this.move));
                    }
                    if (this.strafe) {
                        velocity.add(new Vector3().fromYawPitch(this.yaw-90, 0).mul(seconds*speed*this.strafe));
                    }
                    if (this.move || this.strafe) {
                        if (editing) {
                            position.add(velocity.mulNew(speed*seconds));
                        }
                    }

                    this.lastPosition = this.position.copy();
                }

                this.position = position;
                this.velocity = velocity;
            },

            jump: function() {
//                if (this.isOnFloor() || World.getMaterial(this.position) === MATERIAL.WATER) {
                    this.velocity.z += this.movementSpeed*2;
//                }
            },

            renderDynamic: function() {
                this.renderPhysical();
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
            this.renderPhysical();
        },
    },
]));

