
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

