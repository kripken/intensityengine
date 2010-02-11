
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

typedef int physicsHandle;

// An abstract interface to a physics engine. Specific physics engines implement
// this interface.
class PhysicsEngine
{
public:
    virtual void init() = 0;

    //! Erase all contents
    virtual void destroy() = 0;

    virtual void clearStaticPolygons() = 0;

    // Whether we need static polygon info
    virtual bool requiresStaticPolygons() = 0;

    //! Add a polygon to be collided against, that is treated as completely fixed - static geometry
    virtual void addStaticPolygon(std::vector<vec> vertexes) = 0;

    //! Add a dynamic element, something that can move in the world
    virtual physicsHandle addDynamic(float mass, float radius) = 0;

    //!
    virtual void removeDynamic(physicsHandle handle) = 0;

    //! Sets a dynamic's properties. Called to get changes due to scripting or position updates, etc.
    virtual void setDynamicPosition(physicsHandle handle, const vec& position) = 0;
    virtual void setDynamicVelocity(physicsHandle handle, const vec& velocity) = 0;

    //! Outputs a dynamic's properties. Called to get information from the physics engine outside into the rest of the engine
    virtual void getDynamic(physicsHandle handle, vec& position, vec& velocity) = 0;

    virtual void simulate(float seconds) = 0;
};

namespace PhysicsManager
{
    //! Initialize physics engine
    extern void createEngine();

    extern void destroyEngine();

    extern bool hasEngine();

    PhysicsEngine* getEngine();

    //! Erases the contents of the physics engine
    extern void clearWorldGeometry();

    //! Process a vector of vertexes, using sauer coordinates
    extern void setupWorldGeometryVerts(vector<vertex>& verts);

    //! Sets up a submesh for a group of tris, all using the same material
    extern void setupWorldGeometryTriGroup(usvector& data, int tex, int lightmapTex, int orientation);

    //! After presenting vertexes and a series of trigroups using those vertexes, create a mesh, if necessary
    extern void finishWorldGeometryVerts();

    //! Given all the vertex info fed since the last clearWorldGeometry(), build it and send to the physics engine
    extern void finalizeWorldGeometry();

    extern void simulate(float seconds);

    //! Adds a character. The character will continue to be tracked until removed, i.e.,
    //! (1) simulation will notice changes in the character's position and velocity, which
    //! are assumed to be from scripting/position updates, and
    //! (2) simulation will store changes in the entity, so that they are seen
    extern void addWatchedDynamic(int uniqueId);

    extern void removeWatchedDynamic(int uniqueId);
};

