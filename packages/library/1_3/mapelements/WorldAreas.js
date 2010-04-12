
// Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
// This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

Library.include('library/' + Global.LIBRARY_VERSION + '/Events');

WorldAreas = {
    //! The currently active area. Can only have *ONE* at a time
    active: null,

    plugins: {
        Core: {
            shouldAct: true,

            clientOnCollision: function(entity) {
                if (entity !== getPlayerEntity()) return;

                if (WorldAreas.active) return; // Cannot have more than one active. Might be this one, or another - in both cases stop
                WorldAreas.active = this;
                this.queueAction(new WorldAreas.InputCaptureAction());
            },
        },

/* TODO: Make WorldNotices a plugin here
        Notice: {
            clientActivate: function() {
                this.worldArea.notice = {
                    text
                };
            },
        },
*/
    },

    Action: Action.extend({
        doStart: function() {
            this._super();

            eval(assert(' WorldAreas.active === this.actor '));
        },

        doExecute: function(seconds) {
            if (World.isPlayerCollidingEntity(getPlayerEntity(), this.actor)) {
                this.actor.emit('worldAreaActive')
                return false;
            } else
                return true;
        },

        doFinish: function() {
            this._super();

            WorldAreas.active = null;
        },
    }),
};

WorldAreas.InputCaptureAction = WorldAreas.Action.extend(InputCaptureActionPlugin).extend({
    doStart: function() {
        this.clientClick = permanentBind(this.actor.clientClick, this.actor);
        this.actionKey = permanentBind(this.actor.actionKey, this.actor);
        this.performMovement = permanentBind(this.actor.performMovement, this.actor);
        this.performMousemove = permanentBind(this.actor.performMousemove, this.actor);
        this.performJump = permanentBind(this.actor.performJump, this.actor);

        this._super();
    },
});

