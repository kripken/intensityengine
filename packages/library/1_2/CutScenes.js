
// Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
// This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

Library.include('library/1_2/Events.js');

Cutscenes = {
    //! Basic cutscene action: Keeps the player entirely still (ignore mouse movements, even, and hide crosshair).
    //! Queue this action on the player entity.
    BaseAction = ContainerAction.extend({
        canBeCancelled: false,

        /* Extend this with something like
        create: function() {
            this._super([
                new CutScenePart1Action(),
                new CutScenePart2Action(),
                new CutScenePart3Action(),
            ]);
        },
        */

        doStart: function() {
            this.actor.canMove = false;

            this.originalYaw = this.actor.yaw;
            this.originalPitch = this.actor.pitch;
            this.originalRoll = this.actor.roll;

            this.oldCrosshair = ApplicationManager.instance.getCrosshair;
            ApplicationManager.instance.getCrosshair = function() { return ''; };

            this._super();
        },

        doExecute: function(seconds) {
            this.actor.yaw = this.originalYaw;
            this.actor.pitch = this.originalPitch;
            this.actor.roll = this.originalRoll;
            
            return this._super(seconds);
        },

        doFinish: function() {
            ApplicationManager.instance.getCrosshair = this.oldCrosshair;

            this.actor.canMove = true;

            this._super();
        },
    }),
};

/*
Example cutscene action
CutScenePart5ExampleAction = Action.extend({
    secondsLeft: 1.6,

    create: function(target, kwargs) {
        this.target = target;

        this._super(kwargs);
    },

    doStart: function() {
        this.origin = CAPI.getCamera();
        this.total = 0;
    },

    doExecute: function(seconds) {
        var factor = this.total;
        var position = this.origin.position.mulNew(1-factor).add(this.target.mulNew(factor));

        CAPI.forceCamera(
            position.x, position.y, position.z,
            this.origin.yaw*(1-factor) + this.actor.yaw*factor,
            this.origin.pitch*(1-factor) + this.actor.pitch*factor,
            0
        );
        this.total += seconds/1.6;
        return this._super(seconds);
    },

    doFinish: function() {
        this.actor.canMove = true;
        this.actor.lastCamera = undefined;
    },
});
/*

