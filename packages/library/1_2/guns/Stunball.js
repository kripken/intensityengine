
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


/*
StunAction = LocalAnimationAction.extend({
    _name: 'ShootAction',
    secondsLeft: 1.0,
    localAnimation: ANIM_ATTACK1,
});
*/

Stunball = Projectiles.Projectile.extend({
    radius: 4,
    color: 0xABCDFF,
    explosionPower: 50.0,
    speed: 90.0,
    timeLeft: 1.0,

    customDamageFunc: function(entity, damage) {
        if (damage > 25) {
            entity.sufferStun(damage);
        }
    },
});


StunballGun = Gun.extend({
    delay: 0.5,
    repeating: false,
    originTag: '',

    handleClientEffect: function(shooter, originPosition, targetPosition, targetEntity) {
        shooter.projectileManager.add( new Stunball(
            originPosition,
            targetPosition.subNew(originPosition).normalize(),
            shooter
        ));

        Sound.play("olpc/AdamKeshen/CAN_1.L.wav", originPosition);
    },
});

StunballVictimPlugin = {
    sufferingStun: new StateBoolean({ clientSet: true }),

    activate: function() {
        this.sufferingStun = false;
    },

    sufferStun: function(stun) {
        if (!this.sufferingStun) {
            this.oldMovementSpeed = this.movementSpeed;
            this.movementSpeed /= 4;
            this.sufferingStun = true;
        }
        this.sufferingStunLeft = stun/10; // seconds
    },

    clientAct: function(seconds) {
        if (this.sufferingStun) {
            var ox = (Math.random() - 0.5)*2*2;
            var oy = (Math.random() - 0.5)*2*2;
            var oz = (Math.random() - 0.5)*2;
            var speed = 150;
            var density = 2;
            Effect.flame(PARTICLE.SMOKE, this.getCenter().add(new Vector3(ox,oy,oz)), 0.5, 1.5, 0x000000, density, 2.0, speed, 0.6, -15);

            if (this === getPlayerEntity()) {
                this.sufferingStunLeft -= seconds;
                if (this.sufferingStunLeft <= 0) {
                    this.movementSpeed = this.oldMovementSpeed;
                    this.sufferingStun = false;
                }
            }
        }
    },
};

StunballBotPlugin = {
    init: function() {
        this.botFiringParams = {
            firingDelay: this.gun.delay,
            triggerFingerDelay: 0,
        };
    },
};

Map.preloadSound('olpc/AdamKeshen/CAN_1.L.wav'); // TODO: Nicer shot sound

