
// Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
// This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

Library.include('library/1_2/Events');

WorldAreas = {
    plugins: {
        Core: {
            shouldAct: true,

            clientActivate: function() {
                this.worldArea = {
                    colliding: false,
                    action: null,
                    clientClick: null,
                    actionKey: null,
                };
            },

            clientOnCollision: function(entity) {
                if (entity !== getPlayerEntity()) return;

                this.worldArea.colliding = true;

                if (!this.worldArea.action) {
                    this.worldArea.action = new WorldAreas.InputCaptureAction();
                    this.queueAction(this.worldArea.action);
                }
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
        doExecute: function() {
            var ret = !this.actor.worldArea.colliding;
            this.actor.worldArea.colliding = false; // Reset to false, unless a collision occurs and sets to true
            if (!ret) {
                this.actor.emit('worldAreaActive')
            }
            return ret;
        },

        doFinish: function() {
            this._super();

            this.actor.worldArea.action = null;
        },
    }),
};

WorldAreas.InputCaptureAction = WorldAreas.Action.extend(InputCaptureActionPlugin).extend({
    doStart: function() {
        this.clientClick = this.actor.worldArea.clientClick;
        this.actionKey = this.actor.worldArea.actionKey;

        this._super();
    },
});

