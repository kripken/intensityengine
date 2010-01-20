
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
                // Note that cannons etc. need health > 0 to fire, just like players! XXX
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
            gunAmmos: {}, // gunIndex->ammo

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
                        if (this.gunAmmos[this.currGunIndex] !== null && this.gunAmmos[this.currGunIndex] <= 0) {
                            this.queueAction(new OutOfAmmoAction());
                            this.stopShooting(gun);
                        } else {
                            gun.doShot(this, CAPI.getTargetPosition(), CAPI.getTargetEntity());
                            if (this.gunAmmos[this.currGunIndex]) {
                                this.gunAmmos[this.currGunIndex] -= 1;
                            }
                            this.gunDelay += gun.delay;
                            if (!gun.repeating) {
                                this.stopShooting(gun);
                            }
                        }
                    }

    //                if (gun && gun.comment) {
    //                    CAPI.showHUDText(gun.comment, 0.1, 0.9, 0.4, 0x7799DD);
    //                }

                    if (!Global.gameHUD) {
                        var func = CAPI.setHUD ? CAPI.setHUD : CAPI.showHUDImage;
                        func(gun.hud, 0.80, 0.88, 0.05, 0.05);
                    } else {
                        var params = Global.gameHUD.getFiringParams(gun);
                        CAPI.showHUDImage(params.gun.icon, params.gun.x, params.gun.y, params.gun.w, params.gun.h);
                        if (params.ammo1) {
                            CAPI.showHUDImage(params.ammo1.icon, params.ammo1.x, params.ammo1.y, params.ammo1.w, params.ammo1.h);
                        }
                        if (params.ammo2) {
                            CAPI.showHUDImage(params.ammo2.icon, params.ammo2.x, params.ammo2.y, params.ammo2.w, params.ammo2.h);
                        }
                    }
                }
            },

            startShooting: function(position) {
                this.nowFiring = true;
            },

            stopShooting: function(position) {
                var gun = Firing.guns[this.currGunIndex];

                if (gun.repeating) {
                    this.gunDelay = 0;
                }

                if (gun.stopShooting) {
                    gun.stopShooting(this);
                }

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

    findTarget: function(shooter, visualOrigin, targetingOrigin, fallbackTarget, range, scatter) {
        // Targeting from the camera - where the player aimed the mouse
        var direction = new Vector3().fromYawPitch(shooter.yaw, shooter.pitch);
        if (isNaN(direction.x)) return { target: fallbackTarget };
        if (scatter) direction.add(Random.normalizedVector3().mul(scatter)).normalize();
        var target = World.getRayCollisionWorld(targetingOrigin, direction, range);
        var temp = World.getRayCollisionEntities(targetingOrigin, target, shooter);
        var targetEntity;
        if (temp) {
            target = temp.collisionPosition;
            targetEntity = temp.entity;
        }

        // Check for hitting an entity from the gun source
        var temp = World.getRayCollisionEntities(visualOrigin, target, shooter);
        if (temp) {
            target = temp.collisionPosition;
            targetEntity = temp.entity;
        }

        // Check for hitting the scenery from the gun source
        direction = target.subNew(visualOrigin);
        var dist = direction.magnitude();
        var target2 = World.getRayCollisionWorld(visualOrigin, direction.normalize(), dist);
        if (target2.isCloseTo(visualOrigin, dist-2)) {
            target = target2;
            targetEntity = null;
        }

        return {
            target: target,
            targetEntity: targetEntity,
        };
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
        if (Global.CLIENT && shooter !== getPlayerEntity()) return;

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


RepeatingShootAction2 = LocalAnimationAction.extend({
    _name: 'ShootAction',
    localAnimation: ANIM_ATTACK2 | ANIM_LOOP,
    canBeCancelled: false,
});


OutOfAmmoAction = Action.extend({
    _name: 'OutOfAmmoAction',
    secondsLeft: 0.5,
    canBeCancelled: false,
    canMultiplyQueue: false,

    doStart: function() {
        this._super.apply(this, arguments);

        GameManager.getSingleton().addHUDMessage("(Out of ammo)", 0xAA8833, 1.5, 0.5);
        Sound.play('gk/imp_05.ogg');
    },
});


