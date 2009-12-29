
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


Library.include('library/1_2/Events');

// Phases

TutorialPhaseAction = (ContainerAction.extend(InputCaptureActionPlugin)).extend({ // Mix in input capture
    canBeCancelled: false,
    secondsLeft: 1.0,

    stopHere: false,

    actionKey: function(index, down) {
        if (!down) return;
        if (index === 5) {
            this.stopHere = true;
            this.abort();
            this.finish();
            getEntityByTag('start_people').placeEntity(getPlayerEntity());
            GameManager.getSingleton().addHUDMessage({
                text: 'Tutorial has been canceled.',
                color: 0xDDEEFF,
                duration: 4.0,
                y: 0.2,
                size: 0.6,
            });
            Sound.play('gk/imp_05.ogg');
        } else if (index === 7) {
            Sound.play('gk/imp_01');
            Tutorial.textSizeFactor = Math.max(0.1, Tutorial.textSizeFactor*0.9);
        } else if (index === 8) {
            Sound.play('gk/imp_01');
            Tutorial.textSizeFactor = Math.min(10.0, Tutorial.textSizeFactor*1.11111);
        }
    },

    doStart: function() {
        this._super.apply(this, arguments);

        GameManager.getSingleton().clearHUDMessages();

        Sound.play('0ad/alarmvictory_1.ogg');
    },

    doExecute: function() {
        CAPI.showHUDText("Tutorial Part " + this.phase + ': ' + this.phaseDescription, 0.5, 0.15, 0.8*Tutorial.textSizeFactor, 0xEEEEFF);

        return this._super.apply(this, arguments);
    },

    doFinish: function() {
        this._super.apply(this, arguments);

        GameManager.getSingleton().clearHUDMessages();
    },
});


// Texts

Texts = {
    adjustTextSize: "Adjust the text size with '7','8'.",
    mouselookModeHelpText: "If you can only move the mouse cursor, press 'M'.",
    completeTask: '(complete the task to continue)'
};


// Actions (put on player)

TutorialSubPhaseAction = (NoticeAction.extend(InputCaptureActionPlugin)).extend({ // Mix in input capture
    canBeCancelled: false,
    canMultiplyQueue: true,

    lowerText: '[text missing]',
    lowerText: '(click the mouse to continue)',
    helpText: [
        "If the text is hard to read, move the mouse/press 'M'",
        Texts.adjustTextSize,
    ],

    clientClick: function(button, down) {
        if (down) {
            Sound.play('olpc/AdamKeshen/BeatBoxCHIK.wav');
            this.done = true;
            return true;
//        } else {
//            return this.oldClientClick(button, down);
        }

        return false;
    },

    actionKey: function(index, down) {
        if (index === 0 && down) { // help
            Sound.play('gk/imp_01', getPlayerEntity().position.copy());

            if (typeof this.helpText !== 'object') {
                this.helpText = [this.helpText];
            }
            GameManager.getSingleton().addHUDMessage({
                text: this.helpText[0] + (
                    this.helpText.length > 1 ? "   ['H': more]" : ''
                ),
                color: 0xDDEEFF,
                duration: 5.0,
                y: 0.6,
                size: 0.45*Tutorial.textSizeFactor,
            });

            this.helpText.push(this.helpText.shift()); // cycle
        } else {
            this.oldActionKey(index, down);
        }
    },

    create: function(kwargs) {
        kwargs = defaultValue(kwargs, {});

        this.done = false;

        this._super(kwargs);

        this.upperText = defaultValue(kwargs.upperText, this.upperText);
        this.lowerText = defaultValue(kwargs.lowerText, this.lowerText);
        this.helpText = defaultValue(kwargs.helpText, this.helpText);
    },

    doStart: function() {
        this._super.apply(this, arguments);

        GameManager.getSingleton().clearHUDMessages();
    },

    shouldContinue: function() {
        CAPI.showHUDText(this.upperText, 0.5, 0.275, 0.5*this.currSizeRatio*Tutorial.textSizeFactor, 0xDDEEFF);
        CAPI.showHUDText(this.lowerText, 0.5, 0.375, 0.4*this.currSizeRatio*Tutorial.textSizeFactor, 0xDDEEFF);

        return !this.done;
    },
});


// Specific phases

OrientationSubPhaseAction = TutorialSubPhaseAction.extend({
    upperText: 'Turn to look directly at the sparkling light.',
    lowerText: Texts.completeTask,
    helpText: [
        'Move the mouse to change where you look.',
        "The yellow arrows indicate where you should turn.",
        "Aim the crosshair (center of screen) on the light.",
        Texts.mouselookModeHelpText,
    ],

    clientClick: function(button, down) {
        if (down) {
            Sound.play('olpc/AdamKeshen/BeatBoxCHIK.wav');
            return true;
//            } else {
//                return this.oldClientClick(button, down);
        }
        return false;
    },

    setNextTarget: function() {
        if (this.yaws.length === 0) return -1;

        this.currDeltaYaw = this.yaws.shift();
        this.targetYaw = this.actor.yaw + this.currDeltaYaw;
        return this.yaws.length;
    },

    doStart: function() {
        this.yaws = [45, -55, 90, -150, 65];

        this._super.apply(this, arguments);

        var colors = [0xFFAA88, 0xAAFF99, 0x99AAFF];
        var color = colors[Math.floor(Math.random()*colors.length)];
        this.actor.targetColor = color;
        this.targetPosition = this.actor.getCenter().add(new Vector3(0,0,10));
        this.sendTargetPosition();
        this.positionTimer = new RepeatingTimer(1/25.0); // 25fps

        this.upperTextBase = this.upperText;

        this.setNextTarget();
        this.currYaw = this.targetYaw;

        this.delayTimer = new RepeatingTimer(2.0);
    },

    moveTarget: function(seconds) {
        if (this.targetYaw !== undefined) {
            this.currYaw = moveAngleTowards(this.currYaw, this.targetYaw, 60*seconds);

            this.targetPosition = this.actor.getCenter().add(
                new Vector3().fromYawPitch(this.currYaw, 10).mul(37.5)
            );
        }
    },

    doExecute: function(seconds) {
        this.moveTarget(seconds);

        // Show indicator of yaw
        var yaw = this.targetPosition.subNew(this.actor.getCenter()).toYawPitch().yaw;
        yaw = normalizeAngle(yaw, this.actor.yaw);
        yaw -= this.actor.yaw;
        if (yaw > 10) {
            CAPI.showHUDText('->', 0.9, 0.2, 1.0, 0xFFCC65);
        } else if (yaw < -10) {
            CAPI.showHUDText('<-', 0.1, 0.2, 1.0, 0xFFCC65);
        }

        if (this.positionTimer.tick(seconds)) {
            this.sendTargetPosition();
        }

        if (this.delayTimer.tick(seconds)) {
            if (this.checkTargeting()) {
                Effect.text(this.targetPosition.addNew(new Vector3(0, 0, 10)), 'Well done!', 2.0, 0xFFDD99, 7.0, 0);
                Sound.play('olpc/Berklee44BoulangerFX/rubberband.wav');

                var left = this.setNextTarget() + 1;

                if (left === 0) {
                    this.done = true;
                    return true;
                }
                if (left > 1) {
                    this.upperText = this.upperTextBase + ' [' + left + ' to go]';
                } else {
                    this.upperText = this.upperTextBase + ' [last time]';
                }
/*
                GameManager.getSingleton().addHUDMessage({
                    text: 'Well done!',
                    color: 0xFFDD99,
                    duration: 1.0,
                    y: 0.75,
                    size: 0.5,
                });
*/
            } else {
                this.delayTimer.prime();
            }
        }

        return this._super.apply(this, arguments);
    },

    doFinish: function() {
        this._super.apply(this, arguments);

        this.actor.targetColor = 0;
    },

    sendTargetPosition: function() {
        this.actor.targetPosition = this.targetPosition.asArray();
    },

    checkTargeting: function() {
        this.currYaw = normalizeAngle(this.currYaw, this.actor.yaw);
        return (Math.abs(this.currYaw - this.actor.yaw) < 7 && Math.abs(this.actor.pitch) < 20);
    },
});

MouselookSubPhaseAction = OrientationSubPhaseAction.extend({
    upperText: 'Aim the mouse at the sparkling light.',
    lowerText: Texts.completeTask,
    helpText: ["Press 'M' to toggle mouse mode.",],

    doStart: function() {
        this._super.apply(this, arguments);

        this.yaws = [-45, 40, -35, 15];

        this.originalYaw = this.actor.yaw;
    },

    doExecute: function(seconds) {
        if (Math.abs(this.actor.yaw - this.originalYaw) > 10) {
            GameManager.getSingleton().addHUDMessage({
                text: "You are changing facing. Press 'M', then move the mouse.",
                color: 0xFF6640,
                duration: 3.0,
                y: 0.6,
                size: 0.4,
            });
            Sound.play('gk/imp_01');

            this.originalYaw = this.actor.yaw;
            this.targetYaw = this.actor.yaw + this.currDeltaYaw;
        }

        return this._super.apply(this, arguments);
    },

    checkTargeting: function() {
        var mouseTarget = CAPI.getTargetPosition();
        var mouseYaw = mouseTarget.sub(this.actor.getCenter()).toYawPitch().yaw;
        this.currYaw = normalizeAngle(this.currYaw, mouseYaw);
        return (Math.abs(this.currYaw - mouseYaw) < 20);
    },

});

ShortOrientationSubPhaseAction = OrientationSubPhaseAction.extend({
    upperText: 'Like before, turn to look directly at the light.',
    helpText: [Texts.mouselookModeHelpText, 'Move the mouse to change where you look.'],

    doStart: function() {
        this._super.apply(this, arguments);
        this.yaws = [-45, 45];
    },
});

TutorialPhaseAction1 = TutorialPhaseAction.extend({
    phase: 1,
    phaseDescription: 'Looking Around',

    create: function() {
        this.done = false;

        this._super.apply(this, [[
            new TutorialSubPhaseAction({ upperText: 'Welcome to the Syntensity tutorial!' }),
            new TutorialSubPhaseAction({ upperText: "Press 'H' at any time for help." }),
            new TutorialSubPhaseAction({ upperText: Texts.adjustTextSize }),
            new TutorialSubPhaseAction({ upperText: "Press '5' at any time to quit the tutorial." }),
            new TutorialSubPhaseAction({
                upperText: "Let's practice moving the mouse to look around.",
                helpText: Texts.mouselookModeHelpText,
            }),
            new OrientationSubPhaseAction(),
            new TutorialSubPhaseAction({ upperText: "Looks like you've got it!" }),
            new TutorialSubPhaseAction({ upperText: "Now let's learn how to move the mouse cursor." }),
            new TutorialSubPhaseAction({ upperText: "Press 'M' to toggle the mouse mode." }),
            new MouselookSubPhaseAction(),
            new TutorialSubPhaseAction({ upperText: "Excellent!" }),
            new TutorialSubPhaseAction({ upperText: "Press 'M' to return to the original mouse mode." }),
            new TutorialSubPhaseAction({ upperText: "Let's quickly practice changing where you look again." }),
            new ShortOrientationSubPhaseAction(),
            new TutorialSubPhaseAction({ upperText: "You're doing great!" }),
        ]]);
    },

    doStart: function() {
        this._super.apply(this, arguments);

        var markers = getEntitiesByTag('tutorial_1');
        var factor = Math.random();
        var position = markers[0].position.mulNew(factor).add(markers[1].position.mulNew(1-factor));
        var yaw = markers[0].yaw*factor + markers[1].yaw*(1-factor);
        this.actor.position = position;
        this.actor.yaw = -yaw;

        this.actor.canMove = false; // This is a non-moving tutorial
    },

    doFinish: function() {
        this._super.apply(this, arguments);

        this.actor.canMove = true;

        if (!this.stopHere) this.actor.queueAction(new TutorialPhaseAction2());
    },
});

/////////// Phase 2

MovementSubPhaseAction = OrientationSubPhaseAction.extend({
    upperText: 'Walk to the sparkling light.',
    lowerText: Texts.completeTask,
    helpText: [
        "Use the 'W','A','S','D' or the arrow keys to move.",
        "Move the mouse to look around ('M' may help).",
        "The yellow arrows indicate where you should turn.",
    ],

    setNextTarget: function() {
        if (this.offsets.length === 0) return -1;

        this.nextTargetPosition = this.center.addNew(this.offsets.shift());
        return this.offsets.length;
    },

    moveTarget: function(seconds) {
        if (this.nextTargetPosition) {
            var factor = seconds;
            this.targetPosition.mul(1-factor).add(this.nextTargetPosition.mulNew(factor));   
        }
    },

    doStart: function() {
        this.offsets = [
            new Vector3(0, 30, 0),
            new Vector3(-25, -50, 0),
            new Vector3(-44, 33, 0),
            new Vector3(35, 50, 0),
            new Vector3(0, -50, 0),
            new Vector3(0, 20, 0),
        ];

        var markers = getEntitiesByTag('tutorial_2');
        var factor = Math.random();
        this.center = markers[0].position.mulNew(factor).add(markers[1].position.mulNew(1-factor));

        this._super.apply(this, arguments);
    },

    doExecute: function(seconds) {

        return this._super.apply(this, arguments);
    },

    checkTargeting: function() {
        return this.targetPosition.subNew(this.actor.getCenter()).magnitude() < 10;
    },
});

JumpingSubPhaseAction = TutorialSubPhaseAction.extend({
    upperText: 'Jump a few times.',
    lowerText: Texts.completeTask,
    helpText: ['Press the space bar to jump.'],

    clientClick: null,

    doStart: function() {
        this._super.apply(this, arguments);

        this.jumpsLeft = 3;

        this.upperTextBase = this.upperText;

        this.baselineZ = this.actor.position.z;

        this.delayTimer = new RepeatingTimer(1.0);
    },

    doExecute: function(seconds) {
        this.baselineZ = Math.min(this.baselineZ, this.actor.position.z);
        if (this.delayTimer.tick(seconds)) {
            if (this.actor.position.z > this.baselineZ + 0.75) {
                Sound.play('olpc/Berklee44BoulangerFX/rubberband.wav');
                this.jumpsLeft -= 1;

                if (this.jumpsLeft > 1) {
                    this.upperText = this.upperTextBase + ' [' + this.jumpsLeft + ' to go]';
                } else {
                    this.upperText = this.upperTextBase + ' [last time]';
                }
            } else {
                this.delayTimer.prime();
            }
        }

        this._super.apply(this, arguments);

        return this.jumpsLeft === 0;
    },
});

TutorialPhaseAction2 = TutorialPhaseAction.extend({
    phase: 2,
    phaseDescription: 'Moving',

    create: function() {
        this.done = false;

        this._super.apply(this, [[
            new TutorialSubPhaseAction({ upperText: "Now let's practice moving around." }),
            new TutorialSubPhaseAction({ upperText: "Use the 'W','A','S','D' or the arrow keys to move." }),
            new MovementSubPhaseAction(),
            new TutorialSubPhaseAction({ upperText: "You've got it!" }),
            new TutorialSubPhaseAction({ upperText: "Press the space bar to jump." }),
            new JumpingSubPhaseAction(),
            new TutorialSubPhaseAction({ upperText: "Nicely done!" }),
        ]]);
    },

    doFinish: function() {
        this._super.apply(this, arguments);

        if (!this.stopHere) this.actor.queueAction(new TutorialPhaseAction3());
    },
});


////////// 3

TutorialTryItSubPhaseAction = TutorialSubPhaseAction.extend({
    lowerText: '(try it a few times, then click the mouse to continue)',
});

TutorialPhaseAction3 = TutorialPhaseAction.extend({
    phase: 3,
    phaseDescription: 'Other Controls',

    create: function() {
        this.done = false;

        this._super.apply(this, [[
            new TutorialSubPhaseAction({ upperText: "Now let's quickly learn some of the other controls." }),
            new TutorialTryItSubPhaseAction({ upperText: "Scrolling with the mouse wheel moves the camera." }),
            new TutorialTryItSubPhaseAction({ upperText: "Pressing '9' will toggle first person mode." }),
            new TutorialTryItSubPhaseAction({ upperText: "Hold down 'Tab' to see who else is in the same world." }),
            new TutorialTryItSubPhaseAction({
                upperText: "Press 'Escape' to bring up (or close) the main menu.",
                helpText: "Press 'Escape' to close the main menu",
            }),
            new TutorialTryItSubPhaseAction({ upperText: "(You can mute the music in the menu 'options'->'audio'.)" }),
            new TutorialSubPhaseAction({ upperText: "One final thing:" }),
            new TutorialSubPhaseAction({ upperText: "You can text (message) everyone in the same world." }),
            new TutorialSubPhaseAction({ upperText: "'T' starts a message (Enter sends, Escape cancels)." }),
            new TutorialSubPhaseAction({
                upperText: "That's it, you completed the tutorial!",
                lowerText: '(click the mouse to finish)',
            }),
        ]]);
    },

    doFinish: function() {
        this._super.apply(this, arguments);

        Sound.play('0ad/alarmvictory_1.ogg');
        getEntityByTag('start_people').placeEntity(getPlayerEntity());

        GameManager.getSingleton().addHUDMessage({
            text: 'Now go have fun :)',
            color: 0xEEF3FF,
            duration: 3.5,
            y: 0.333,
            size: 0.666,
        });
    },
});


// Main namespace

Tutorial = {
    textSizeFactor: 1.0,

    playerPlugin: {
        targetColor: new StateInteger(),
        targetPosition: new StateArrayFloat({ reliable: false }),

        init: function() {
            this.targetColor = 0;
            this.targetPosition = [0,0,0];
        },

        clientActivate: function() {
            this.currTargetPosition = new Vector3(0,0,0);
        },

        clientAct: function(seconds) {
            if (this.targetColor !== 0) {
                // Interp
                var nextPosition = new Vector3(this.targetPosition.asArray());
                var factor = seconds*10;
                if (nextPosition.subNew(this.currTargetPosition).magnitude() > 100) {
                    factor = 1.0;
                }
                this.currTargetPosition.mul(1-factor).add(
                    nextPosition.mul(factor)
                );

                // Render
                var renderPosition = this.currTargetPosition.copy();
                renderPosition.x += Math.sin(0.1 + Global.time*2.5)*2;
                renderPosition.y += Math.sin(0.9 - Global.time*2.5)*2;
                renderPosition.z += Math.sin(1.7 + Global.time*2.14159)*2;
//log(ERROR, this.targetPosition.asArray().toString() + ',' + renderPosition.toString() + ',' + this.currTargetPosition.toString());
                Effect.splash(PARTICLE.SPARK, 30, (0.1+seconds*5)/2, renderPosition, this.targetColor, 1.0, 70, 1);
                Effect.addDynamicLight(renderPosition, 30, this.targetColor);
            }
        },
    },

    managerPlugin: {
    },

    clientClick: function(button, down, position, entity) {
        if (entity && entity.clientClick) {
            entity.clientClick();
        }
        return true;
    },

    actionKey: function(index, down) {
        if (index === 0 && down) {
            getPlayerEntity().queueAction(new TutorialPhaseAction1());
        }
    },
};


Map.preloadSound('0ad/alarmvictory_1.ogg');
Map.preloadSound('olpc/AdamKeshen/BeatBoxCHIK.wav');
Map.preloadSound('gk/imp_01');
Map.preloadSound('olpc/Berklee44BoulangerFX/rubberband.wav');

// TODO: Move players starting tutorial so they are not all on top of each other

