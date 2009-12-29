
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

