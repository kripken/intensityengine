
// Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
// This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

Library.include('library/1_1/Firing');


InstaGun = Gun.extend({
    delay: 1.0,
    repeating: false,
    originTag: 'tag_chaingun', // Works with 'stromar' character model; override for others
    damage: 100,

    handleStartLogic: function(shooter, originPosition, targetPosition, targetEntity) {
        this.doRecoil(shooter, 60);

        // damage
        if (targetEntity && targetEntity.sufferDamage) {
            this.damage = 100;

            // Quad damage for headshots
            var aboveEye = targetEntity.aboveEye;
            if (aboveEye) {
                var neck = CAPI.getAttachmentPosition(targetEntity, 'tag_neck');
                if (targetPosition.z >= neck.z-aboveEye*2 && targetPosition.isCloseTo(neck, aboveEye*2)) {
                    this.damage = 400;
                    GameManager.getSingleton().addHUDMessage("Headshot!", 0xFF3015, 2.0);
                }
            }

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
        shooter.queueAction(new ShootAction2({ secondsLeft: 1.0 }));
    },
});

/* Not needed
InstaGunPlugin = {
    activate: function() {
        this.attachments.push('*tag_neck');
    },
};
*/

Map.preloadSound('olpc/MichaelBierylo/sfx_DoorSlam.wav');

