
// Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
// This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

Firing = {
    plugins: {
        //! Network protocol for firing events (shots)
        protocol: {
            firingInfo: new StateArrayFloat({ clientSet: true, hasHistory: false }),

            activate: function() {
                this.connect('onModify_firingInfo', this.onFiringInfo);
            },

            clientActivate: function() {
                this.connect('client_onModify_firingInfo', this.onFiringInfo);
            },

            onFiringInfo: function(info) {
                if (info.length !== 5) return;
                var gunIndex = info[0];
                var targetPosition = new Vector3(info.slice(1,4));
                var targetEntity = getEntity(info[4]);
                var gun = Firing.guns[gunIndex];
                if (Global.CLIENT) {
                    if (gun.handleClientEffect) {
                        gun.handleClientEffect(this, gun.getOrigin(this), targetPosition, targetEntity);
                    }
                } else { // SERVER
                    if (gun.handleServerLogic) {
                        gun.handleServerLogic(this, gun.getOrigin(this), targetPosition, targetEntity);
                    }
                }
            },
        },

        //! Manages a player firing a gun - delay, repeat, etc.
        player: {
            currGunIndex: new StateInteger({ clientSet: true }),

            init: function() {
                forEach(values(Firing.guns), function(gun) {
                    var tag = '*' + gun.originTag;
                    if (gun.forPlayers && gun.originTag && findIdentical(this.attachments.asArray(), tag) < 0) {
                        this.attachments.push(tag);
                    }
                }, this);

                this.currGunIndex = Firing.getInitialPlayerGunIndex();
            },

            clientActivate: function() {
                this.gunDelay = 0;
                this.lastHandledShotCounter = 0;
                this.nowFiring = false;

                this.connect('client_onModify_currGunIndex', function() {
                    Sound.play('gk/imp_01', this.position.copy());
                });
            },

            clientAct: function(seconds) {
                if (this !== getPlayerEntity()) return;

                this.gunDelay = Math.max(this.gunDelay - seconds, 0);

                var gun = Firing.guns[this.currGunIndex];

                if (this.nowFiring && this.gunDelay == 0) {
                    gun.doShot(this, CAPI.getTargetPosition(), CAPI.getTargetEntity());
                    this.gunDelay += gun.delay;
                    if (!gun.repeating) {
                        this.stopShooting(gun);
                    }
                }

//                if (gun && gun.comment) {
//                    CAPI.showHUDText(gun.comment, 0.1, 0.9, 0.4, 0x7799DD);
//                }

                var func = CAPI.setHUD ? CAPI.setHUD : CAPI.showHUDImage;
                func(gun.hud, 0.80, 0.88, 0.05, 0.05);
            },

            startShooting: function(position) {
                this.nowFiring = true;
            },

            stopShooting: function(gun) {
                this.nowFiring = false;
            },
        },
    },

    guns: [],

    playerGuns: [],

    registerGun: function(gun, forPlayers, comment, hud) {
        forPlayers = defaultValue(forPlayers, false);

        var index = this.guns.length;
        comment = defaultValue(comment, index.toString());
        this.guns[index] = gun;
        gun.gunIndex = index;
        gun.forPlayers = forPlayers;
        gun.comment = comment;
        gun.hud = hud;

        if (forPlayers) {
            this.playerGuns.push(index);

            // XXX remove
            if (this.showHUD && !this.__shownInitialWeapon) {
                func(hud, 0.80, 0.88, 0.05, 0.05);
                this.__shownInitialWeapon = true;
            }
        }
    },

    getInitialPlayerGunIndex: function() {
        var index = this.playerGuns[0];
        return index;
    },

    clientClick: function(button, down, position, entity) {
        var player = getPlayerEntity();
        if (button === 1) {
            if (down) {
                if (player.canMove) {
                    player.startShooting(position);
                }
            } else {
                // TODO: Automatically stop shooting when canMove changes to false (connect to signal)
                player.stopShooting(position);
            }
        } else if (button === 3 && down) {
            Firing.cycleGunIndex(player, Firing.playerGuns);
        }

        return true; // Handled
    },

    cycleGunIndex: function(entity, indexes) {
        var curr = entity.currGunIndex + 1;
        if (curr >= indexes.length) {
            curr = 0;
        }
        entity.currGunIndex = indexes[curr];
    },
};

Gun = Class.extend({
    //! Called once when the shot is fired (not on all clients, just where the shot initiated).
    //! If returns false, cancels the shot
    handleStartLogic: null,

    //! Called on all clients when they receive notice of the shot. Should show visual effects, etc.
    handleClientEffect: null,

    doShot: function(shooter, targetPosition, targetEntity) {
        // Additional gun-specific initial logic
        if (this.handleStartLogic) {
            if (this.handleStartLogic(shooter, this.getOrigin(shooter), targetPosition, targetEntity) === false) return;
        }

        // Send network protocol message
        shooter.firingInfo = [this.gunIndex, targetPosition.x, targetPosition.y, targetPosition.z, targetEntity ? targetEntity.uniqueId : -1];
    },

    getOrigin: function(shooter) {
        return CAPI.getAttachmentPosition(shooter, this.originTag);
    },

    doRecoil: function(shooter, magnitude) {
        if (shooter.canMove) {
            var dir = (new Vector3()).fromYawPitch(shooter.yaw, shooter.pitch).normalize(1).mul(-magnitude);
            shooter.velocity.add(dir);
        }
    },
});


ShootAction1 = LocalAnimationAction.extend({
    _name: 'ShootAction',
    localAnimation: ANIM_ATTACK1,
    canBeCancelled: false,
});

ShootAction = ShootAction1; // Convenience

ShootAction2 = LocalAnimationAction.extend({
    _name: 'ShootAction',
    localAnimation: ANIM_ATTACK2,
    canBeCancelled: false,
});

ShootAction3 = LocalAnimationAction.extend({
    _name: 'ShootAction',
    localAnimation: ANIM_ATTACK3,
    canBeCancelled: false,
});

