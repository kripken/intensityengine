
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


// Model rendering parameters. Synch with iengine.h
MODEL = {
    CULL_VFC: 1<<0,
    CULL_DIST: 1<<1,
    CULL_OCCLUDED: 1<<2,
    CULL_QUERY: 1<<3,
    SHADOW: 1<<4,
    DYNSHADOW: 1<<5,
    LIGHT: 1<<6,
    DYNLIGHT: 1<<7,
    FULLBRIGHT: 1<<8,
    NORENDER: 1<<9
};

//! A logic entity that supports animation
AnimatableLogicEntity = LogicEntity.extend({
    _class: "AnimatableLogicEntity",

    //! The animation currently shown for this entity. Setting this changes the start_time
    //! value
    animation: new WrappedCInteger({ cSetter: 'CAPI.setAnimation', clientSet: true }),

    //! Read-only value for getting the starttime from C
    startTime: new WrappedCInteger({ cGetter: 'CAPI.getStartTime' }),

    //! Filename of the model used for this character. This is a path under packages/models.
    modelName: new WrappedCString({ cSetter: 'CAPI.setModelName' }),

    //! List of attachments for the character, shown in addition to the model (armor, weapons, etc. - variable stuff).
    attachments: new WrappedCArray({ cSetter: 'CAPI.setAttachments' }),

    init: function(uniqueId, kwargs) {
        this._super(uniqueId, kwargs);

//        log(DEBUG, "Animatable.init " + uniqueId);

        //! Dictionary form of attachments, for convenience. We generate the StateVariable 'attachments' from this
        this._attachmentsDict = {};

        this.modelName = "";
        this.attachments = [];
        this.animation = ANIM_IDLE | ANIM_LOOP;

//        log(DEBUG, "Animatable.init done");
    },

    activate: function(kwargs) {
        log(DEBUG, "Animatable.activate:");

        this._super(kwargs);

        log(DEBUG, "Animatable.activate (2)");

        try {
            // XXX Workaround: We need to delay doing addentity/removeentity until this late stage, otherwise
            // minions can walk through barriers in tideturner. This works for now, as setting modelName
            // causes addentity/removeentity
            this.modelName = this.modelName;
        } catch (e) {
            // Do nothing - if no model name, then no physical presence (e.g., just a Light), so no worries
        }

        log(DEBUG, "Animatable.activate complete");
    },

    //! Sets an attachment at a certain position.
    //! @param tag Designates a position on a model. See examples (e.g., storming_the_castle.py)
    //! @param model_name The model to place at that tag. Set to None to remove an attachment at that tag.
    setAttachment: function(tag, modelName) {
        if (!modelName) {
            if (this._attachmentsDict[tag] !== undefined) {
                delete this._attachmentsDict[tag];
            }
        } else {
            this._attachmentsDict[tag] = modelName;
        }

        // Create an appropriate attachments vector, this is what is sent over the wire and so forth
        // We do this to a temporary vector, so as not to have many updates over the wire.
        // Yes, this is not fast, but changes to attachments should not be a common occurrence.
        var result = [];
        forEach(items(this._attachmentsDict), function(pair) {
            result.push( modelAttachment(pair[0], pair[1]) );
        });
        this.attachments = result; // Triggers a send over the wire
    },

    //! DEPRECATED?
    //! Gets the current animation frame of the LE, depending on the current animation/action, as appropriate.
    getAnimationFrame: function() {
        return this.actionSystem.getAnimationFrame();
    },

    //! Sets the animation value *locally*, i.e., with an effect only this client can see. Does not synch.
    setLocalAnimation: function(animation) {
        CAPI.setAnimation(this, animation);
        this.stateVariableValues['animation'] = animation; // Store value so that readings of this.animation return the value
    },
});


// An action that sets a *local* animation (see setLocalAnimation in AnimatableLogicEntity)
// Note that we use localAnimation and not animation, which already exists in Action -
// if we used the same one, then it would *also* send updates.
LocalAnimationAction = Action.extend({
    _name: 'LocalAnimationAction',

    doStart: function() {
        this.oldAnimation = this.actor.animation;
        this.actor.setLocalAnimation(this.localAnimation);
    },

    doFinish: function() {
        // Return to original animation, unless it was changed meanwhile - then do nothing
        if (this.actor.animation === this.localAnimation) {
            this.actor.animation = this.oldAnimation;
        }
    }
});

