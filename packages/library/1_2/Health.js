
// Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
// This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

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
        },

        activate: function() {
            this.connect('onModify_health', this.onHealth);
            this.connect('onModify_spawnStage', this.onSpawnStage);
        },

        clientActivate: function() {
            this.connect('client_onModify_health', this.onHealth);
            this.connect('client_onModify_spawnStage', this.onSpawnStage);

            var that = this;
            var _decideAnimation = this.decideAnimation;
            this.decideAnimation = function() {
                if (that.health > 0) {
                    return _decideAnimation.apply(that, arguments);
                } else {
                    return ANIM_DYING|ANIM_RAGDOLL;
                }
            };
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
            var damage = typeof _source.damage === 'number' ? _source.damage : _source;
            if (this.health > 0 && damage) {
                // TODO: Custom interactions between source and sufferer
                this.health = Math.max(0, this.health - damage);
            }
        },
    },

    //! Use this for Application.clientOnEntityOffMap
    dieIfOffMap: function(entity) {
        if (entity === getPlayerEntity() && entity.health > 0 && entity.state !== CLIENTSTATE.EDITING && entity.state !== CLIENTSTATE.SPECTATOR) {
            entity.health = 0; // Kill instantly
        }
    },

    isValidTarget: function(entity) {
        return !entity.deactivated && entity.health > 0 && entity.clientState !== CLIENTSTATE.EDITING;
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
};

DeadlyArea = registerEntityClass(bakePlugins(AreaTrigger, [Health.deadlyAreaTriggerPlugin, {
    _class: 'DeadlyArea',
}]));

Map.preloadSound('0ad/death_11.ogg');

