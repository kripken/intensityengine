
//=============================================================================
// Copyright (C) 2008 Alon Zakai ('Kripken') kripkensteiner@gmail.com
//
// This file is part of the Intensity Engine project,
//    http://www.intensityengine.com
//
// The Intensity Engine is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published by
// the Free Software Foundation, version 3.
//
// The Intensity Engine is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with the Intensity Engine.  If not, see
//     http://www.gnu.org/licenses/
//     http://www.gnu.org/licenses/agpl-3.0.html
//=============================================================================


Library.include('library/' + Global.LIBRARY_VERSION + '/Events.js');

CutScenes = {
    //! Basic cutscene action: Keeps the player entirely still (ignore mouse movements, even, and hide crosshair).
    //! Queue this action on the player entity.
    BaseAction: ContainerAction.extend({
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

            this.oldShowHUDText = CAPI.showHUDText;
            this.oldShowHUDRect = CAPI.showHUDRect;
            this.oldShowHUDImage = CAPI.showHUDImage;

            CAPI.showHUDText = CAPI.showHUDRect = CAPI.showHUDImage = function() { };

            this.originalSecondsLeft = this.secondsLeft;

            this._super();
        },

        doExecute: function(seconds) {
            this.actor.yaw = this.originalYaw;
            this.actor.pitch = this.originalPitch;
            this.actor.roll = this.originalRoll;

            if (this.subtitles) {
                this.showSubtitles(this.originalSecondsLeft - this.secondsLeft);
            }

            return this._super(seconds) || isPlayerEditing();
        },

        doFinish: function() {
            ApplicationManager.instance.getCrosshair = this.oldCrosshair;

            this.actor.canMove = true;

            CAPI.showHUDText = this.oldShowHUDText;
            CAPI.showHUDRect = this.oldShowHUDRect;
            CAPI.showHUDImage = this.oldShowHUDImage;

            this._super();
        },

        showSubtitles: function(time) {
            forEach(this.subtitles, function(subtitle) {
                if (time >= subtitle.start && time <= subtitle.end) {
                    if (this.showSubtitleBackground) {
                        this.showSubtitleBackground();
                    }
                    this.oldShowHUDText(subtitle.text, subtitle.x, subtitle.y, subtitle.size, subtitle.color);
                }
            }, this);
        },
    }),

    Subtitle: Class.extend({
        start: 0,
        end: 0,
        text: '',
        x: 0.5,
        y: 0.92,
        size: 0.5,
        color: 0xffffff,
    }),

    // A subaction in a cutscene, that moves smoothly between some positions+orientations
    SmoothAction: Action.extend({
        secondsPerMarker: 4.0,
        delayBefore: 0,
        delayAfter: 0,

        doStart: function() {
            this._super();

            this.timer = -this.secondsPerMarker/2 - this.delayBefore;
            this.secondsLeft = this.secondsPerMarker*this.markers.length;
        },

        doExecute: function(seconds) {
            this.timer += Math.min(seconds, 1/25); // Get around loading time delays by ignoring long frames
            this.setMarkers();

            CAPI.forceCamera(
                this.position.x, this.position.y, this.position.z, this.yaw, this.pitch, 0
            );

            this._super(seconds);

            return this.secondsLeft <= -this.delayAfter-this.delayBefore;
        },

        smoothFunc: function(x) {
            if (x <= -0.5) {
                return 0; // 0 until -0.5
            } else if (x >= 0.5) {
                return 1;
            } else {
                return 0.5*Math.pow(Math.abs(x+0.5), 2); // Gives 0 for -0.5, 0.5 for 0.5
            }
        },

        setMarkers: function() {
            var raw = this.timer/this.secondsPerMarker;
            var currIndex = clamp(Math.floor(raw + 0.5), 0, this.markers.length-1);

            var alpha = this.smoothFunc(currIndex - raw); //! How much to give the previous
            var beta = this.smoothFunc(raw - currIndex); //! How much to give the next

            var lastMarker = this.markers[clamp(currIndex-1, 0, this.markers.length-1)];
            var currMarker = this.markers[clamp(currIndex, 0, this.markers.length-1)];
            var nextMarker = this.markers[clamp(currIndex+1, 0, this.markers.length-1)];

            this.position = lastMarker.position.mulNew(alpha).
                add(currMarker.position.mulNew(1-alpha-beta)).
                add(nextMarker.position.mulNew(beta));
            this.yaw = normalizeAngle(lastMarker.yaw, currMarker.yaw)*alpha +
                       currMarker.yaw*(1-alpha-beta) +
                       normalizeAngle(nextMarker.yaw, currMarker.yaw)*beta;
            this.pitch = normalizeAngle(lastMarker.pitch, currMarker.pitch)*alpha +
                       currMarker.pitch*(1-alpha-beta) +
                       normalizeAngle(nextMarker.pitch, currMarker.pitch)*beta;
        },
    }),
};



//Example cutscene action
//CutScenePart5ExampleAction = Action.extend({
//    secondsLeft: 1.6,
//
//    create: function(target, kwargs) {
//        this.target = target;
//
//        this._super(kwargs);
//    },
//
//    doStart: function() {
//        this.origin = CAPI.getCamera();
//        this.total = 0;
//    },
//
//    doExecute: function(seconds) {
//        var factor = this.total;
//        var position = this.origin.position.mulNew(1-factor).add(this.target.mulNew(factor));
//
//        CAPI.forceCamera(
//            position.x, position.y, position.z,
//            this.origin.yaw*(1-factor) + this.actor.yaw*factor,
//            this.origin.pitch*(1-factor) + this.actor.pitch*factor,
//            0
//        );
//        this.total += seconds/1.6;
//        return this._super(seconds);
//    },
//
//    doFinish: function() {
//        this.actor.canMove = true;
//        this.actor.lastCamera = undefined;
//    },
//});

