
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
                if (!Health.isActiveEntity(this)) return; // Do not shoot if just killed (even though canMove=false didn't arrive yet)

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
            gunIndexes: new StateArrayInteger(),

            init: function() {
                forEach(values(Firing.guns), function(gun) {
                    var tag = '*' + gun.originTag;
                    if (gun.originTag && findIdentical(this.attachments.asArray(), tag) < 0) {
                        this.attachments.push(tag);
                    }
                }, this);

                this.gunIndexes = [];
            },

            activate: function() {
                this.connect('onModify_gunIndexes', function(indexes) {
                    if (indexes.length > 0) {
                        this.currGunIndex = indexes[0]; // Sets initial value
                    }
                });
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

                if (gun) {
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
                }
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

    registerGun: function(gun, comment, hud) {
        var index = this.guns.length;
        comment = defaultValue(comment, index.toString());
        this.guns[index] = gun;
        gun.gunIndex = index;
        gun.comment = comment;
        gun.hud = hud;

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
            Firing.cycleGunIndex(player, player.gunIndexes.asArray());
        }

        return true; // Handled
    },


    cycleGunIndex: function(entity, indexes) {
        var curr = findIdentical(indexes, entity.currGunIndex) + 1;
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
        if (!Health.isActiveEntity(shooter)) return; // Do not shoot if just killed (even though canMove=false didn't arrive yet)

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

