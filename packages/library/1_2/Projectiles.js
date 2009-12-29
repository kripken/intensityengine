
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


Library.include('library/1_2/Firing.js');

Projectiles = {
    //! Each client does itself. Server does everything else.
    //! Entities that receive damage should have a sufferDamage() function.
    //! @param velocity The velocity of the projectile. This is useful because, if the
    //!                 projectile explodes right on the target - a direct hit - then
    //!                 out calculation of the direction to push is very sensitive, and
    //!                 may lead to being thrown in the opposite direction of the
    //!                 projectile, which would look odd. So, we bias in the direction of
    //!                 the velocity.
    doBlastWave: function(position, power, velocity, customDamageFunc) {
        // Simple Power-Distance^exp physics
        var expo = 1.333;
        var maxDist = Math.pow(power-1, 1/expo);
        var entities;
        if (Global.CLIENT) {
            entities = [getPlayerEntity()];
        } else {
            entities = getCloseEntities(position, maxDist);
            entities = map(function(pair) { return pair[0]; }, entities);
            entities = filter(function(entity) { return !(entity instanceof Player); }, entities);
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
                entity.sufferDamage({ damage: bump });
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
        },

        tick: function(seconds) {
            this.timeLeft -= seconds;
            if (this.timeLeft < 0) {
                return false;
            }

            if (this.gravity) {
                this.velocity.z -= World.gravity*seconds;
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

                if ( World.isColliding(this.position, this.radius, this.owner) )
                {
                    // Do some fine-tuning of the explosion position - small steps. Not doing this all the time
                    // saves CPU, and would only lead to bugs if there are really really thin obstacles.
                    var NUM_STEPS = 5;
                    var step = this.velocity.mulNew(this.physicsFrameSize/NUM_STEPS);
                    for (var i = 0; i < NUM_STEPS; i++) {
                        lastPosition.add(step);
                        // Don't do last step - we already know the answer
                        if ( (i == NUM_STEPS-1) || World.isColliding(lastPosition, this.radius, this.owner) ) {
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
                Effect.fireball(PARTICLE.EXPLOSION, this.position, this.radius*5, 1.0, this.color, this.radius);
                Sound.play("yo_frankie/DeathFlash.wav", this.position);
                Effect.addDecal(DECAL.SCORCH, this.position, this.velocity.copy().normalize().mul(-1), this.radius, 0xFF8877);
                Effect.addDynamicLight(this.position, this.radius*14, this.color, 0.8, 0.1, 0, this.radius*9);
            }

            Projectiles.doBlastWave(this.position, this.explosionPower, this.velocity, this.customDamageFunc);

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
            this.projectileManager.tick(seconds);
        },
        clientAct: function(seconds) {
            this.projectileManager.tick(seconds);
            this.projectileManager.render();
        },
        renderDynamic: function(HUDPass, needHUD) {
            if (!HUDPass) {
                this.projectileManager.renderDynamic();
            }
        },
    },

    Gun: Gun.extend({
        shootProjectile: function(shooter, originPosition, targetPosition, targetEntity, projectileClass) {
            shooter.projectileManager.add( new projectileClass(
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


