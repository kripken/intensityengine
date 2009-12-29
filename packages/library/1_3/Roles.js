
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


// Roles are Classes that players can take in a game. We call them Roles in
// the code so as to not confuse with entity classes and so forth.

Roles = {
    plugin: {
        role: new StateString(),

        clientActivate: function() {
            this.connect('clientRespawn', function() {
                if (this !== getPlayerEntity() || this.team === 'spectators') return;
                this.queueAction(new Roles.SelectAction());
            });

            this.connect('client_onModify_role', function(role) {
                if (this !== getPlayerEntity()) return;
                if (role) {
                    Sound.play('gk/imp_01', this.position.copy());
                }
            });
        },

        clientAct: function() {
            if (this === getPlayerEntity() && this.role) {
                CAPI.showHUDText(this.role, 0.5, 0.93, 0.5, 0xCCFFBB);
            }
        },
    },

    getPossible: null, //!< Override

    //! Lets the user select their class. Captures mouse clicks until finished. TODO: Refactor out mouse click stuff
    SelectAction: Action.extend({
        _name: 'SelectRoleAction',

        canBeCancelled: false,
        canMultiplyQueue: false,

        doStart: function() {
            this.savedClickHandler = ApplicationManager.instance.clientClick;
            ApplicationManager.instance.clientClick = bind(this.clientClick, this);

            this.selection = -1;

            this.actor.role = '';

            this.possibleRoles = Roles.getPossible(this.actor);

            var numRoles = this.possibleRoles.length;
//            this.actor.yaw = 180/numRoles; // Face in between two roles

            this.totalMove = 0;
        },

        doExecute: function(seconds) {
            if (!this.actor.lastRole && this.actor.canMove) { // Freeze if no last role
                this.actor.canMove = false;
            }

            // If moving, check if moved enough to cancel selection and use last role
            if (this.actor.lastRole && (this.actor.move !== 0 || this.actor.strafe !== 0)) {
                this.totalMove += seconds;

                if (this.totalMove > 0.25) {
                    this.useLastRole = true;
                }
            }

            CAPI.showHUDText('<-- Turn to scroll, click to select your class -->', 0.5, 0.2, 0.5, 0xDDCCBB);

            if (this.actor.lastRole) {
                CAPI.showHUDText('(move away to repeat your last role: ' + this.actor.lastRole + ')', 0.5, 0.6, 0.5, 0xDDCCBB);
            }

            var numRoles = this.possibleRoles.length;
            var yaw = normalizeAngle(-this.actor.yaw, 180);
            var currentPosition = yaw/(360/numRoles);
            if (currentPosition > numRoles-0.5) {
                currentPosition -= numRoles;
            }

            this.currentRole = clamp(Math.round(currentPosition), 0, numRoles-1);

            for (var i = 0; i < numRoles; i++) {
                var diff = currentPosition - i;
                var diff2 = currentPosition + numRoles - i;
                if (Math.abs(diff2) < Math.abs(diff)) {
                    diff = diff2;
                }
                diff2 = currentPosition - numRoles - i;
                if (Math.abs(diff2) < Math.abs(diff)) {
                    diff = diff2;
                }

                if (Math.abs(diff) <= 1) {
                    var size = 1-Math.abs(diff);
//                    size = size*size;
                    CAPI.showHUDText(this.possibleRoles[i], 0.5 + diff/2, 0.4, size*1.5, 0xDDCCD0);
                }
            }

            if (this.selection >= 0) {
                this.actor.role = this.possibleRoles[this.selection];
                this.actor.lastRole = this.possibleRoles[this.selection];
                return true;
            } else if (this.useLastRole) {
                this.actor.role = this.actor.lastRole;
                return true;
            } else {
                return false;
            }
        },

        doFinish: function() {
            ApplicationManager.instance.clientClick = this.savedClickHandler;

            this.actor.canMove = true;
        },

        clientClick: function(button, down, position, entity) {
            if (button === 1 && down) {
                this.selection = this.currentRole;
            }
        },
    }),
};

