
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

    virtual void clearStaticGeometry() { };

    // Whether we need static polygon info
    virtual bool requiresStaticPolygons() { return false; };

    //! Add a polygon to be collided against, that is treated as completely fixed - static geometry
    virtual void addStaticPolygon(std::vector<vec> vertexes) { };

    virtual bool requiresStaticCubes() { return false; };
    //! Provides the info for a static cube
    virtual void addStaticCube(vec o, vec r) { };
    virtual void addStaticConvex(std::vector<vec>& vecs) { };

    virtual physicsHandle addSphere(float mass, float radius) = 0;
    virtual physicsHandle addBox(float mass, float rx, float ry, float rz) = 0;

    //!
    virtual void removeBody(physicsHandle handle) = 0;

    //! Sets a body's properties. Called to get changes due to scripting or position updates, etc.
    virtual void setBodyPosition(physicsHandle handle, const vec& position) = 0;
    virtual void setBodyVelocity(physicsHandle handle, const vec& velocity) = 0;

    //! Outputs a body's properties. Called to get information from the physics engine outside into the rest of the engine
    virtual void getBodyPosition(physicsHandle handle, vec& position) = 0;
    virtual void getBodyRotation(physicsHandle handle, quat& rotation) = 0;
    virtual void getBodyVelocity(physicsHandle handle, vec& velocity) = 0;

    virtual void simulate(float seconds) = 0;
};

namespace PhysicsManager
{
    //! Initialize physics engine
    extern void createEngine(std::string type="sauer");

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
};

