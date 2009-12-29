
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
        this.actor.animation = ANIM_DYING|ANIM_RAGDOLL;
    },

    doFinish: function() {
        this.actor.respawn();
    }
});


Health = {
    maxHealth: function(entity) {
        return 100;
    },

    plugin: {
        //! clientSet for health means that when we shoot someone, we get
        //! immediate feedback - no need to wait for server response
        health: new StateInteger({ clientSet: true }),

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
                            this.health = Health.maxHealth(this); // Do this first
                            this.canMove = true; // Note bug - if set to cannot move by e.g. the game ending, and come
                                                 // back to life after that - then can move
                            this.modelName = this.defaultModelName; // Show

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

        activate: function() {
            this.connect('onModify_health', this.onHealth);
            this.connect('onModify_spawnStage', this.onSpawnStage);
        },

        clientActivate: function() {
            this.connect('client_onModify_health', this.onHealth);
            this.connect('client_onModify_spawnStage', this.onSpawnStage);
        },

        clientAct: function() {
            if (this !== getPlayerEntity()) return;

            // HUD
            if (CAPI.showHUDImage) {
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
                    Sound.play("0ad/death_11.ogg", this.position);
                    if (!serverOriginating || health > 0) {
                        this.queueAction(new PainAction());
                    }
                    if (this === getPlayerEntity() && this.oldHealth !== health) {
                        Effect.clientDamage(diff, diff);
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
            this.health = Math.max(0, this.health - _source.damage); // TODO: Custom interactions between source and sufferer
        },
    },

    //! Use this for Application.clientOnEntityOffMap
    dieIfOffMap: function(entity) {
        if (entity === getPlayerEntity() && entity.health > 0 && entity.state !== CLIENTSTATE.EDITING && entity.state !== CLIENTSTATE.SPECTATOR) {
            entity.health = 0; // Kill instantly
        }
    },

    isValidTarget: function(entity) {
        return !entity.deactivated && entity.health > 0; // Should have this, but isn't valid on server due to bug: && entity.clientState !== CLIENTSTATE.EDITING;
    },

    isActiveEntity: function(entity) {
        return this.isValidTarget(entity);
    },
};

Map.preloadSound('0ad/death_11.ogg');

