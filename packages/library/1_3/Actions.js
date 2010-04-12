
// Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
// This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

// Must reflect shared/ents.h

ANIM_DEAD = 0;
ANIM_DYING = 1;
ANIM_IDLE = 2;
ANIM_FORWARD = 3;
ANIM_BACKWARD = 4;
ANIM_LEFT = 5;
ANIM_RIGHT = 6;
ANIM_HOLD1 = 7;
ANIM_HOLD2 = 8;
ANIM_HOLD3 = 9;
ANIM_HOLD4 = 10;
ANIM_HOLD5 = 11;
ANIM_HOLD6 = 12;
ANIM_HOLD7 = 13;
ANIM_ATTACK1 = 14;
ANIM_ATTACK2 = 15;
ANIM_ATTACK3 = 16;
ANIM_ATTACK4 = 17;
ANIM_ATTACK5 = 18;
ANIM_ATTACK6 = 19;
ANIM_ATTACK7 = 20;
ANIM_PAIN = 21;
ANIM_JUMP = 22;
ANIM_SINK = 23;
ANIM_SWIM = 24;
ANIM_EDIT = 25;
ANIM_LAG = 26;
ANIM_TAUNT = 27;
ANIM_WIN = 28;
ANIM_LOSE = 29;
ANIM_GUN_IDLE = 30;
ANIM_GUN_SHOOT = 31;
ANIM_VWEP_IDLE = 32;
ANIM_VWEP_SHOOT = 33;
ANIM_SHIELD = 34;
ANIM_POWERUP = 35;
ANIM_MAPMODEL = 36;
ANIM_TRIGGER = 37;
NUMANIMS = 38;

ANIM_INDEX = 0x7F;
ANIM_LOOP = 1 << 7;
ANIM_START = 1 << 8;
ANIM_END = 1 << 9;
ANIM_REVERSE = 1 << 10;
ANIM_SECONDARY = 11;

ANIM_RAGDOLL = 1 << 27;


//! An activity that a logic entity can perform. Actions are both server- and client-side, but might do different
//! things on each.
Action = Class.extend({
    //! The name of this action type. Overridden in children
    _name: "OVERRIDETHIS",

    create: function(kwargs) {
        if (kwargs === undefined) {
            kwargs = {};
        }

        //! Whether the action has begun. Is set in execute() to 'true'.
        this.begun = false;

        //! Whether the action is finished. Set in finish().
        this.finished = false;

        //! When the action began.
        this.startTime = CAPI.currTime();

        //! How much time is left for the action to run, until it completes.
        if (this.secondsLeft === undefined) { // Allows syntax of setting these values in the class (see Actions__test.js)
            this.secondsLeft = defaultValue(kwargs.secondsLeft, 0);
        }

        //! A default animation, if we have one, for the entity performing this action, while the action is running
        if (this.animation === undefined) {
            this.animation = defaultValue(kwargs.animation, null);
        }

        //! The actor performing the action, a LogicEntity. Not all actions might have actors,
        //! but most do. Set by the queue() in ActionSystem, this prevents users needing to write ME.queue( Action(ME) ) which
        //! is repetitive.
        this.actor = null;

        //! Whether multiple copies can be queued at the same type. Often this is impossible, but sometimes it is (e.g., drink potion x2)
        //! By default we allow this, actions that disallow it should change this.
        if (this.canMultiplyQueue === undefined) {
            this.canMultiplyQueue = defaultValue(kwargs.canMultiplyQueue, true);
        }

        //! Whether the action can be cancelled in the middle. Most can, but some can't, e.g., an action that shows a 'pain' animation
        //! after being hit. Pain would cancel all other actions and run until it is finished - nothing else can cancel it.
        //! Another example of a non-cancellable action is a cutscene, in which we load some noncancellable movements etc. for the PC
        //! to do. Another is when under control by a spell. Another is the refractory period after attacking/casting a spell/etc.
        if (this.canBeCancelled === undefined) {
            this.canBeCancelled = defaultValue(kwargs.canBeCancelled, true);
        }

        //! Another action that this one is in parallel to. If this is not null, then we mirror that other action's finish status, i.e.,
        //! we run as long as it does, and we finish when it does. Useful for animations that occur in parallel and so forth.
        if (this.parallelTo === undefined) {
            this.parallelTo = defaultValue(kwargs.parallelTo, null);
        }
    },

    //! Called once, when the action is about to start. At this stage, the this.actor exists, and can be utilized
    //! (it doesn't yet exist in the create, for notational convenience, see comment on this.actor).
    //! This calls doStart for actual added behaviour.
    start: function() {
        this.begun = true;
        this.doStart();
    },

    //! See start.
    doStart: function() {
    },

    //! Performs the action, called until the action finishes. Returns true when indeed finished. Calls
    //! doExecute() to do the actual specific execution details.
    //! Actions that have their own methods for deciding when they finish can override this and not
    //! call this parent, or adjust secondsLeft. Most don't override and just implement doExecute.
    //! @param seconds How many seconds to simulate as passing.
    //! @return Whether the action finished
    execute: function(seconds) {
        // If the actor to whom this action belongs has been deactivated, then finish this action
        if (this.actor !== null && this.actor.deactivated) {
            this.finish();
            return true;
        }

        if (!this.begun) {
            this.start();

            // Set the animation, if there is one, once as we begin this action
            if (this.animation !== null) {
                this.lastAnimation = this.actor.animation;
                if (this.actor.animation !== this.animation) {
                    this.actor.animation = this.animation; // Set only if changed, to save bandwidth
                }
            }
        }

        if (this.parallelTo === null) {
            log(INFO, "Executing action " + this._name);

            var finished = this.doExecute(seconds);
            eval(assert(" finished === true || finished === false ")); // doExecute must return a value, finished or not
            if (finished) {
                this.finish();
            }

            log(INFO, "                   ...finished: " + finished);

            return finished;
        } else {
            // We are parallel to some other action, so mirror its finish status.
            if (this.parallelTo.finished) {
                this.parallelTo = null; // Avoid memory leaks
                this.finish();
                return true;
            } else {
                return false;
            }
        }
    },

    //! Function to be overridden to perform the actual action-specific execution. Called by execute(). By default,
    //! this deducts time from the seconds left and returns whether there are any seconds remaining. Child classes
    //! can call this in the parent class in order to do that.
    //! @param seconds How many seconds to simulate as passing.
    //! @return Whether there are no seconds left, i.e., the action is finished
    doExecute: function(seconds) {
        this.secondsLeft -= seconds;
        return (this.secondsLeft <= 0);
    },

    //! DEPRECATED?
    //! The current frame in the current animation. Returns 0 (no animation to speak of) unless overridden.
    getAnimationFrame: function() {
        return 0;
    },

    //! Called when the action finishes, either normally or by being cancelled. Calls doFinish for user-defined
    //! behaviour.
    finish: function() {
        this.finished = true;

        // If this action has an animation, i.e. we changed the actor's animation, then restore the previous one
        // (typically ANIM_IDLE | ANIM_LOOP)
        // TODO: We only do this if the animation has not changed since we changed it earlier. If it has changed,
        // then we assume someone else knows what they are doing, and do not restore the old animation
        if (this.animation && this.lastAnimation !== undefined) {
            if (this.actor.animation !== this.lastAnimation) {
                this.actor.animation = this.lastAnimation; // Set only if changed, to save bandwidth
            }
        }

        this.doFinish();
    },

    //! Useful to override for additional cleanup/finalization. This is called by finish().
    doFinish: function() {
    },

    //! Cancel the action, if possible. Calls finished().
    cancel: function() {
        if (this.canBeCancelled) {
            this.finish();
        }
    }
});

//! A useful action that never ends. Make an action parallel to one of these one to keep it running forever (some other
//! mechanism would then be needed to make the action quit, by changing the 'parallelTo' attribute, calling finish()
//! directly, or some other way).
NeverEndingAction = Action.extend({
    _name: 'NeverEndingAction',

    //! Return false, to signal that the action is not finished
    doExecute: function(seconds) {
        return false;
    }
});


// Specific Action types


//! An action that has a LogicEntity as a target. Such actions inherit this class, and save a few lines of coding.
TargetedAction = Action.extend({
    _name: 'TargetedAction',

    //! @param target A LogicEntity that will be kept in this.target. Child classes can then use that value.
    create: function(target, kwargs) {
        this._super(kwargs);

        //! The target of the action
        this.target = target;
    }
});


//! An action that runs a single command, with parameters. This is useful as a way to queue such a command
//! for the next act() of an entity, e.g. removing it, as doing the removal immediately - inside some other script and/or
//! Sauerbraten C++ operation - might be problematic.
SingleCommandAction = Action.extend({
    _name: 'SingleCommandAction',

    //! @param command The command to be run
    create: function(command, kwargs) {
        this._super(kwargs);

        this.command = command;
    },

    //! Runs the command, and returns true, signalling that the action is finished
    doExecute: function(seconds) {
        this.command();
        return true;
    }
});


//
///
////
///
//


//! A queue of actions for a single LogicEntity and their management. Basically an interface to a queue of actions.
ActionSystem = Class.extend({
    create: function(parent) {
        //! The parent LogicEntity whose ActionSystem this is.
        this.parent = parent;

        //! The actual list of actions. In position 0 is the current one, and so forth.
        this.actionList = [];
    },

    //! @return Whether there are any actions in the system.
    isEmpty: function() {
        return (this.actionList.length === 0);
    },

    //! Run the current action and otherwise manage the list
    //! @param seconds How many seconds to simulate as passing.
    manageActions: function(seconds) {
        // Run current action, if any
        if (this.actionList.length > 0) {
            // Remove all finished actions - due to a clear() call that might have occurred
            this.actionList = filter(function(action) { return !action.finished; }, this.actionList);

            if (this.actionList.length > 0) {
                log(INFO, "Executing: %s" % (this.actionList[0]._name));
                if (this.actionList[0].execute(seconds)) { // If the action completed, remove it now - don't wait til next time
                                                           // (no special reason)
                    this.actionList.shift();
                }
            }
        }

        //!// TODO: for now, we don't move the remaining seconds to the next action.
        //!// If we have 30fps or so, then more than a single action/frame is quite unlikely. But, FIXME eventually. Do not forget
        //!// in this case to do a clear in between each action.
    },

    //! Tries to cancel all existing actions.
    clear: function() {
        // Note: Does not wipe them here (just calls cancel()), because the cause
        // for the clearing might bean action, i.e., one of them; and do not queue a wipe, because then we would wipe
        // actions added afterwards!
        forEach(this.actionList, function(action) {
            action.cancel();
        });
    },

    //! Queues a new action. Sets the actor of the action to the parent of this ActionSystem. Also stops
    //! the parent from sleeping; queuing an actions stops sleeping and lets the action be done as soon as possible.
    //! @param action The new action to be queued, at the end of the list.
    queue: function(action) {
        if (!action.canMultiplyQueue) {
            var multiple = false;

            forEach(this.actionList, function(existingAction) {
                if (existingAction._name === action._name) {
                    log(WARNING, format("Trying to multiply queue {0}, but that isn't allowed\r\n", action._name));
                    multiple = true;
                    return;
                }
            });

            if (multiple) {
                return;
            }
        }

        this.actionList.push(action);
        action.actor = this.parent; // Set the actor, our parent. A notational convenience, see Action() for the reason
    },

    //! DEPRECATED?
    //! @return The current animation frame of the current action. Returns 0 if there isn't an action to ask.
    getAnimationFrame: function() {
        if (this.actionList.length === 0) {
            return 0;
        } else {
            return this.actionList[0].getAnimationFrame();
        }
    }
});

