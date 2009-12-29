
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


//! Generation and handling of doors, a very specific and useful RPG-related thing.

//! Doors will work as follows:
//!
//!      Doors are, for starters, axis-aligned to 90 degrees. We will
//!      use the existing bounding box method on them, which will prevent
//!      people from walking through closed doors.
//!
//!      Once opened, the door will be at 135 degrees or more, and consider
//!      using an ellipsoid collision space for it in this state. Basically
//!      it should be close enough to flat against the nearby wall that it
//!      should not be an issue.
//!
//!      Consider letting doors open only up to 90 degrees in some cases,
//!      which is important if there is a wall blocking the way. But there
//!      might also be a character, furniture, etc., so perhaps check this
//!      at runtime.
//!
//!      Note that rotation is around the object center, so the center
//!      should be at (0,0) and the door extend to (0,1), and so forth. I.e.,
//!      the center is at the edge. The bounding box system works with this,
//!      unless I am mistaken.
//!
//!      To open doors, we will try to use the built-in collision system:
//!      rotate the door small steps and see if that would work. If the
//!      entire path is not free, say "door cannot be opened!", perhaps show
//!      a little 'shaking animation' (not enough to show the area behind
//!      the door! Less shaking than the door's depth). Note that this is
//!      somewhat conservative, i.e., you cannot open a door even if someone
//!      is close to preventing you, which is probably a good idea actually.
//!
//!      We might want to control the bounding box manually, at least during
//!      rotation - extending the box sufficiently to prevent people from
//!      entering the area in which the door is moving while it is opening.
//!      That is, after a door has been agreed-upon to open, it will open,
//!      that is unstoppable. Note that an elliptic bounding box might work
//!      here.
//!
//!      We can also try to be fancy and allow stopping the door in the middle
//!      - move the door step by step until it is stopped. Then it might be
//!      open 'just enough' to let you slip through, for example, if your
//!      character is not too fat. But, let us leave this for now.
//!
//!      The rotation itself will be done in software, i.e., we do not
//!      want to make special models for this. We will generate the relevant
//!      interpolated rotated positions manually using simple trigonometry.
//!
//!      We will therefore create doors as a subclass of vertexmodel, which
//!      is an animated model built with vertexes.
//!
//!      Learn from md3.h how to make a derivative of vertmodel. Actually,
//!      perhaps we should subclass md3model, so we can load a static md3
//!      model (with all the right textures, bumpmaps, etc.) and just
//!      programmatically open and close it. That seems best.
//!
//!      Make the swinging open start slow, then fast, then slow - like a real
//!      door swing. That + nice sound will make it passable.
//!
//!      We can use mapmodel behavior 7, toggling between two states. However
//!      we need to have collision detection on, unlike switches.
//!
//!      Note that doors shouldn't cast shadows, like all other moving things.
//!      Somewhat ugly, but ignorable for now.
//!
//!      Let's add a door-swinging animation for any mapmodel starting with 'door'.
//!
//! DEBUGGING: use these creation commands:
//!      Door:   /newent mapmodel 0 11 1
//!      Switch: /newent mapmodel 2 7 1

struct DoorManager
{
    //! Number of frames for the door animation. Currently the same for all doors (30)
    static int         numFrames;

    //! The sauer .cfg file command that generates the animation. Depends on the # of frames
    static const char* animationCommand;

    //! Decide whether a filename of a model represents a door. We test this way, as opposed
    //! to inside the .cfg, because the .cfg has not yet been loaded fully.
    static inline bool isDoor(const char *filename)
    {
        #define DOOR_FULL_HEADER  "packages/models/doors"
        #define DOOR_SHORT_HEADER "doors/"
        return ( strncmp(filename, DOOR_FULL_HEADER,  strlen(DOOR_FULL_HEADER))  == 0  ||
                 strncmp(filename, DOOR_SHORT_HEADER, strlen(DOOR_SHORT_HEADER)) == 0 );
    }

    //! Alters the initial frame so that the origin is appropriate
    //! for rotating. I.e., when we generate the next frames we will
    //! get appropriate door rotation, around the axis. We assume
    //! the model is created with an axis at the far 'left'. TODO: Generalize this.
    static void modifyInitialFrame(vert* firstVert, int numVerts);

    //! Generate a single frame in the door's animation.
    static void generateDoorAnimationFrame(int   i         /*!< index of frame to generate                              */ ,
                                           vert* firstVert /*!< pointer to first vertex in first frame (the base frame) */ ,
                                           vert* currVert  /*!< pointer to first vertex in current frame to generate    */ ,
                                           int   numVerts  /*!< num vertexes per frame                                  */ );
};

