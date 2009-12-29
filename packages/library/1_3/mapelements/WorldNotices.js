
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


WorldNotice = registerEntityClass(bakePlugins(AreaTrigger, [{
    _class: 'WorldNotice',
    shouldAct: true,

    text: new StateString(),
    color: new StateInteger(),
    size: new StateFloat(),

    init: function() {
        this.text = 'World notice text';
        this.color = 0xFFFFFF;
        this.size = 0.5;
    },

    clientActivate: function() {
        this.colliding = false;
    },

    clientAct: function(seconds) {
    },

    clientOnCollision: function(entity) {
        if (entity !== getPlayerEntity()) return;

        if (!this.noticeAction) {
            this.noticeAction = new WorldNoticeAction();
            this.queueAction(this.noticeAction);
        }

        this.colliding = true;
    },
}]));


NoticeAction = Action.extend({
    canMultiplyQueue: false,

    x: 0.5,
    y: 0.88,

    doStart: function() {
        this.currTime = 0;
        this.currSizeRatio = 0;
    },

    doExecute: function(seconds) {
        if (this.shouldContinue()) {
            if (this.currTime === 0) {
                Sound.play('gk/imp_01', getPlayerEntity().position.copy());
            }

            this.currTime += seconds*3;
            this.currTime = Math.min(Math.PI/2, this.currTime);
            this.currSizeRatio = Math.sin(this.currTime);
            var currSize = this.currSizeRatio*this.size;
        } else {
            this.currTime -= seconds*4;
            this.currTime = Math.max(0, this.currTime);
            this.currSizeRatio = Math.sin(this.currTime);
            var currSize = this.currSizeRatio*this.size;
        }

        if (this.currTime !== 0 && this.text) {
            CAPI.showHUDText(this.text, this.x, this.y, currSize, this.color);
        }

        return this.currTime === 0;
    },
});


WorldNoticeAction = NoticeAction.extend({
    doStart: function() {
        this._super();

        this.text = this.actor.text;
        this.color = this.actor.color;
        this.size = this.actor.size;
    },

    shouldContinue: function() {
        var ret = this.actor.colliding;
        this.actor.colliding = false; // Reset to false, unless a collision occurs and sets to true
        return ret;
    },

    doFinish: function() {
        this._super();

        this.actor.noticeAction = null;
    },
});


Map.preloadSound('gk/imp_01');

