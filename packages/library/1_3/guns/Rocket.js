
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


Library.include('library/' + Global.LIBRARY_VERSION + '/Firing');
Library.include('library/' + Global.LIBRARY_VERSION + '/Projectiles');
Library.include('library/' + Global.LIBRARY_VERSION + '/Events');


Rocket = Projectiles.Projectile.extend({
    radius: 2,
    visualRadius: 4,
    color: 0xDCBBAA,
    explosionPower: 100.0,
    speed: 160.0,
    timeLeft: 5.0,
    gravityFactor: 0.0,

    create: function() {
        this._super.apply(this, arguments);

/*
        var finalDestination = World.getRayCollisionWorld(this.position, this.velocity.copy().normalize(), 2048);
        this.collideFunc = function(position, radius, ignore) {
            return position.isCloseTo(finalDestination, radius);
        };
*/
    },

    tick: function(seconds) {
        if (this.gravityFactor) {
            this.velocity.z -= this.gravityFactor*World.gravity*seconds;
        }
        return this._super(seconds);
    },

    renderDynamic: function() {
        var o = this.position;
        var flags = MODEL.LIGHT | MODEL.CULL_VFC | MODEL.CULL_OCCLUDED | MODEL.FULLBRIGHT | MODEL.CULL_DIST | MODEL.DYNSHADOW;
        var yawPitch = this.velocity.toYawPitch();
        var yaw = yawPitch.yaw - 90;
        var pitch = replaceNaN(90 - yawPitch.pitch); // NaN is possible if velocity is equal to 0
        var args = [this.owner, 'guns/rocket', ANIM_IDLE|ANIM_LOOP, o.x, o.y, o.z, yaw, pitch, flags, 0];
        CAPI.renderModel.apply(this.owner, args);
    },

    render: function() {
        Effect.splash(PARTICLE.SMOKE, 2, 0.3, this.position, 0xF0F0F0, 1.2, 50, -20);
        Effect.flame(PARTICLE.FLAME, this.position, 0.5, 0.5, 0xBB8877, 2, 3.0, 100, 0.4, -6);
        Effect.addDynamicLight(this.position, this.visualRadius*9, this.color);
    },
});


RocketFireAction = ParallelAction.extend({
    _name: 'RocketFireAction',
    canMultiplyQueue: false,
});


RocketGun = Projectiles.Gun.extend({
    projectileClass: Rocket,
    delay: 0.5,
    repeating: false,
    originTag: 'tag_shoulder_weap', // Works with 'stromar' character model; override for others

    handleClientEffect: function(shooter, originPosition, targetPosition, targetEntity) {
        var that = this;

        Sound.play('gk/jump', originPosition);

        var actionPerformer = shooter.shouldAct ? shooter : GameManager.getSingleton();
        var actionAdder = shooter.shouldAct ? shooter.queueAction : GameManager.getSingleton().queueParallelAction;

        actionAdder.call(actionPerformer, new RocketFireAction([
            new ShootAction1({ secondsLeft: 1.0 }),
            new DelayedAction(function() {
                if (!Health.isActiveEntity(shooter)) return;

                // Note that origin and target may have moved meanwhile
                var currentOriginPosition = that.getOrigin(shooter);
                var targetingOrigin = shooter.getTargetingOrigin(currentOriginPosition);
                var targetData = Firing.findTarget(shooter, currentOriginPosition, targetingOrigin, targetPosition, 2048);
                var currentTargetPosition = targetData.target;

                Effect.fireball(PARTICLE.EXPLOSION_NO_GLARE, currentOriginPosition, 3, 0.5, 0xFF775F, 3);
                Effect.addDynamicLight(currentOriginPosition, 20, 0xFF775F, 0.8, 0.1, 0, 10);

                Sound.play('yo_frankie/DeathFlash.wav', currentOriginPosition);

                // Ensure we shoot from without our xy radius, so if we are adjacent to a wall, the rocket won't immediately explode
                if (shooter.radius > 0) {
                    log(DEBUG, "Adjusting rocket origin");
                    var shooterPosition = shooter.position.copy();
                    shooterPosition.z = currentOriginPosition.z;
                    var dir = shooterPosition.subNew(currentOriginPosition);
                    var dist = dir.magnitude() + that.projectileClass.prototype.radius;
                    if (dist > shooter.radius) {
                        currentOriginPosition.add(dir.normalize().mul(dist - shooter.radius + 0.1));
                    }
                }

                that.shootProjectile(shooter, currentOriginPosition, currentTargetPosition, targetEntity, that.projectileClass);

                that.doRecoil(shooter, 40);
                },
                { secondsLeft: 0.2 } // Fire rocket a little into the shoot animation
            )
        ]));
    },

/* // Only clients do this
    handleServerLogic: function(shooter, originPosition, targetPosition, targetEntity) {
// delay as well TODO
        this.shootProjectile(shooter, originPosition, targetPosition, targetEntity, this.projectileClass);
    },
*/
});


// Seeking rocket

SeekingRocket = Rocket.extend({
    color: 0xAADC8A,
    explosionPower: 150.0,
    speed: 90.0,
    timeLeft: 8.0,
    gravityFactor: 0,
    accuracy: 1.0,
    seekFrameTime: 1/5,

    create: function() {
        this._super.apply(this, arguments);

        if (Global.CLIENT && this.targetEntity === getPlayerEntity()) {
            GameManager.getSingleton().addHUDMessage('Incoming missile', 0xCCFF49, 2.0, 0.5);
        }

        this.timer = new RepeatingTimer(this.seekFrameTime);
    },

    tick: function(seconds) {
        if (this.timer.tick(seconds) && Health.isValidTarget(this.targetEntity)) {
            var direction = this.targetEntity.getCenter().subNew(this.position).normalize();
//            var cosineFactor = direction.cosineAngleWith(this.velocity);
            var currSpeed = this.speed*this.seekFrameTime*1.5*this.accuracy;// / ((cosineFactor+1)/2);
            this.velocity.add( direction.mul(currSpeed) );
            this.velocity.normalize().mul(this.speed);
//            this.velocity.mul(0.99); // Friction slowness

/*
            if (Global.CLIENT) {
                if (!this.lightningTimer) {
                    this.lightningTimer = new RepeatingTimer(0.1);
                    this.lightningTimer.prime();
                }
                if (this.lightningTimer.tick(seconds)) {
                    Effect.lightning(this.position, this.targetEntity.getCenter(), 0.1, 0xC0F0A0, 2.0);
                }
            }
*/
        }

        return this._super(seconds);
    },

    render: function() {
        Effect.splash(PARTICLE.SMOKE, 2, 0.3, this.position, 0xC0F0A0, 1.2, 50, -20);
        Effect.flame(PARTICLE.FLAME, this.position, 0.5, 0.5, 0xBBCC77, 2, 3.0, 100, 0.4, -6);
        Effect.addDynamicLight(this.position, this.visualRadius*9, this.color);
    },
});


SeekingRocketGun = RocketGun.extend({
    projectileClass: SeekingRocket,
    delay: 1.5,
    supportsLockedTargets: true,

    handleStartLogic: function(shooter, originPosition, targetPosition, targetEntity) {
        if (!targetEntity) {
            if (shooter.lockedTargetEntity) {
                // Shoot in facing direction
                targetPosition = shooter.position.addNew(new Vector3().fromYawPitch(shooter.yaw, shooter.pitch).mul(100));

                this.doShot(shooter, targetPosition, shooter.lockedTargetEntity);
                shooter.lockedTargetEntity = null;
            } else {
                if (shooter === getPlayerEntity()) {
                    GameManager.getSingleton().addHUDMessage("(No target)", 0xAA8833, 1.5, 0.5);
                    Sound.play('gk/imp_05.ogg');
                }
            }
            return false; // In any case cancel this shot
        }

        this.doRecoil(shooter, 50);
    },
});


//! Lets you shoot at someone you 'locked' a short while ago
TargetLockingPlugin = {
    clientAct: function(seconds) {
        if (this !== getPlayerEntity()) return;

        var gun = Firing.guns[this.currGunIndex];
        if (!gun || !gun.supportsLockedTargets) return;

        var currTargetEntity = CAPI.getTargetEntity();
        if (currTargetEntity) {
            GameManager.getSingleton().addHUDMessage("Target locked", 0xAA8833, 1.5, 0.5);
            this.lockedTargetEntity = currTargetEntity;
            this.lockedTargetEntityStaleness = 0;
        } else if (this.lockedTargetEntity) {
            this.lockedTargetEntityStaleness += seconds;
            if (this.lockedTargetEntityStaleness > 1.5) {
                this.lockedTargetEntity = null;
            }
        }

        // TODO: Show on crosshair
    },
};

/*
BotTargetLockingPlugin = {
    act: function(seconds) {
        this.lockedTargetEntity = this.currAutoTarget;
    },

    clientAct: function(seconds) {
        this.lockedTargetEntity = this.currAutoTarget;
    },
};
*/

// Preloads

Map.preloadModel('guns/rocket');
Map.preloadSound('yo_frankie/DeathFlash.wav');

