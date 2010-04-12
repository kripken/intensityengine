
// Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
// This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

Firing = {
    plugins: {
        //! Network protocol for firing events (shots)
        protocol: {
            firingInfo: new StateArrayFloat({ clientSet: true, hasHistory: false }),

            clientActivate: function() {
                this.connect('client_onModify_firingInfo', this.onFiringInfo);
            },

            onFiringInfo: function(info) {
                if (info.length !== 5) return;
                var gunIndex = info[0];
                var targetPosition = new Vector3(info.slice(1,4));
                var targetEntity = getEntity(info[4]);
                var gun = Firing.guns[gunIndex];
                if (gun.handleClientEffect) {
                    gun.handleClientEffect(this, gun.getOrigin(this), targetPosition, targetEntity);
                }
            },
        },

        //! Manages a player firing a gun - delay, repeat, etc.
        player: {
            init: function() {
                forEach(values(Firing.guns), function(gun) {
                    if (gun.forPlayers && gun.originTag) {
                        this.attachments.push('*' + gun.originTag);
                    }
                }, this);
            },

            clientActivate: function() {
                this.gunDelay = 0;
                this.lastHandledShotCounter = 0;
                this.nowFiring = null;
            },

            clientAct: function(seconds) {
                if (this !== getPlayerEntity()) return;

                this.gunDelay = Math.max(this.gunDelay - seconds, 0);

                var gun = this.nowFiring;
                if (gun && this.gunDelay == 0) {
                    gun.doShot(this, CAPI.getTargetPosition(), CAPI.getTargetEntity());
                    this.gunDelay += gun.delay;
                    if (!gun.repeating) {
                        this.stopShooting(gun);
                    }
                }
            },

            startShooting: function(gun, position) {
                this.nowFiring = gun;
            },

            stopShooting: function(gun) {
                this.nowFiring = null;
            },
        },
    },

    guns: {}, // TODO: Make per-class

    //! Gun indexes 1-3 are usually used for player button indexes
    registerGun: function(index, gun, forPlayers) {
        forPlayers = defaultValue(forPlayers, false);

        this.guns[index] = gun;
        gun.gunIndex = index;
        gun.forPlayers = forPlayers;
    },

    clientClick: function(button, down, position, entity) {
        if (down) {
            if (getPlayerEntity().canMove) {
                getPlayerEntity().startShooting(Firing.guns[button], position);
            }
        } else {
            // TODO: Automatically stop shooting when canMove changes to false
            getPlayerEntity().stopShooting(Firing.guns[button], position);
        }

        return true; // Handled
    },
};

Gun = Class.extend({
    //! Called once when the shot is fired (not on all clients, just where the shot initiated)
    handleStartLogic: null,

    //! Called on all clients when they receive notice of the shot. Should show visual effects, etc.
    handleClientEffect: null,

    doShot: function(shooter, targetPosition, targetEntity) {
        // Send network protocol message
        shooter.firingInfo = [this.gunIndex, targetPosition.x, targetPosition.y, targetPosition.z, targetEntity ? targetEntity.uniqueId : -1];

        // Additional gun-specific initial logic
        if (this.handleStartLogic) {
            this.handleStartLogic(shooter, this.getOrigin(shooter), targetPosition, targetEntity);
        }
    },

    getOrigin: function(shooter) {
        return CAPI.getAttachmentPosition(shooter, this.originTag);
    },
});


ShootAction = LocalAnimationAction.extend({
    _name: 'ShootAction',
    secondsLeft: 1.0,
    localAnimation: ANIM_ATTACK1,
});


SniperRifle = Gun.extend({
    delay: 1.0,
    repeating: false,
    originTag: 'tag_chaingun', // Works with 'stromar' character model; override for others
    damage: 100,

    handleStartLogic: function(shooter, originPosition, targetPosition, targetEntity) {
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
        if (targetEntity) {
            Effect.splash(PARTICLE.BLOOD, 13, 1.0, targetPosition, 0x60FFFF, 1.0, 70, 1);
        } else {
            Effect.splash(PARTICLE.SPARK, 15, 0.2, targetPosition, 0xB49B4B, 1.0, 70, 1);
            Effect.addDecal(DECAL.BULLET, targetPosition, originPosition.subNew(targetPosition).normalize(), 3.0);
        }
        Effect.trail(PARTICLE.SMOKE, 0.5, originPosition, targetPosition, 0xC0C0C0, 0.6, 20);
        Sound.play('olpc/MichaelBierylo/sfx_DoorSlam.wav', originPosition);
        shooter.queueAction(new ShootAction());
// if(muzzlelight) adddynlight(hudgunorigin(gun, d->o, to, d), 25, vec(0.5f, 0.375f, 0.25f), 75, 75, DL_FLASH, 0, vec(0, 0, 0), d);
    },
});

Map.preloadSound('olpc/MichaelBierylo/sfx_DoorSlam.wav');

