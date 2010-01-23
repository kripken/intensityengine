
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


Library.include('library/' + Global.LIBRARY_VERSION + '/Firing.js');

Projectiles = {
    serverside: true,

    //! Each client does itself. Server does everything else.
    //! Entities that receive damage should have a sufferDamage() function.
    //! @param velocity The velocity of the projectile. This is useful because, if the
    //!                 projectile explodes right on the target - a direct hit - then
    //!                 out calculation of the direction to push is very sensitive, and
    //!                 may lead to being thrown in the opposite direction of the
    //!                 projectile, which would look odd. So, we bias in the direction of
    //!                 the velocity.
    doBlastWave: function(position, power, velocity, customDamageFunc, owner) {
        // Simple Power-Distance^exp physics
        var expo = 1.333;
        var maxDist = Math.pow(power-1, 1/expo);

        var entities;
        if (Projectiles.serverside) {
            if (Global.CLIENT) {
                entities = [getPlayerEntity()];
            } else {
                entities = getCloseEntities(position, maxDist);
                entities = map(function(pair) { return pair[0]; }, entities);
                entities = filter(function(entity) { return !(entity instanceof Player); }, entities);
            }
        } else {
            entities = [];
            if (owner === getPlayerEntity()) { // The owner does most everything
                entities = getCloseEntities(position, maxDist);
                entities = map(function(pair) { return pair[0]; }, entities);
                entities = filter(function(entity) { return !(entity instanceof Player); }, entities);
            }
            entities.push(getPlayerEntity()); // Every player does themselves
        }

        forEach(entities, function(entity) {
            if (!entity.sufferDamage) return;

            var distance = entity.getCenter().sub(position).magnitude();
            distance = Math.max(1, distance);
            var bump = Math.max(0, power-Math.pow(distance, expo));
            if (!customDamageFunc) {
                if (entity.velocity) {
                    entity.velocity.add(entity.position.subNew(position).add(velocity.copy().normalize().mul(4)).add(new Vector3(0,0,2)).normalize().mul(bump*4));
                }
                if (Projectiles.serverside) {
                    entity.sufferDamage({ damage: bump });
                } else {
//                    if (owner === getPlayerEntity()) {
                        entity.sufferDamage({ damage: bump });
//                    } else {
//                        entity.sufferDamage({ damage: 0, nonControllerDamage: bump });
//                    }
                }
            } else {
                customDamageFunc(entity, bump);
            }
        });
    },

    Projectile: Class.extend({
        physicsFrameSize: 0.02, // 50fps
        speed: 1.0,
        timeLeft: 5.0,
        gravity: 0,
        radius: 1.0,
        explosionPower: 0,

        create: function(position, velocity, owner, targetEntity) {
            //! The start position of the projectile
            this.position = position.copy();
            //! The velocity, in units/second
            this.velocity = velocity.copy().mul(this.speed);

            this.owner = owner;
            this.ignore = owner;

            this.targetEntity = targetEntity;

            this.physicsFrameTimer = new RepeatingTimer(this.physicsFrameSize, true);

            if (owner) {
                this.yaw = owner.yaw;
                this.pitch = owner.pitch;
            }

            this.collideFunc = World.isColliding;
        },

        tick: function(seconds) {
            this.timeLeft -= seconds;
            if (this.timeLeft < 0) {
                return false;
            }

            if (this.gravity) {
                this.velocity.z -= World.gravity*this.gravity*seconds;
            }

            var firstTick = this.physicsFrameTimer.tick(seconds); // Tick once with entire time, then
                                                                  // tick with 0's to pick off all frames
            while (firstTick || this.physicsFrameTimer.tick(0)) {
                firstTick = false;

                var lastPosition = this.position.copy();
                if (this.bounceFunc) {
                    // Let custom bounce function handle actual movement and bouncing etc.
                    this.bounceFunc(this.physicsFrameSize);
                } else {
                    this.position.add( this.velocity.mulNew(this.physicsFrameSize) );
                }

                if ( this.collideFunc(this.position, this.radius, this.owner) )
                {
                    // Do some fine-tuning of the explosion position - small steps. Not doing this all the time
                    // saves CPU, and would only lead to bugs if there are really really thin obstacles.
                    var NUM_STEPS = 5;
                    var step = this.velocity.mulNew(this.physicsFrameSize/NUM_STEPS);
                    for (var i = 0; i < NUM_STEPS; i++) {
                        lastPosition.add(step);
                        // Don't do last step - we already know the answer
                        if ( (i == NUM_STEPS-1) || this.collideFunc(lastPosition, this.radius, this.owner) ) {
                            break;
                        }
                    }
                    this.position = lastPosition;
                    return this.onExplode();
                }
            }
            return true;
        },

        render: function() {
            Effect.fireball(PARTICLE.EXPLOSION_NO_GLARE, this.position, this.radius, 0.05, this.color, this.radius);
            Effect.addDynamicLight(this.position, this.radius*9, this.color);
        },

        onExplode: function() {
            if (Global.CLIENT) {
                var radius = this.visualRadius !== undefined ? this.visualRadius : this.radius;
                Effect.fireball(PARTICLE.EXPLOSION, this.position, radius*5, 0.333, this.color, radius);
                Sound.play("yo_frankie/DeathFlash.wav", this.position);
                Effect.addDecal(DECAL.SCORCH, this.position, this.velocity.copy().normalize().mul(-1), radius, 0xFF8877);
                Effect.addDynamicLight(this.position, radius*14, this.color, 0.2666, 0.0333, 0, radius*9);
            }

            Projectiles.doBlastWave(this.position, this.explosionPower, this.velocity, this.customDamageFunc, this.owner);

            return false; // The projectile ends its life
        }
    }),

    Manager: Class.extend({
        create: function() {
            this.projectiles = [];
        },

        add: function(projectile) {
            this.projectiles.push(projectile);
        },

        tick: function(seconds) {
            this.projectiles = filter(function(projectile) { return projectile.tick(seconds); }, this.projectiles);
        },

        render: function() {
            forEach(this.projectiles, function(projectile) { if (projectile.render) projectile.render(); });
        },

        renderDynamic: function() {
            forEach(this.projectiles, function(projectile) { if (projectile.renderDynamic) projectile.renderDynamic(); });
        },
    }),

    plugin: {
        activate: function() {
            this.projectileManager = new Projectiles.Manager();
        },
        clientActivate: function() {
            this.projectileManager = new Projectiles.Manager();
        },
        act: function(seconds) {
            if (this.projectileManager.projectiles.length === 0) return;

            var time;
            if (Global.profiling && Global.profiling.data) {
                time = CAPI.currTime();
            }

            this.projectileManager.tick(seconds);

            if (Global.profiling && Global.profiling.data) {
                var _class = 'projectileManager::' + this.uniqueId;
                time = CAPI.currTime() - time;
                if (Global.profiling.data[_class] === undefined) Global.profiling.data[_class] = 0;
                Global.profiling.data[_class] += time;
            }
        },
        clientAct: function(seconds) {
            if (this.projectileManager.projectiles.length === 0) return;

            this.projectileManager.tick(seconds);
            this.projectileManager.render();
        },
        renderDynamic: function(HUDPass, needHUD) {
            if (this.projectileManager.projectiles.length === 0) return;

            if (!HUDPass) {
                this.projectileManager.renderDynamic();
            }
        },
    },

    Gun: Gun.extend({
        shootProjectile: function(shooter, originPosition, targetPosition, targetEntity, projectileClass) {
            var projectileHandler = shooter.shouldAct && shooter.projectileManager ? shooter : GameManager.getSingleton();

            projectileHandler.projectileManager.add( new projectileClass(
                originPosition,
                targetPosition.subNew(originPosition).normalize(),
                shooter,
                targetEntity
            ));
        },
    }),
};

Map.preloadSound('yo_frankie/DeathFlash.wav');


// Examples

SmallShot = Projectiles.Projectile.extend({
    radius: 5,
    color: 0xFFCC66,
    explosionPower: 50.0,
    speed: 50.0
});

Projectiles.debris = Projectiles.Projectile.extend({
    radius: 2,
    color: 0xDCBBAA,
    timeLeft: 5,
    gravity: 1.0,
    elasticity: 0.5,
    friction: 0.6,

    create: function(position, velocity, kwargs) {
        this._super.apply(this, arguments);

        this.bounceFunc = partial(World.bounce, this, this.elasticity, this.friction);
    },

    render: function() {
        Effect.splash(PARTICLE.SMOKE, 1, 0.25, this.position, 0x000000, 1, 2, -20);
    },

    renderDynamic: function() {
        var o = this.position;
        var flags = MODEL.LIGHT | MODEL.CULL_VFC | MODEL.CULL_DIST | MODEL.DYNSHADOW;
        var args = [GameManager.getSingleton(), 'debris', ANIM_IDLE, o.x, o.y, o.z, 0, 0, 0, flags, 0];
        CAPI.renderModel2.apply(null, args);
    },

    onExplode: function() {
        return false;
    },
});

