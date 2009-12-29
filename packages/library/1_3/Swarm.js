
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


Library.include('library/' + Global.LIBRARY_VERSION + '/Projectiles'); // For swarmbug ragdoll
Library.include('library/' + Global.LIBRARY_VERSION + '/mapelements/EditingTools');


//! Swarms are containers for large amounts of 'subentities' (aka swarm entities) -
//! things that are in large amounts, or high degrees of transience, or that
//! are managed differently on the server/clients (maybe just done on the server,
//! or on one client - that is, not synched like a normal LogicEntity everywhere).
//! Basically, swarms are used for situations where it doesn't make sense to use a
//! separate LogicEntity for each. Instead, Swarms can define
//! their own protocols for subentity creation, destruction, and so forth.
//!
//! Swarm entities are physical: they have a position, velocity and radius.
//!
//! Usage: In the LogicEntity managing the swarm, add the Swarms.plugin. Then do
//!           this.swarmManager.add(new Swarms.SwarmEntity(position, ...));
//!        (or use a subclass of SubEntity).
//!
Swarms = {
    SwarmEntity: Class.extend({
        physicsFrameSize: 0.02, //!< Default is 50fps. If you have many subentities, consider tweaking this
        secondsLeft: null, //!< If set, will count down and then remove the subentity
        gravity: 1, //!< 1 means normal gravity, 0 means no gravity
        radius: 1.0,

        create: function(kwargs) {
            this.position = kwargs.position.copy();
            this.velocity = defaultValue(kwargs.velocity, new Vector3(0, 0, 0));

            this.physicsFrameTimer = new RepeatingTimer(this.physicsFrameSize, true);

            this.active = true;

            var that = this;
            this.__defineGetter__('center', function() { return that.position; });

            log(DEBUG, "swarmEntity created at " + this.position);
        },

        //! Called after added. Override if you want
        onAdd: function() {
        },

        onRemove: function() {
        },

        //! Returns true if the entity is to continue its life
        tick: function(seconds) {
            if (this.secondsLeft !== null) {
                this.secondsLeft -= seconds;
                if (this.secondsLeft < 0) {
                    return false;
                }
            }

            var firstTick = this.physicsFrameTimer.tick(seconds); // Tick once with entire time, then
                                                                  // tick with 0's to pick off all frames
            while (firstTick || this.physicsFrameTimer.tick(0)) {
                firstTick = false;

               if (!this.tickFrame(this.physicsFrameSize)) return false;
            }
            return true;
        },

        //! Tick one physics frame. Usually seconds is this.physicsFrameSize, but use 'seconds' - it might differ
        tickFrame: function(seconds) {
            if (this.gravity) {
                this.velocity.z -= World.gravity*this.gravity*seconds;
            }

            var lastPosition = this.position.copy();
            if (this.physicsFunc) {
                // Let custom physics function handle actual movement and bouncing etc.
                this.physicsFunc(seconds);
            } else {
                // Trivial physics
                this.position.add( this.velocity.mulNew(seconds) );
            }

            // If after physics we are still colliding (the physicsFunc should bounce us off things, keep us off the floor, etc,
            // so we aren't), then the collision is fundamental and unavoidable - call a handler.
            if ( World.isColliding(this.position, this.radius, this.ignore) ) {
                return this.handleCollision(lastPosition);
            }
        },

        //! Override with nicer rendering. Also you can override renderDynamic, which can render models etc.
        render: function() {
            Effect.fireball(PARTICLE.EXPLOSION_NO_GLARE, this.position, this.radius, 0.05, this.color, this.radius);
            Effect.addDynamicLight(this.position, this.radius*9, this.color);
        },

        //! An unavoidable collision happened. We receive the last position as well, so by default we can fine-tune where it happened
        handleCollision: function(lastPosition) {
            // Do some fine-tuning of the collision position - small steps. Not doing this all the time
            // saves CPU, and would only lead to bugs if there are really really thin obstacles.
            var currPosition = lastPosition.copy();
            var NUM_STEPS = 5;
            var step = this.velocity.mulNew(this.physicsFrameSize/NUM_STEPS);
            for (var i = 0; i < NUM_STEPS; i++) {
                currPosition.add(step);
                // Don't do last step - we already know the answer
                if ( World.isColliding(currPosition, this.radius, this.ignore) ) {
                    break;
                }
            }
            this.position = currPosition;
            return this.handleSpecificCollision();
        },

        //! Called by handleCollision after it calculates a more specific collision point. Override this to
        //! show an explosion or whatever
        handleSpecificCollision: function() {
            return false;
        },
    }),

    Manager: Class.extend({
        create: function(kwargs) {
            this.parent = kwargs.parent;

            this.subEntities = [];
        },

        add: function(subEntity) {
            this.subEntities.push(subEntity);
            subEntity.manager = this;
            subEntity.onAdd();
        },

        tick: function(seconds) {
            log(DEBUG, "swarmManager.tick() beginning");

            var removed = [];
            this.subEntities = filter(function(subEntity) {
                if (!subEntity.active || !subEntity.tick(seconds)) {
                    removed.push(subEntity);
                    return false;
                } else {
                    return true;
                }
            }, this.subEntities);

            forEach(removed, function(removedSubEntity) {
                removedSubEntity.onRemove();
            });

            log(DEBUG, "swarmManager.tick() complete");
        },

        render: function() {
            forEach(this.subEntities, function(subEntity) { if (subEntity.render) subEntity.render(); });
        },

        renderDynamic: function() {
            forEach(this.subEntities, function(subEntity) { if (subEntity.renderDynamic) subEntity.renderDynamic(); });
        },
    }),

    plugin: {
        activate: function() {
            this.swarmManager = new Swarms.Manager({ parent: this });
        },

        clientActivate: function() {
            this.swarmManager = new Swarms.Manager({ parent: this });
        },

        act: function(seconds) {
            if (this.swarmManager.subEntities.length === 0) return;

            this.swarmManager.tick(seconds);
        },

        clientAct: function(seconds) {
            if (this.swarmManager.subEntities.length === 0) return;

            this.swarmManager.tick(seconds);
            this.swarmManager.render();
        },

        renderDynamic: function(HUDPass, needHUD) {
            if (this.swarmManager.subEntities.length === 0) return;

            if (!HUDPass) {
                this.swarmManager.renderDynamic();
            }
        },
    },
};


// Examples

SwarmBugs = {
    SYNC_RATE: 0.5,
    MAX_CLIENT_SYNC: 0.75, // Slightly more then SYNC_RATE
    MAX_CLIENT_SURVIVE: 4.0, //

    plugin: {
        // Protocol: id, x, y, z, targetEntity
        swarmBugUpdate: new StateArrayFloat({ reliable: false, hasHistory: false }),
        swarmBugClientDeath: new StateInteger({ reliable: false, hasHistory: false }),
        swarmBugClientPain: new StateInteger({ reliable: false, hasHistory: false }),

        activate: function() {
            this.swarmManager.ids = {};
            this.swarmManager.idCounter = 1;

            this.connect('onModify_swarmBugClientDeath', function(value) {
                var bug = this.swarmManager.ids[value];
                if (bug) bug.die();
                throw "CancelStateDataUpdate";
            });

            this.connect('onModify_swarmBugClientPain', function(value) {
                var bug = this.swarmManager.ids[value];
                if (bug) bug.sufferPain();
                throw "CancelStateDataUpdate";
            });

            this.setupSystem();
        },

        clientActivate: function() {
            this.swarmManager.ids = {};

            this.connect('client_onModify_swarmBugUpdate', function(value) {
                if (value.length < 5) return;
                var subEntityId = value[0];
                var position = new Vector3(value[1], value[2], value[3]);
                var targetEntity = getEntity(value[4]);
                var rangedAttack = value[5];

                // If we recently killed this one, do not let it be resurrected right afterwards
                if (this.swarmManager.ids[subEntityId] instanceof RepeatingTimer) {
                    // Notify server to remove it
                    this.swarmBugClientDeath = subEntityId;

                    return;
                }

                var bug;
                // Add new bug if necessary
                if (!this.swarmManager.ids[subEntityId]) {
                    bug = new SwarmBugs.SwarmBug({
                        position: position,
                    });
                    this.swarmManager.add(bug);
                    this.swarmManager.ids[subEntityId] = bug;
                    bug.subEntityId = subEntityId;
                } else {
                    bug = this.swarmManager.ids[subEntityId];
                }

                // Update bug
                bug.positionUpdate = position;
//                if (bug.positionUpdate.subNew(bug.position).magnitude() > bug.radius*5) {
//                    bug.die();
//                }
                bug.positionUpdateTimer = 0;
                bug.target = targetEntity;

                if (rangedAttack) {
                    bug.rangedAttacking = true;
                    bug.rangedHitting = rangedAttack < bug.rangedHitProbability;
                    bug.rangedAttackingTimer = new RepeatingTimer(bug.rangedAttackDuration);
                    bug.basetime = Global.lastmillis ? Global.lastmillis : 0;
                }
            });

            this.setupSystem();
        },

        clientAct: function(seconds) {
            if (this.swarmManager.subEntities.length === 0) return;

            // Watches for recently killed swarmers
            forEach(keys(this.swarmManager.ids), function(id) {
                if (this.swarmManager.ids[id] instanceof RepeatingTimer && this.swarmManager.ids[id].tick(seconds)) {
                    this.swarmManager.ids[id] = null;
                }
            }, this);
        },

        // Sets up physics etc. to take into account swarm bugs
        // WARNING XXX: Once done, cannot be undone. If you remove this entity, restart the map.
        // In particular, you may crash as clients will still try to harm your botters, and pain updates will
        // crash since you are deactivated.
        setupSystem: function() {
            // Collision physics

            var that = this;

            // Weapons and blast waves - TODO

            var oldDoBlastWave = Projectiles.doBlastWave;
            Projectiles.doBlastWave = function(position, power, velocity, customDamageFunc) {
                oldDoBlastWave.apply(Projectiles, arguments);

                var expo = 1.333;
                var maxDist = Math.pow(power-1, 1/expo);

                if (!customDamageFunc) {
                    forEach(that.swarmManager.subEntities, function (subEntity) {
                        var distance = subEntity.position.subNew(position).magnitude();
                        distance = Math.max(1, distance);
                        var bump = Math.max(0, power-Math.pow(distance, expo));

                        subEntity.sufferDamage({ damage: bump*10, origin: position, nonControllerDamage: 0 });
                    });
                }
            }

            // Entity collisions

            var oldGetCollidableEntities = World.getCollidableEntities;
            World.getCollidableEntities = function() {
                return oldGetCollidableEntities().concat(that.swarmManager.subEntities);
            }
        },
    },

    //! Walks on any surface it can find
    SwarmBug: Swarms.SwarmEntity.extend({
        radius: 6.0,
        speed: 70.0,
        gravity: 1.0,
        attachmentRadius: 7.0, // How far we are from attached surfaces
        attachmentError: 1.0,
        personalSpace: 12.5,
        physicsFrameSize: 1/50,
        desiredTargetDistance: 10,
        searchRadius: 200,
        rangedAttackProbability: 0.66,
        rangedHitProbability: 0.1,
        minRangedDistance: 30,
        maxRangedDistance: 60,
        rangedAttackDuration: 0.66,

        create: function(kwargs) {
            this._super.apply(this, arguments);

            this.target = kwargs.target;

            //! The current surface normal, or null if none
            this.surface = defaultValue(kwargs.surface, null);

            this.surfaceTimer = new RepeatingTimer(1/5);

            this.neighborTimer = new RepeatingTimer(1/10);
            this.neighbors = [];

            this.up = new Vector3(0,0,1); // When on a surface, usually equal to this.surface
            this.forward = Random.normalizedVector3().projectAlongSurface(this.up).normalize(); // Perpendicular to this.orientation. The direction we are facing
            this.targetDirection = new Vector3(0,1,0);

            this.model = 'gk/botter/' + Random.randint(0, 8);

            this.animations = {
                forward: Random.randint(49, 51),
                meleeAttack: Random.randint(52, 55),
                rangedAttack: 56,
                pain: Random.randint(41, 48),
            };

            this.health = 75;
            this.pain = 0;
        },

        onAdd: function() {
            if (Global.SERVER) {
                // Get ID
                this.subEntityId = this.manager.idCounter;
                this.manager.ids[this.subEntityId] = this;
                this.manager.idCounter += 1;
                if (this.manager.idCounter === 120) {
                    this.manager.idCounter = 0; // Always keep this number small so it compresses well. XXX Assumes < 120 bugs!
                }

                this.syncTimer = new RepeatingTimer(SwarmBugs.SYNC_RATE);
                this.syncTimer.prime();
                this.lastSyncedPosition = new Vector3(0,0,0);
            } else {
    //            this.manager.parent.swarmBugIds
                this.basetime = 0;

/*
                Effect.lightning(this.manager.parent.position, this.position, 1.0, 0xAABBFF, 2.0);
                Effect.addDynamicLight(this.position, this.radius*4, 0xAABBFF, 0.5, 0.25);
                Sound.play('swarm/artifact_lightning.ogg', this.position);
*/
            }
        },

        onRemove: function() {
        },

        handleCollision: function(lastPosition) {
            // Simple error handling: go back to safe position; zero the velocity.
            this.position = lastPosition;
            this.velocity = new Vector3(0, 0, 0);
            //log(ERROR, "handleCOLLISION");
            return true;
        },

        tickFrame: function(seconds) {
            this.oldPosition = this.position.copy();

            if (Global.CLIENT) {
                this.interpolate(seconds);
                if (this.positionUpdateTimer > SwarmBugs.MAX_CLIENT_SURVIVE) {
                    this.die(new Vector3(0,0,0), 1);
                    return false; // Not updated in too long, expire. Can be recreated anyhow later
                }
            }

            var target = this.think();
            this.targetDirection = target ? target.subNew(this.position) : null;
            this.targetDistance = target ? this.targetDirection.magnitude() : 0;

            if (target && this.targetDistance <= this.desiredTargetDistance+0.1 && !this.pain) {
                // Attack!
                if (!this.attacking) {
                    this.basetime = Global.lastmillis ? Global.lastmillis : 0;
                    this.attackingTimer = new RepeatingTimer(0.75); // 41 frame animations at 60fps
                    this.attackingTimer.tick(0.5); // head start
                    this.attacking = true;
                }
            } else {
                if (this.attacking) {
                    this.basetime = Global.lastmillis ? Global.lastmillis : 0;
                    this.attackingTimer = null;
                    this.attacking = false;
                }
            }

            if (this.attacking && Global.CLIENT && this.target === getPlayerEntity() && this.attackingTimer.tick(seconds)) {
                this.target.sufferDamage(5);
            }

            // Ranged attacks
            if (Global.SERVER) {
                if (target && !this.attacking && !this.rangedAttacking && Math.random() < seconds*this.rangedAttackProbability &&
                    this.targetDistance >= this.minRangedDistance && this.targetDistance <= this.maxRangedDistance) {
                    this.sendUpdate(Math.random());
                    this.rangedAttacking = true;
                    this.rangedAttackingTimer = new RepeatingTimer(this.rangedAttackDuration);
                }
            }
            if (this.target && this.rangedAttacking) {
                if (this.pain) {
                    this.rangedAttacking = false;
                    this.rangedAttackingTimer = null;
                } else if (this.rangedAttackingTimer.tick(seconds)) {
                    if (Global.CLIENT) {
                        var rangedOrigin = this.position.copy();
                        rangedOrigin.z += this.radius/3;
                        var rangedTarget = this.target.center;
                        if (hasLineOfSight(rangedOrigin, rangedTarget)) {
                            if (!this.rangedHitting) {
                                rangedTarget.add(Random.normalizedVector3().mul(clamp(Math.random()*13, 6, 13)));
                            } else if (this.target === getPlayerEntity()) {
                                this.target.sufferDamage(5);
                            }
                            Effect.flare(PARTICLE.STREAK, rangedOrigin, rangedTarget, 0.333, 0xE4FF4B, 0.6);
                            Effect.lightning(rangedOrigin, rangedTarget, 0.333, 0xA9FF33, 1.0);
                            Effect.splash(PARTICLE.SPARK, 15, 0.15, rangedTarget, 0xCCFF33, 1.0, 70, 1);
                        }
                    }
                    this.rangedAttacking = false;
                    this.rangedAttackingTimer = null;
                }
            }

            this.pain = Math.max(0, this.pain - seconds);

            // Where to test for a surface next
            var testAttachmentPositions = [];

            if (this.targetDirection) this.targetDirection.normalize();

            var factor = clamp(seconds*6, 0, 1);

            // Try to move in the direction of the target, if on a surface. Otherwise fall.
            if (this.surface) {
                // Debug printout of surface normal

    /*
                Effect.flare(
                    PARTICLE.STREAK,
                    this.position.addNew(this.surface.mulNew(-this.attachmentRadius*1)),
                    this.position.addNew(this.surface.mulNew(this.attachmentRadius*3)),
                    0, 0x257AFF, 1.0);
    */

                this.velocity = this.getSurfaceVelocity(seconds);

                if (this.justSpun) {
                    this.velocity.mul(0);
                }

                var newAttachmentPosition = this.position.subNew(this.surface.mulNew(this.attachmentRadius));
                testAttachmentPositions.push( newAttachmentPosition.add(this.velocity.mulNew(seconds)) );

                this.lastAttachmentPosition = newAttachmentPosition;

                // Update orientation
                this.up.mul(1-factor).add(this.surface.mulNew(factor));
            } else {
                this.velocity.mul(clamp(1-seconds, 0, 1)); // Friction
                this.velocity.z = clamp(this.velocity.z -World.gravity*this.gravity*seconds, -100, 100);

                // TODO: Also move in direction of target, even if in air

                this.lastAttachmentPosition = null;

                testAttachmentPositions.push( this.position.addNew(new Vector3(Math.random()-0.5, Math.random()-0.5, Math.random()-0.5).normalize().mul(this.attachmentRadius+0.5)) );

                // Update orientation
                this.up.mul(1-factor).add(new Vector3(0,0,factor));
            }

                if (target) {
                    this.forward.mul(1-factor).add(this.targetDirection.copy().normalize().mul(factor));
                }
                this.forward.projectAlongSurface(this.up).normalize();

            // Complete update of orientation
    /*
            Effect.flare(
                PARTICLE.STREAK,
                this.position,
                this.position.addNew(this.up.mulNew(this.attachmentRadius*2)),
                0, 0xFF7A33, 1.0);

            Effect.flare(
                PARTICLE.STREAK,
                this.position,
                this.position.addNew(this.forward.mulNew(this.attachmentRadius*2)),
                0, 0x55FF33, 1.0);
    */
            // Or use physicsframesize,        this._super(seconds);        ?

            // Test next relevant position to attach to

            var currMove = this.velocity.mulNew(seconds);

            var surfaceTimerTick = this.surfaceTimer.tick(seconds);
            if (!surfaceTimerTick && !this.justSpun && this.surface) {
                this.position.add(currMove);
            } else {
                // Simulate
                var distToCollision = rayCollisionDistance(this.position, currMove.mulNew(10));
                if (distToCollision < 0 || distToCollision > currMove.magnitude()) {
                    this.position.add(currMove);
                } else {
                    this.position.add(currMove.normalize().mul(distToCollision - this.attachmentRadius));
                }

                // First priority is to check where we are going, for a new surface there
                if (!this.velocity.isZero()) {
                    testAttachmentPositions.unshift( this.position.addNew(this.velocity.copy().normalize().mul(this.attachmentRadius + this.attachmentError + this.speed*seconds)) );
                }

                var lastSurface = this.surface;
                this.surface = null;
                forEach(
                    testAttachmentPositions,
                    function(position) {
                        if (this.surface) return; // Another iteration already found one
                        var dir = position.subNew(this.position);
                        if (dir.magnitude() === 0) return;
                        var dist = rayCollisionDistance(this.position, dir.mul(this.attachmentRadius+this.attachmentError));
                        if (dist > this.radius && dist < this.attachmentRadius+this.attachmentError) {
                            this.surface = World.getSurfaceNormal(
                                this.position,
                                this.position.addNew(dir.mulNew(dist))
                            );

                            // If surface is not productive, and not a floor - give up (but not if this is a new surface!)
                            if (target && this.surface.scalarProduct(this.targetDirection) > 0.75 && this.surface.z < 0.5 && (!lastSurface || this.surface.isCloseTo(lastSurface, 0.1))) {
                                this.surface = null; // Moving along this surface would not be productive
                                this.lastAttachmentPosition = null; // Spinning would also be futile
                            }
                        }
                    },
                    this
                );
            }

            // Re-attach to current surface at right distance, if on one
            if (this.surface) {
                // Current dist should be ~attachmentRadius
                var currDist = rayCollisionDistance(this.position, this.surface.mulNew(-this.attachmentRadius-this.attachmentError));
                var diff = currDist - this.attachmentRadius;
                if (diff <= this.attachmentError) {
                    this.position.add(this.surface.mulNew(-clamp(diff, -this.speed*seconds/2, this.speed*seconds/2)));
                } else {
                    // We are off the surface
                    this.surface = null;
                }
            }

            if (target && !this.surface && this.lastAttachmentPosition && this.justSpun < 0.5) {
                // Pivot on the attachment position
                this.surface = this.position.addNew(this.targetDirection.mulNew(this.speed*seconds)).sub(this.lastAttachmentPosition).normalize();
                if (this.surface.z >= 0.9) {
                    this.surface = null;
                } else {
                    this.position = this.lastAttachmentPosition.addNew(this.surface.mulNew(this.attachmentRadius));
                }

                this.justSpun += seconds;
            } else {
                this.justSpun = 0;
            }

            if (Global.SERVER) {
                // Sync to clients, possible
                if (this.syncTimer.tick(seconds)) {// && this.position.subNew(this.lastSyncedPosition).magnitude() > 0.1) {
                    this.sendUpdate();
                }
            }

            return true;
        },

        sendUpdate: function(extra) {
            var ret = [
                this.subEntityId,
                this.position.x, this.position.y, this.position.z,
                (this.target && this.target.uniqueId) ? this.target.uniqueId : -1
            ];
            if (extra !== undefined) {
                ret.push(extra);
            }
            this.manager.parent.swarmBugUpdate = ret;
        },

        //! Given a vector along the surface in the direction of the target, calculate the
        //! final velocity
        getSurfaceVelocity: function(seconds) {
            if (this.rangedAttacking) {
                return new Vector3(0, 0, 0);
            }

            // Push target, if it is too close and it is the player
            if (Global.CLIENT) {
                if (this.target === getPlayerEntity() && this.targetDirection && this.targetDistance < this.desiredTargetDistance*0.9) {
                    this.target.velocity.add(this.target.position.subNew(this.position).normalize().mulNew(seconds*2000));
                }
            }

            // Move self
            var towardTarget = this.targetDistance > this.desiredTargetDistance ? 1 : (
                this.targetDistance < this.desiredTargetDistance*0.9 ? -1 : 0
            );
            var alongSurface = this.targetDirection ? this.targetDirection.mulNew(towardTarget) : new Vector3(0, 0, 0);
            if (!alongSurface.isZero()) alongSurface.projectAlongSurface(this.surface).normalize();
            if (this.pain) alongSurface.mul(-this.pain * towardTarget);

            var that = this;

            // Swarm effect - avoid neighbors
            if (this.neighborTimer.tick(seconds)) {

                this.neighbors = filter(function(other) {
                    return other !== that && other.position.isCloseTo(that.position, that.personalSpace);
                }, this.manager.subEntities);
            }

            forEach(
                map(
                    function(other) {
                        var ret = other.position.subNew(that.position);
                        return ret.mul(-2*that.personalSpace/ret.magnitude());
                    },
                    this.neighbors
                ),
                function(direction) {
                    alongSurface.add(direction);
                }
            );

            // Final normalization
            if (!alongSurface.isZero()) {
                alongSurface.normalize();
                if (Global.CLIENT && Math.random() < seconds/3) {
                    Sound.play('swarm/scrape.ogg', this.position);
                }
            }

            return alongSurface.mul(this.speed);
        },

        render: function() {
    //        Effect.splash(PARTICLE.SPARK, 15, 0, this.position, 0xB49B4B, 1.0, 70, 1);
        },

        renderDynamic: function() {
            var o = this.position.addNew(this.up.mulNew(-this.attachmentRadius+1)); // +1 - so origin is not in world geometry. Corresponds to -1 in mdl.trans for the model
            var anim = this.attacking ? this.animations.meleeAttack|ANIM_LOOP : (
                this.pain > 0 ? this.animations.pain|ANIM_LOOP : (
                    this.rangedAttacking ? this.animations.rangedAttack : this.animations.forward|ANIM_LOOP
                )
            );
            var flags = MODEL.LIGHT | MODEL.CULL_VFC | MODEL.CULL_DIST | MODEL.DYNSHADOW;
            var orientation = this.forward.toYawPitchRoll(this.up, this.targetDirection);
            var args = [GameManager.getSingleton(), this.model, anim, o.x, o.y, o.z, orientation.yaw-90, orientation.pitch, orientation.roll, flags, this.basetime];
            GameManager.getSingleton().renderingHashHint = this.subEntityId;
            CAPI.renderModel2.apply(null, args);
        },

        think: function() {
            if (Global.SERVER) {
                var targets = getCloseEntities(this.position, this.searchRadius, Player);
                for (var i = 0; i < targets.length; i++) {
                    var target = targets[i][0];
                    var dist = targets[i][1];
                    if (Health.isValidTarget(target)) {
                        // In 1/3 the search radius, always find. Otherwise, need line of sight
                        if (dist < this.searchRadius/3 || hasLineOfSight(target.center, this.position)) {
                            this.target = target;
                            break;
                        }
                    }
                }
            }

            return Health.isValidTarget(this.target) ? this.target.getCenter() : null;
        },

        interpolate: function(seconds) {
            if (this.positionUpdateTimer < SwarmBugs.MAX_CLIENT_SYNC) {
                var factor = clamp((SwarmBugs.MAX_CLIENT_SYNC - this.positionUpdateTimer)/SwarmBugs.MAX_CLIENT_SYNC, 0, 1)*seconds*2;
                factor *= clamp(this.positionUpdate.subNew(this.position).magnitude()/this.radius, 1, 10); // More weight if far
                this.position = this.positionUpdate.lerp(this.position, factor);
            }

            this.positionUpdateTimer += seconds;
        },

        die: function(direction, bump) {
            direction = defaultValue(direction, new Vector3(0, 0, 0));
            bump = defaultValue(bump, 0);

            // Expire
            this.active = false;

            if (Global.CLIENT) {
                this.playPainSound();

                this.manager.ids[this.subEntityId] = new RepeatingTimer(10.0); // Time before we will accept this id again
                                                                              // prevents server 'resurrecting' them before they hear we
                                                                              // killed them
                GameManager.getSingleton().projectileManager.add(new SwarmBugs.SwarmBugRagdoll(
                    this.position.addNew(this.up.mulNew(-this.attachmentRadius*0.5)),
                    direction.mul(Math.min(bump*4, 100)),
                    { subEntity: this }
                ));
            } else { // SERVER
                this.manager.ids[this.subEntityId] = null;

                this.spawnPickups();
            }
        },

        spawnPickups: function() {
            if (!this.surface) return;
            if (Math.random() < 0.1) {
                // health
                GameManager.getSingleton().addPickup({
                    position: this.position,
                    type: 'h',
                });
            } else if (Math.random() < 0.05) {
                // rockets
                GameManager.getSingleton().addPickup({
                    position: this.position,
                    type: 'r',
                });
            }
        },

        sufferDamage: function(data) {
            // Suffer damage on both controlling client and other ones
            var damage = data.damage + data.nonControllerDamage;
            this.health -= damage;
            if (this.health <= 0) {
                var direction;
                if (Global.CLIENT) {
                    direction = this.position.subNew(data.origin).normalize();
                    if (this.surface) {
                        direction.mul(1).add(this.surface).normalize();
                    }
                }
                this.die(direction, damage);
            } else {
                if (damage >= 5 && this.pain < 0.2) { // send pain from harming client, and not too often
                    this.manager.parent.swarmBugClientPain = this.subEntityId;
                }
                if (damage >= 5) {
                    this.sufferPain();
                }
            }
        },

        sufferPain: function() {
            if (Global.CLIENT && this.pain < 0.2) {
                this.playPainSound();
            }
            this.pain = 0.5;
        },

        playPainSound: function() {
            Sound.play('swarm/Wolfsinger_inhumanscreech.ogg', this.position);
        },
    }),

    SwarmBugRagdoll: Projectiles.Projectile.extend({
        radius: 2,
        color: 0xDCBBAA,
        timeLeft: 2.5,
        gravity: 1.0,
        elasticity: 0.5,
        friction: 0.6,

        create: function(position, velocity, kwargs) {
            this._super.apply(this, arguments);

            this.subEntity = kwargs.subEntity;

            this.bounceFunc = partial(World.bounce, this, this.elasticity, this.friction);

            this.anim = ANIM_DYING;
            this.basetime = Global.lastmillis ? Global.lastmillis : 0;
        },

        render: function() {
            Effect.splash(PARTICLE.SMOKE, 2, 0.333, this.position, 0x000000, 2, 50, -20);
        },

        tick: function(seconds) {
            // Tend to turn upside down
            this.subEntity.up.add(new Vector3(
                Random.uniform(-seconds/10, seconds/10),
                Random.uniform(-seconds/10, seconds/10),
                -seconds
            )).normalize();

            // Fix forward vector
            this.subEntity.forward.projectAlongSurface(this.subEntity.up).normalize();

            return this._super.apply(this, arguments);
        },

        renderDynamic: function() {
            var o = this.position;
            var flags = MODEL.LIGHT | MODEL.CULL_VFC | MODEL.CULL_DIST | MODEL.DYNSHADOW;
            var orientation = this.subEntity.forward.toYawPitchRoll(this.subEntity.up, this.subEntity.targetDirection);
            var args = [GameManager.getSingleton(), this.subEntity.model, this.anim, o.x, o.y, o.z, orientation.yaw-90, orientation.pitch, orientation.roll, flags, this.basetime];
            GameManager.getSingleton().renderingHashHint = this.subEntity.subEntityId;
            CAPI.renderModel2.apply(null, args);
        },

        onExplode: function() {
            return false;
        },
    }),
};

SwarmBugSpawner = registerEntityClass(
    bakePlugins(WorldMarker, [
        Swarms.plugin,
        SwarmBugs.plugin,
        {
            _class: 'SwarmBugSpawner',
            shouldAct: true,

            minWaveBugs: new StateInteger(),
            maxWaveBugs: new StateInteger(),
            waveDelay: new StateFloat(),
            spawnWhenActive: new StateBoolean(),
            spawnRadius: new StateFloat(),
            surviveRadius: new StateFloat(),
            totalBugs: new StateInteger(),
            reactivateDelay: new StateFloat(),
            activateRadius: new StateFloat(),

            init: function() {
                this.minWaveBugs = 1;
                this.maxWaveBugs = 5;
                this.waveDelay = 5.0;
                this.spawnWhenActive = false;
                this.spawnRadius = 35;
                this.surviveRadius = 100; 
                this.totalBugs = 10;
                this.reactivateDelay = 5*60;
                this.activateRadius = 75;
            },

            activate: function() {
                this.spawnerActive = false; // We start out inactive until something triggers us
                this.thinkTimer = new RepeatingTimer(1.0); // Need to restart server for changes to spawnDelay to take effect!
                this.currReactivateDelay = 0;
            },

            act: function(seconds) {
                if (this.spawnerActive) {
                    if (this.thinkTimer.tick(seconds)) {
log(ERROR, Global.time + "spawner " + this.uniqueId + " : " + this.swarmManager.subEntities.length + '/' + this.spawnedBugs + '/' + this.totalBugs);
                        // Kill off ones that stray too far
                        forEach(this.swarmManager.subEntities, function(bug) {
                            if (!bug.position.isCloseTo(this.position, this.surviveRadius)) {
                                bug.die();
                            }
                        }, this);

                        // Deactivate if empty and spawned all we need
                        if (this.spawnedBugs === this.totalBugs && this.swarmManager.subEntities.length === 0) {
                            this.deactivateSpawner();
                        }

                        // Deactivate if all players are beyond the survive radius anyhow
                        if (this.swarmManager.subEntities.length > 0) {
                            var that = this;
                            if (filter(
                                function(player) {
                                    return player.position.isCloseTo(that.position, 1.1*that.surviveRadius);
                                },
                                getClientEntities()
                            ).length === 0) {
                                this.deactivateSpawner();
                            }
                        }
                    }

                    // Spawn a wave, if necessary
                    if (this.waveTimer.tick(seconds) && this.spawnedBugs < this.totalBugs && this.swarmManager.subEntities.length < this.maxWaveBugs) {
                        if (this.spawnWhenActive || this.swarmManager.subEntities.length === 0) {
                            this.spawnWave();
                        }
                    }
                } else {
                    // Inactive

                    // Tick the delay
                    this.currReactivateDelay = Math.max(this.currReactivateDelay - seconds, 0);

                    if (this.thinkTimer.tick(seconds)) {
                        // If players are close enough, trigger
                        if (this.currReactivateDelay === 0) {
                            var possibles = getCloseEntities(this.position, this.activateRadius, getEntityClass('Player'));
                            possibles = filter(function(pair) { return Health.isValidTarget(pair[0]); }, possibles);
                            if (possibles.length > 0) {
                                this.activateSpawner();
                            }
                        }
                    }
                }
            },

            activateSpawner: function() {
                if (this.spawnerActive) return; // Already active
                if (this.currReactivateDelay > 0) return; // Not ready yet

                this.spawnerActive = true;
                this.waveTimer = new RepeatingTimer(this.waveDelay);
                this.waveTimer.prime();
                this.spawnedBugs = 0;
            },

            deactivateSpawner: function() {
                this.spawnerActive = false;

                forEach(this.swarmManager.subEntities, function(subEntity) {
                    subEntity.die();
                });

                this.currReactivateDelay = this.reactivateDelay;
            },

            spawnWave: function() {
                var num = Math.min(
                    Random.randint(this.minWaveBugs, this.maxWaveBugs),
                    this.totalBugs - this.spawnedBugs,
                    this.maxWaveBugs - this.swarmManager.subEntities.length
                );

                for (var i = 0; i < num; i++) {
                    // New bug

                    // Pick direction
                    var spawnSurfaces = filter(
                        function(pair) {
                            var possible = pair[0];
                            return possible.parentEntity === this.uniqueId;
                        },
                        getCloseEntities(this.position, 16384, getEntityClass('SwarmBugSpawnerSurface')),
                        this
                    );
                    spawnSurfaces = map(
                        function(pair) { return pair[0]; },
                        spawnSurfaces
                    );

                    var direction, distance;
                    if (spawnSurfaces.length > 0) {
                        direction = Random.choice(spawnSurfaces).position.subNew(this.position);
                        distance = direction.magnitude();
                        direction.mul(1/distance);
                    } else {
                        direction = Random.normalizedVector3();
                        distance = this.spawnRadius;
                    }
                    var target = World.getRayCollisionWorld(this.position, direction, distance);

                    // Add bug

                    this.swarmManager.add(new SwarmBugs.SwarmBug({
                        position: target.addNew(direction.mulNew(-6.0)),
                        surface: direction.mulNew(-1),
                        target: null, //bind(getPlayerEntity().getCenter, getPlayerEntity())
                    }));
                }

                this.spawnedBugs += num;
            },

            clientAct: function(seconds) {
                if (isPlayerEditing() && this === CAPI.getTargetEntity()) {
                    EditingTools.showDebugSphere(this.position, this.activateRadius, 0xFF3355);
                    EditingTools.showDebugSphere(this.position, this.surviveRadius, 0x221177);
                }
            },
        },
    ])
);


//! A hint as to where to spawn swarmbugs
SwarmBugSpawnerSurface = registerEntityClass(
    bakePlugins(WorldMarker, [ChildEntityPlugin, {
        _class: 'SwarmBugSpawnerSurface',
        parentEntityClass: 'SwarmBugSpawner',
        debugDisplay: {
            color: 0x22BBFF,
            radius: 1.0,
        },
    }])
);

//! A hint as to where to spawn swarmbugs
SwarmBugSpawnerTrigger = registerEntityClass(
    bakePlugins(AreaTrigger, [ChildEntityPlugin, {
        _class: 'SwarmBugSpawnerTrigger',
        parentEntityClass: 'SwarmBugSpawner',
        debugDisplay: {
            color: 0xCCFF32,
            radius: 1.0,
        },

        onCollision: function(collider) {
            if (Health.isActiveEntity(collider)) {
                var parent = getEntity(this.parentEntity);
                if (parent) {
                    parent.activateSpawner(); // Parent will check if it can actually do so
                }
            }
        },
    }])
);

