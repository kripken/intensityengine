
// Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
// This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

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

