
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


PainAction = LocalAnimationAction.extend({
    _name: 'PainAction',
    secondsLeft: 0.6,
    localAnimation: ANIM_PAIN,
    canMultiplyQueue: false,
});

// Signals (server-side): fragged, respawn
DeathAction = Action.extend({
    _name: 'DeathAction',
    canMultiplyQueue: false,
    canBeCancelled: false,
    secondsLeft: 5.5,

    doStart: function() {
        this.actor.emit('fragged');
        this.actor.clearActions(); // this won't clear us, as we cannot be cancelled
        this.actor.canMove = false;
//  XXX Done with health = 0 now.       this.actor.animation = ANIM_DYING|ANIM_RAGDOLL;
    },

    doFinish: function() {
        this.actor.respawn();
    }
});


Health = {
    plugin: {
        //! clientSet for health means that when we shoot someone, we get
        //! immediate feedback - no need to wait for server response
        health: new StateInteger({ clientSet: true }),

        maxHealth: new StateInteger({ clientSet: true }),

        spawnStage: new StateInteger(),

        onSpawnStage: function(stage, actorUniqueId) {
            switch (stage) {
                case 1:
                    { // Client ack
                        if (Global.CLIENT) {
                            this.spawnStage = 2;
                        }
                    }
                    break;
                case 2:
                    { // Server vanishes player
                        // Start this stage when the entity itself acks, not others,
                        // but TODO consider requiring an ack from them all.
                        // Note that stage 1 sends from all clients -- client may not
                        // even know its uniqueId yet at this stage, or not have getPlayerEntity
                        if (Global.SERVER) {
                            if (actorUniqueId === this.uniqueId) {
                                this.modelName = '';
                                this.animation = ANIM_IDLE|ANIM_LOOP;

                                this.spawnStage = 3;
                            }

                            throw "CancelStateDataUpdate"; // No need to send this to clients, the last line is what we need
                        }
                    }
                    break;
                case 3:
                    { // Client repositions, etc.
                        if (Global.CLIENT && this === getPlayerEntity()) {
                            this.emit('clientRespawn');

                            this.spawnStage = 4;
                        }
                    }
                    break;
                case 4:
                    { // Server appears player and sets in motion
                        if (Global.SERVER) {
                            this.health = this.maxHealth; // Do this first
                            this.canMove = true; // Note bug - if set to cannot move by e.g. the game ending, and come
                                                 // back to life after that - then can move
                            this.modelName = this.defaultModelName; // Show
                            if (this.defaultHUDModelName) {
                                this.HUDModelName = this.defaultHUDModelName; // Show
                            }

                            this.spawnStage = 0;

                            throw "CancelStateDataUpdate"; // No need to send this to clients, the last line is what we need
                        }
                    }
                    break;
            }
        },

        respawn: function() {
            this.spawnStage = 1;
        },

        init: function() {
            this.maxHealth = 100; // default
            this.health = this.maxHealth; // So even before we spawn, we have a valid health value
        },

        activate: function() {
            this.connect('onModify_health', this.onHealth);
            this.connect('onModify_spawnStage', this.onSpawnStage);
        },

        clientActivate: function() {
            this.connect('client_onModify_health', this.onHealth);
            this.connect('client_onModify_spawnStage', this.onSpawnStage);
        },

        decideAnimation: function() {
            if (this.health > 0) {
                return this._super.apply(this, arguments);
            } else {
                return ANIM_DYING|ANIM_RAGDOLL;
            }
        },

        decideActionAnimation: function() {
            var ret = this._super.apply(this, arguments);

            // Clean up dying animation stuff when we are not dead
            if (this.health > 0 && (ret === ANIM_DYING || ret === (ANIM_DYING|ANIM_RAGDOLL))) {
                this.setLocalAnimation(ANIM_IDLE|ANIM_LOOP);
                ret = this.animation;
            }

            return ret;
        },

        clientAct: function() {
            if (this !== getPlayerEntity()) return;

            // HUD
            if (CAPI.showHUDImage) {
                if (!Global.gameHUD) {
                    var health = integer(this.health);
                    if (health || health === 0) {
                        var color, icon = 'packages/hud/gui_gk_Icon_s01.png';
                        if (health > 75) {
                            color = 0x88FFAA;
                        } else if (health > 33) {
                            color = 0xCCDD67;
                        } else {
                            color = 0xFF4431;
                            if (health === 0) {
                                icon = 'packages/hud/gui_gk_Icon_s02.png';
                            }
                        }
                        CAPI.showHUDImage(icon, 0.88, 0.88, 0.05, 0.05);
                        CAPI.showHUDText(health.toString(), 0.94, 0.88, 0.5, color);
                    }
                } else {
                    var raw = Math.floor(34*this.health/this.maxHealth);
                    var whole = Math.floor(raw/2);
                    var half = raw > whole*2;
                    var params = Global.gameHUD.getHealthParams();
                    CAPI.showHUDImage('packages/gamehud/gk/swarm/gk_pc_h_pbar_' +
                        (whole >= 10 ? whole : '0' + clamp(whole, 1, 100)) +
                        (half ? '_5' : '') + '.png',
                        params.x, params.y, params.w, params.h // .0.9, 0.5, 0.04, 0.4
                    );
                }
            }
        },

        //! We handle pain on the client, and death on the server. This
        //! reflects that death must be handled carefully from a central
        //! location, but we do want a client that shoots someone to
        //! immediately get feedback in the form of a pain sound
        //! and animation.
        //! Note that we are careful to not play a pain animation in
        //! the case of death, as the server is already driving
        //! the animation, and we do not want to fight with it. However,
        //! we *do* show a pain animation even if death will occur, if we
        //! are the ones changing the health, i.e., we are the shooter.
        //! So we see a pain animation until the server overrides us with
        //! death, if we are the shooter.
        onHealth: function(health, serverOriginating) {
            if (this.oldHealth !== undefined && health < this.oldHealth) {
                var diff = this.oldHealth - health;

                if (Global.CLIENT) {
                    if (diff >= 5) {
                        Sound.play("0ad/death_11.ogg", this.position);
                        if (!serverOriginating || health > 0) {
                            this.queueAction(new PainAction());
                        }
                        if (this === getPlayerEntity() && this.oldHealth !== health) {
                            Effect.clientDamage(diff, diff);
                        }
                    }
                } else {
                    if (health <= 0) {
                        this.queueAction(new DeathAction());
                    }
                }
            }
            this.oldHealth = health;
        },

        sufferDamage: function(_source) {
            var damage = typeof _source.damage === 'number' ? _source.damage : _source;
            if (this.health > 0 && damage) {
                // TODO: Custom interactions between source and sufferer
                this.health = Math.max(0, this.health - damage);
            }
        },
    },

    //! Use this for Application.clientOnEntityOffMap
    dieIfOffMap: function(entity) {
        if (entity === getPlayerEntity() && Health.isValidTarget(entity)) {
            entity.health = 0; // Kill instantly
        }
    },

    isValidTarget: function(entity) {
        return entity && entity !== null && !entity.deactivated && entity.health > 0 && entity.clientState !== CLIENTSTATE.EDITING && (entity.spawnStage === 0 || entity.spawnStage === undefined) && entity.clientState !== CLIENTSTATE.LAGGED;
    },

    isActiveEntity: function(entity) {
        return this.isValidTarget(entity);
    },

    deadlyAreaTriggerPlugin: {
        clientOnCollision: function(entity) {
            if (entity !== getPlayerEntity()) return;

            if (Health.isValidTarget(entity)) {
                entity.health = 0;
            }
        },
    },

    //! Additional plugins provided by community members (a la django contrib.*)
    Contrib: {
        //! make water deadly, by quaker66
        //! run it under ClientActivate in your GamePlayer part
        //! needs to be initialized just once
        //! parameters:
        //! countnum - number of seconds to wait before killing begins
        //! healthrestore - amount of health to restore every sec if you get out on time
        //! healthlose - amount of health to lose every sec if you dont get out in limit
        //! btext - text shown before number when in limit
        //! gtext - text shown when after limit
        //! bcolor - color of in-limit text
        //! gcolor - color of not-in-limit text
        //! pos1, pos2, pos3 - coordinations of HUD text
        //! hidehud - set to 1 if you dont want the counting text
        WaterBreathingPlugin: {
            start: function(params) { with(params) {
                var underwatercounter = 0;
                var damaged = 0;
                var count = 0;

                var countnum = defaultValue(countnum, 50);
                var healthrestore = defaultValue(healthrestore, 10);
                var healthlose = defaultValue(healthlose, 10);
                var btext = defaultValue(btext, "Water breathing:");
                var gtext = defaultValue(gtext, "Get out!");
                var bcolor = defaultValue(bcolor, 0xFFFFFF);
                var gcolor = defaultValue(gcolor, 0xFF0000);
                var pos1 = defaultValue(pos1, 0.5);
                var pos2 = defaultValue(pos2, 0.6);
                var pos3 = defaultValue(pos3, 0.5);
                var pos3 = defaultValue(pos3, 0.5);
                var hidehud = defaultValue(hidehud, 0);

                GameManager.getSingleton().eventManager.add({
                    secondsBefore: 0,
                    secondsBetween: 1,
                    func: bind(function() {
                        if (CAPI.getMaterial(entity.position.x, entity.position.y, entity.position.z + 15) === MATERIAL.WATER && !isPlayerEditing(this)) {
                            underwatercounter += 1;
                            if (underwatercounter >= countnum) {
                                if (entity.health < healthlose) {
                                    entity.health = 0;
                                    damaged = 0;
                                } else {
                                    entity.health -= healthlose;
                                    damaged += healthlose;
                                }
                            }
                        }
                        else {
                            underwatercounter = 0;
                            if (damaged > 0) {
                                if (damaged > healthrestore) {
                                    entity.health += healthrestore;
                                    damaged -= healthrestore;
                                }
                                else {
                                    entity.health += damaged;
                                    damaged = 0;
                                }
                            }
                        }
                    }, entity),
                    entity: entity,
                });
                if (!hidehud) {
                    GameManager.getSingleton().eventManager.add({
                        secondsBefore: 0,
                        secondsBetween: 0,
                        func: bind(function() {
                            if (CAPI.showHUDRect) {
                                if (CAPI.getMaterial(entity.position.x, entity.position.y, entity.position.z + 15) === MATERIAL.WATER) {
                                    count = countnum - underwatercounter
                                    if (count > 0)
                                        CAPI.showHUDText(btext+" "+count, pos1, pos2, pos3, bcolor);
                                    else
                                        CAPI.showHUDText(gtext, pos1, pos2, pos3, gcolor);
                                    }
                                }
                            }, entity),
                        entity: entity,
                    });
                }
            } },
        },
    },
};

DeadlyArea = registerEntityClass(bakePlugins(AreaTrigger, [Health.deadlyAreaTriggerPlugin, {
    _class: 'DeadlyArea',
}]));

Map.preloadSound('0ad/death_11.ogg');

