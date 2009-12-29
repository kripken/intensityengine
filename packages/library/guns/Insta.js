
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


Library.include('library/Firing');


InstaGun = Gun.extend({
    delay: 1.0,
    repeating: false,
    originTag: 'tag_chaingun', // Works with 'stromar' character model; override for others
    damage: 100,

    handleStartLogic: function(shooter, originPosition, targetPosition, targetEntity) {
        // recoil
        if (shooter.canMove) {
            var dir = (new Vector3()).fromYawPitch(shooter.yaw, shooter.pitch).normalize(1).mul(-60.0);
            shooter.velocity.add(dir);
        }

        // damage
        if (targetEntity && targetEntity.sufferDamage) {
            targetEntity.sufferDamage(this);
        }
    },

    handleClientEffect: function(shooter, originPosition, targetPosition, targetEntity) {
        Effect.addDynamicLight(originPosition, 20, 0xFFFFFF, 0.8, 0.1, 0, 10);
        Effect.splash(PARTICLE.SPARK, 15, 0.1, originPosition, 0xB49B4B, 1.0, 70, 1);

        Effect.flare(
            PARTICLE.STREAK,
            originPosition,
            targetPosition,
            0.1,
            0xFFFFFF
        );
        if (targetEntity && Health.isValidTarget(targetEntity)) {
            Effect.splash(PARTICLE.BLOOD, 13, 1.0, targetPosition, 0x60FFFF, 1.0, 70, 1);
        } else {
            Effect.splash(PARTICLE.SPARK, 15, 0.2, targetPosition, 0xB49B4B, 1.0, 70, 1);
            Effect.addDecal(DECAL.BULLET, targetPosition, originPosition.subNew(targetPosition).normalize(), 3.0);
        }
        Effect.trail(PARTICLE.SMOKE, 0.5, originPosition, targetPosition, 0xC0C0C0, 0.6, 20);
        Sound.play('olpc/MichaelBierylo/sfx_DoorSlam.wav', originPosition);
        shooter.queueAction(new ShootAction());
    },
});

Map.preloadSound('olpc/MichaelBierylo/sfx_DoorSlam.wav');

