
// Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
// This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

Testing.replaceEnvironment();

// Tests

var parent = {};

parent.actionSystem = new ActionSystem(parent);
parent.setSleep = function(seconds) { };

eval(assert(" parent.actionSystem.isEmpty() "));

var feedback = 0;

var TestAction = Action.extend({
    secondsLeft: 6,
    doExecute: function(seconds) {
        feedback += seconds;
        return this._super(seconds);
    }
});

var act = new TestAction();

eval(assert(" !act.begun "));
eval(assert(" !act.finished "));
eval(assert(" act.startTime === 17001 "));
eval(assert(" act.secondsLeft === 6 "));

parent.actionSystem.queue(act);

eval(assert(" !parent.actionSystem.isEmpty() "));

parent.actionSystem.manageActions(2);

eval(assert(" act.begun "));
eval(assert(" !act.finished "));
eval(assert(" act.secondsLeft === 4 "));

eval(assert(" feedback === 2 "));
eval(assert(" !parent.actionSystem.isEmpty() "));

var act2 = new Action( { secondsLeft: 1000 } );

parent.actionSystem.queue(act2);

parent.actionSystem.manageActions(3);

eval(assert(" act.begun "));
eval(assert(" !act.finished "));
eval(assert(" act.secondsLeft === 1 "));
eval(assert(" feedback === 5 "));
eval(assert(" !parent.actionSystem.isEmpty() "));

parent.actionSystem.manageActions(3);

eval(assert(" act.begun "));
eval(assert(" act.finished "));
eval(assert(" feedback >= 6 "));
eval(assert(" !parent.actionSystem.isEmpty() "));

eval(assert(" !act2.finished "));

parent.actionSystem.manageActions(3000);

eval(assert(" act2.finished "));
eval(assert(" parent.actionSystem.isEmpty() "));

// Cleanup

Testing.restoreEnvironment();

