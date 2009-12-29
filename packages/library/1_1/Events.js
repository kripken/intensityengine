
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


//! An action that can queue more actions on itself, which run on its actor.
//! Finishes when both this action itself is over, and when all subactions are done.
ContainerAction = Action.extend({
    _name: 'ContainerAction',

    create: function(otherActions, kwargs) {
        this._super(kwargs);

        this.otherActions = otherActions;
    },

    doStart: function() {
        this.actionSystem = new ActionSystem(this.actor); // actor is now known

        forEach(this.otherActions, function(otherAction) {
            this.actionSystem.queue(otherAction);
        }, this);
    },

    doExecute: function(seconds) {
        this.actionSystem.manageActions(seconds);
        return this._super(seconds) && this.actionSystem.isEmpty();
    },
});

//! Like ContainerAction, but runs actions in parallel. Finishes when all are done.
ParallelAction = Action.extend({
    _name: 'ParallelAction',

    canBeCancelled: false, // Simple assumption for now, TODO: Generalize

    create: function(otherActions, kwargs) {
        this._super(kwargs);

        this.otherActions = otherActions;
    },

    doStart: function() {
        this.actionSystems = []

        forEach(this.otherActions, function(otherAction) {
            var actionSystem = new ActionSystem(this.actor);
            actionSystem.queue(otherAction);
            this.actionSystems.push(actionSystem);
        }, this);
    },

    doExecute: function(seconds) {
        this.actionSystems = filter(function(actionSystem) {
            actionSystem.manageActions(seconds);
            return !actionSystem.isEmpty();
        }, this.actionSystems);
        return this._super(seconds) && this.actionSystems.length == 0;
    },

    doFinish: function(seconds) {
        forEach(this.actionSystems, function(actionSystem) {
            actionSystem.clear();
        });
    },
});

DelayedAction = Action.extend({
    _name: 'DelayedAction',

    //! @param command The command to be run
    create: function(command, kwargs) {
        this._super(kwargs);

        this.command = command;
    },

    //! Runs the command, and returns true, signalling that the action is finished
    doExecute: function(seconds) {
        if (this._super(seconds)) {
            this.command();
            return true;
        } else {
            return false;
        }
    },
});


