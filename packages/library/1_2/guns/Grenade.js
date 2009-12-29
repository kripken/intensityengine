
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


Library.include('library/1_2/Firing');
Library.include('library/1_2/Projectiles');
Library.include('library/1_2/Events');
Library.include('library/1_2/World');


Grenade = Projectiles.Projectile.extend({
    radius: 4,
    color: 0xDCBBAA,
    explosionPower: 100.0,
    speed: 120.0,
    timeLeft: 5.0,

    elasticity: 0.66,

    create: function() {
        this._super.apply(this, arguments);

        this.ignore = this.owner;
        this.bounceFunc = partial(World.bounce, this, this.elasticity);
    },

    tick: function(seconds) {
        this.velocity.z -= World.gravity*seconds;
        return this._super.apply(this, arguments);
    },

    render: function() {
//        Effect.splash(PARTICLE.SMOKE, 2, 0.3, this.position, 0xF0F0F0, 1.2, 50, -20);
        Effect.flame(PARTICLE.FLAME, this.position, 0.5, 0.5, 0xBB8877, 2, 3.0, 100, 0.4, -6);
        Effect.addDynamicLight(this.position, this.radius*9, this.color);
    },
});


GrenadeGun = Projectiles.Gun.extend({
    projectileClass: Grenade,
    delay: 0.5,
    repeating: false,
    originTag: 'tag_shoulder_weap', // Works with 'stromar' character model; override for others

    handleClientEffect: function(shooter, originPosition, targetPosition, targetEntity) {
        var that = this;

        Sound.play('gk/jump', originPosition);

        shooter.queueAction(new ShootAction1({ secondsLeft: 0.5 }));

//                Effect.fireball(PARTICLE.EXPLOSION_NO_GLARE, currentOriginPosition, 3, 0.5, 0xFF775F, 3);
//                Effect.addDynamicLight(currentOriginPosition, 20, 0xFF775F, 0.8, 0.1, 0, 10);

        this.shootProjectile(shooter, originPosition, targetPosition, targetEntity, this.projectileClass);

//                Sound.play('yo_frankie/DeathFlash.wav', currentOriginPosition);

        this.doRecoil(shooter, 10);
    },

//    handleServerLogic: function(shooter, originPosition, targetPosition, targetEntity) {
//// delay as well TODO
//        this.shootProjectile(shooter, originPosition, targetPosition, targetEntity, this.projectileClass);
//    },
});


// Preloads

Map.preloadModel('guns/rocket');
//Map.preloadSound('yo_frankie/DeathFlash.wav');

