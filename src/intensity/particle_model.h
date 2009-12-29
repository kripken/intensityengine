
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


//! Particle Model: An animated model for the Sauer model rendering system that is really
//! some particle effects. The main usage is as an attachment, thus allowing to
//! attach particle effects to characters (e.g., have a flaming sword, smoking
//! torch, etc.).

struct particle_model : animmodel
{
    particle_model(const char *name) : animmodel(name) {};

    int type() const { return MDL_PARTICLE; };

    bool load()
    {
        // In the future, allow all sorts of particles, with parameters, multiple locations (say, all along the sword)
        // for now, just some sparklies
        Logging::log(Logging::DEBUG, "Trying to load sparkly %s\r\n", loadname);

        parts.add(new part);
        parts[0]->model = this;
        parts[0]->index = 0; // ??? TODO: figure out if relevant

        loaded = true;
        return (strcmp(loadname, "particle/sparkly") == 0);
    }

    //! Render the particles, using a matrix that tells us where we, the attachment, should be rendered
    //! The matrix includes translation to the location of the main model, and then a transformation
    //! - both translation and rotation, I believe - for the attachment itself relative to the model
    //! Kludge: we use the virtual render() and pass a GLfloat * in the dynent. Otherwise, we need to
    //! rework the sauer rendermodel system...
    void render(int anim, float speed, int basetime, const vec &o, float yaw, float pitch, dynent *d, LogicEntityPtr entity, modelattach *a = NULL, const vec &color = vec(0, 0, 0), const vec &dir = vec(0, 0, 0))
    {
        GLfloat* attachmentMatrix = (GLfloat*)d; // A horrible kludge, but necessary, see comment in .h

        // In the future, allow all sorts of particles, with parameters, multiple locations (say, all along the sword)
        // for now, just some sparklies
        vec location(-attachmentMatrix[12],
                     -attachmentMatrix[13],
                     -attachmentMatrix[14]);

        particle_splash(0, 50, 100, location);
    }

    //! We need to override the pure virtual calcbb function; we simpy return (0,0,0) , (0,0,0) - should work, but needs testing
    void calcbb(int frame, vec &center, vec &radius)
    {
        center = vec(0,0,0);
        radius = vec(0,0,0); // FIXME: Is this bad, perhaps we should patch so that radius -1^3 is indeed no expansion for the bb.
    }
};

