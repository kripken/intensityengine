
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

            // Patch projectiles to run on physics engine
            if (Projectiles) {
                var old = Projectiles.Projectile.prototype.create;
                Projectiles.Projectile.prototype.create = function() {
                    old.apply(this, arguments);

                    // Create physics body
                    this.physicsHandle = CAPI.physicsAddSphere(1, this.radius);
                    Physics.Engine.setPosition(this, this.position.asArray());
                    Physics.Engine.setVelocity(this, this.velocity.asArray());
                },

                Projectiles.Projectile.prototype.destroy = function() {
                    // Remove physics body
                    Physics.Engine.teardownPhysicalEntity(this);
                };

                Projectiles.Projectile.prototype.tick = function(seconds) {
                    this.timeLeft -= seconds;
                    if (this.timeLeft < 0) {
                        return false;
                    }

                    // Check for changes in velocity. In z, maybe just gravity, ignore. Otherwise - explode.
                    var oldVelocity = this.velocity.copy();
                    this.position = new Vector3(Physics.Engine.getPosition(this));
                    this.velocity = new Vector3(Physics.Engine.getVelocity(this));
                    this.velocity.z = oldVelocity.z; // Do not care about gravity
                    Physics.Engine.setVelocity(this, this.velocity.asArray());

                    var diff = this.velocity.subNew(oldVelocity).magnitude();
                    if (diff >= 10*seconds) return this.onExplode();
                    return true;
                };

                Projectiles.Projectile.prototype.shooterSafety = 2.0;

            }
        },

        setupPhysicalEntity: function(entity) {
            Global.queuedActions.push(bind(function() {
                entity.physicsHandle = entity.createPhysicalObject();
                CAPI.physicsSetBodyEntity(entity.physicsHandle, entity.uniqueId);

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
            }, this));
        },

        teardownPhysicalEntity: function(entity) {
            if (entity.physicsHandle !== undefined) {
                CAPI.physicsRemoveBody(entity.physicsHandle);
            }
        },

        getPosition: function(entity) {
            return (entity.physicsHandle !== undefined) ? CAPI.physicsGetBodyPosition(entity.physicsHandle) : Vector3.zero;
        },
        getRotation: function(entity) {
            return (entity.physicsHandle !== undefined) ? CAPI.physicsGetBodyRotation(entity.physicsHandle) : Vector4.zero;
        },
        getVelocity: function(entity) {
            return (entity.physicsHandle !== undefined) ? CAPI.physicsGetBodyVelocity(entity.physicsHandle) : Vector3.zero;
        },

        setPosition: function(entity, v) {
            if (entity.physicsHandle !== undefined) {
                CAPI.physicsSetBodyPosition(entity.physicsHandle, v[0], v[1], v[2]);
            } else {
                Global.queuedActions.push(partial(arguments.callee, entity, v));
            }
        },
        setRotation: function(entity, v) {
            if (entity.physicsHandle !== undefined) {
                CAPI.physicsSetBodyRotation(entity.physicsHandle, v[0], v[1], v[2], v[3]);
            } else {
                Global.queuedActions.push(partial(arguments.callee, entity, v));
            }
        },
        setVelocity: function(entity, v) {
            if (entity.physicsHandle !== undefined) {
                CAPI.physicsSetBodyVelocity(entity.physicsHandle, v[0], v[1], v[2]);
            } else {
                Global.queuedActions.push(partial(arguments.callee, entity, v));
            }
        },

        addImpulse: function(entity, v) {
            if (entity.physicsHandle !== undefined) {
                CAPI.physicsAddBodyImpulse(entity.physicsHandle, v[0], v[1], v[2]);
            }
        },

        objectPlugin: {
            position: new WrappedCVector3({ cGetter: 'Physics.Engine.getPosition', cSetter: 'Physics.Engine.setPosition', customSynch: true }),
            rotation: new WrappedCVector4({ cGetter: 'Physics.Engine.getRotation', cSetter: 'Physics.Engine.setRotation', customSynch: true }),
            velocity: new WrappedCVector3({ cGetter: 'Physics.Engine.getVelocity', cSetter: 'Physics.Engine.setVelocity', customSynch: true }),

            mass: new StateFloat(),

            createPhysicalObject: function() {
                return CAPI.physicsAddBox(this.mass, 20, 20, 20);
            },
            init: function() {
                this.mass = 40;
            },
            activate: function() { Physics.Engine.setupPhysicalEntity(this); },
            deactivate: function() { Physics.Engine.teardownPhysicalEntity(this); },
            clientActivate: function() { Physics.Engine.setupPhysicalEntity(this); },
            clientDeactivate: function() { Physics.Engine.teardownPhysicalEntity(this); },

            renderPhysical: function(modelName, offset, animation) {
                if (this.physicsHandle === undefined) return;

                var flags = MODEL.LIGHT | MODEL.DYNSHADOW;

                // Optimized version
                var o = CAPI.physicsGetBodyPosition(this.physicsHandle);
                if (offset) { o[0] += offset.x; o[1] += offset.y; o[2] += offset.z; }
                var r = CAPI.physicsGetBodyRotation(this.physicsHandle);
                var args = [this, modelName, defaultValue(animation, ANIM_IDLE|ANIM_LOOP), o[0], o[1], o[2], 0, 0, 0, flags, 0, r[0], r[1], r[2], r[3]];

//                // Normal version
//                var o = this.position;
//                var r = this.rotation;
//                var args = [this, modelName, ANIM_IDLE|ANIM_LOOP, o.x, o.y, o.z, 0, 0, 0, flags, 0, r.x, r.y, r.z, r.w];
                CAPI.renderModel3.apply(this, args);
            },
        },

        playerPlugin: {
            init: function() {
                this.mass = 10;
            },

            createPhysicalObject: function() {
                // Typical character setup
                var ret = CAPI.physicsAddCapsule(this.mass, this.radius, this.aboveEye+this.eyeHeight-this.radius*2);
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
                if (this.physicsHandle === undefined) return;

                if (this === getPlayerEntity()) {
                    // We control ourselves here, locally

                    var position = this.position.copy();
                    var velocity = this.velocity.copy();

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

                    if (!editing && Health.isActiveEntity(this)) {
                        var flatVelocity = velocity.copy();
                        flatVelocity.z = 0;
                        var cap = speed;
                        targetVelocity.sub(flatVelocity).cap(cap).mul(seconds*3*this.mass);
                        CAPI.physicsAddBodyImpulse(this.physicsHandle, targetVelocity.x, targetVelocity.y, targetVelocity.z);
                    }

                    this.lastPosition = this.position.copy();

                    this.position = position;
                    this.rotation = new Vector4().quatFromAxisAngle(new Vector3(0,0,1), this.yaw-90);
                } else {
                    // Other clients, we read the C++ data and feed that into Bullet
                    Physics.Engine.setPosition(this, CAPI.getDynentO(this));
                    Physics.Engine.setVelocity(this, CAPI.getDynentVel(this));
                    Physics.Engine.setRotation(this, new Vector4().quatFromAxisAngle(new Vector3(0,0,1), this.yaw-90).asArray());
                }
            },

            jump: function() {
                if (!Health.isActiveEntity(this)) return;
//                if (this.isOnFloor() || World.getMaterial(this.position) === MATERIAL.WATER) {
//                    this.velocity.z += this.movementSpeed*2;
                        var delta = Global.currTimeDelta;
                        CAPI.physicsAddBodyImpulse(this.physicsHandle, 0, 0, this.mass*this.movementSpeed);

//                }
            },

            createRenderingArgs: function(mdlname, anim, o, yaw, pitch, flags, basetime) {
                var r;
                if (this.physicsHandle !== undefined) {
                    r = CAPI.physicsGetBodyRotation(this.physicsHandle);
                } else {
                    r = [0,0,0,1];
                    mdlname = '';
                }
                return [this, mdlname, anim, o.x, o.y, o.z - (this.aboveEye+this.eyeHeight)/2, 0, 0, 0, flags, basetime, r[0], r[1], r[2], r[3]];
            },

            getRenderModelFunc: function() {
                return CAPI.renderModel3;
            },
        },
    },
};

//! A generic physics engine entity - no effort is done to synchronize
//! positions etc., so basically a separate physical body is being
//! run on each client and the server.
Physics.Engine.Entity = registerEntityClass(bakePlugins(LogicEntity, [
    Physics.Engine.objectPlugin,
    {
        _class: 'PhysicsEngineEntity',
        _sauerType: '',

        radius: new StateFloat(),

        init: function(uniqueId, kwargs) {
            this.position = new Vector3(kwargs.position.x, kwargs.position.y, kwargs.position.z);
            this.velocity = new Vector3(0, 0, 0);
            this.rotation = new Vector4(0, 0, 0, 1);
            this.radius = 5;
        },

        renderDynamic: function() {
            this.renderPhysical('box');
        },

        // TODO: getCenter and getBottom functions, in general in the API?
        getCenter: function() { return this.position.copy(); },

        sufferDamage: function() { }, //!< Necessary so explosions etc. affect us
    },
]));

//! A physics engine entity that runs on the server - the server is
//! authoritative for it. Positions etc. are synced to the client
Physics.Engine.ServerEntity = registerEntityClass(bakePlugins(Physics.Engine.Entity, [{
    _class: 'PhysicsEngineServerEntity',

    positionUpdate: new StateArrayFloat({ reliable: false, hasHistory: false }),
    positionUpdateRate: 1/10,
    positionUpdatePower: 1000,

    activate: function() {
        GameManager.getSingleton().eventManager.add({
            secondsBefore: 0,
            secondsBetween: this.positionUpdateRate,
            func: bind(function() {
                this.positionUpdate = this.position.asArray().
                    concat(this.velocity.asArray()).
                    concat(this.rotation.asArray());
            }, this),
            entity: this,
        });
    },

    clientActivate: function() {
        this.connect('client_onModify_positionUpdate', function(data) {
            var position = data.slice(0, 3);
            var velocity = data.slice(3, 6);
            var rotation = data.slice(6, 10);
            var alpha = this.positionUpdatePower*Global.currTimeDelta*this.positionUpdateRate;
            Physics.Engine.setPosition(this, new Vector3(position).lerp(this.position, alpha).asArray());
//            Physics.Engine.setPosition(this, position);
            Physics.Engine.setVelocity(this, velocity);
//            Physics.Engine.setRotation(this, new Vector4(rotation).lerp(this.rotation, alpha).asArray());
/*
            var dir = new Vector3(position).subNew(this.position);
            var mag = dir.magnitude();
            var impulse = dir.normalize().mul(
                Math.pow(mag, 2) * this.positionUpdatePower * this.mass * Global.currTimeDelta/this.positionUpdateRate
            );
            if (!impulse.isZero()) {
                Physics.Engine.addImpulse(this, impulse.asArray());
            }
*/
        });
    },
}]));

// Restart map fails XXX

