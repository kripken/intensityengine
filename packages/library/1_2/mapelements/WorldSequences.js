
// Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
// This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

//! Lets you define areas that form 'sequences', so you can track the progression of a player through them.
WorldSequences = {
    plugins: {
        areaTrigger: {
            //! Identifier of the sequence this is a part of
            sequenceId: new StateString(),

            //! The place in the sequence. 0,1,2,3,...
            //! Note that you can have several areas with the same number. The player must then pass at least one.
            //! That is, the default is 'OR'. If you want all the areas with the same number to be passed, in
            //! whatever order the player does it - 'AND' - then set sequenceIsMandatory to true on those areas.
            sequenceNumber: new StateInteger(),

            //! See comment in sequenceNumber.
            sequenceIsMandatory: new StateBoolean(),

            init: function() {
                this.sequenceId = '';
                this.sequenceNumber = 0;
                this.sequenceForceAllPrevious = false;
            },

            clientOnCollision: function(collider) {
                if (collider !== getPlayerEntity()) return;

                if (collider.worldSequences[this.sequenceId] === this.sequenceNumber - 1) {
                    this.sequenceIsMandatoryPassed = true;
                    // Check all mandatories were passed, that have the same number as this
                    var that = this;
                    if (filter(function (entity) {
                            return entity.sequenceId === that.sequenceId &&
                                   entity.sequenceNumber === that.sequenceNumber &&
                                   entity.sequenceIsMandatory &&
                                   !entity.sequenceIsMandatoryPassed;
                        }, getEntitiesByClass(AreaTrigger)).length > 0) return;

                    // Reset all mandatory checks (including of this entity)
                    forEach(filter(function (entity) {
                        return entity.sequenceId === that.sequenceId &&
                                entity.sequenceNumber === that.sequenceNumber;
                    }, getEntitiesByClass(AreaTrigger)), function (entity) {
                        entity.sequenceIsMandatoryPassed = false;
                    });

                    // Arrive
                    collider.worldSequences[this.sequenceId] += 1; // Bump to our value. Means more collisions right now won't matter.
                    this.onSequenceArrival(collider);
//                    log(ERROR, "AT:" + this.sequenceNumber);
                }
            },

            //! Override with custom code, if desired
            onSequenceArrival: function(entity) {
            },
        },

        player: {
            clientActivate: function() {
                this.worldSequences = {};
            },

            //! MUST be called for all sequences relevant for the player. The player can then enter the '0' for that sequence.
            resetWorldSequence: function(sequenceId) {
                this.worldSequences[sequenceId] = -1;
            },
        },
    },
};

