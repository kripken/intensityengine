
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

#include "engine.h"

#include "utility.h"
#include "script_engine_manager.h"

#include "intensity_physics.h"
#include "intensity_physics_realistic.h"

#include "intensity_physics_sauer.h" // Just for line with creation - nothing else
#ifdef INTENSITY_BULLET
    #include "intensity_physics_bullet.h" // Just for line with creation - nothing else
#endif

namespace PhysicsManager
{

PhysicsEngine *engine = NULL;

//

void createEngine()
{
    if (!ScriptEngineManager::hasEngine()) return;

    std::string type = "sauer";

    if (ScriptEngineManager::getGlobal()->getProperty("Global")->hasProperty("physicsEngineType"))
        type = ScriptEngineManager::getGlobal()->getProperty("Global")->getProperty("physicsEngineType")->getString();

    Logging::log(Logging::WARNING, "Physics engine: %s\r\n", type.c_str());

    if (type == "sauer")
    {
        Logging::log(Logging::DEBUG, "Using sauer physics engine\r\n");
        engine = new SauerPhysicsEngine();
    }
#ifdef INTENSITY_BULLET
    else if (type == "bullet")
    {
        Logging::log(Logging::DEBUG, "Using bullet physics engine\r\n");
        engine = new BulletPhysicsEngine();
    }
#endif
    else
    {
        Logging::log(Logging::ERROR, "Invalid physics engine: %s\r\n", type.c_str());
        assert(0);
    }

    engine->init();
}

void destroyEngine()
{
    if (engine != NULL)
    {
        engine->destroy();
        delete engine;
        engine = NULL;
    }
}

bool hasEngine()
{
    return (engine != NULL);
}

PhysicsEngine* getEngine()
{
    assert(engine);
    return engine;
}

#define REQUIRE_ENGINE if (!hasEngine()) return; // Sauer calls physics before the first map even loads

void clearWorldGeometry()
{
    REQUIRE_ENGINE

    engine->clearStaticGeometry();
}

vec* currVecs = NULL;

void setupWorldGeometryVerts(vector<vertex>& verts)
{
    REQUIRE_ENGINE

    if (!engine->requiresStaticPolygons()) return;

    renderprogress(0, "generating physics vertexes");

    unsigned int nVertices = verts.length();
    currVecs = new vec[nVertices];

    for (unsigned int i = 0; i < nVertices; i++)
    {
        currVecs[i] = verts[i].tovec();
    }
}


void setupWorldGeometryTriGroup(usvector& data, int tex, int lightmapTex, int orientation)
{
    REQUIRE_ENGINE

    if (!engine->requiresStaticPolygons()) return;

    static int counter = 0;
    renderprogress(-float(counter)/100, "generating physics simulation verts");
    counter++;
    if (counter == 100) counter = 0;

    // Count indexes (three indexes to a triangle)
    unsigned int ibufCount = data.length();
    if (ibufCount == 0)
        return;
    assert(ibufCount % 3 == 0);
    unsigned int numTris = ibufCount/3;

    Logging::log(Logging::DEBUG, "IO: setupWorldGeometryTriGroup: %d\r\n", ibufCount);

    std::vector<vec> currPolygon;
    int base;
    for (unsigned int i = 0; i < numTris; i++)
    {
        base = i*3;
        currPolygon.clear();
        currPolygon.push_back( currVecs[data[base+0]] );
        currPolygon.push_back( currVecs[data[base+1]] );
        currPolygon.push_back( currVecs[data[base+2]] );
        engine->addStaticPolygon(currPolygon);
    }
}


void finishWorldGeometryVerts()
{
    REQUIRE_ENGINE

    delete[] currVecs;
    currVecs = NULL;
}

// Cube world processing utilities

    void loopOctree(cube* c, int size, ivec o);

    void processOctanode(cube* c, int size, ivec o)
    {
        if (!c->children)
        {
            Logging::log(Logging::WARNING, "processOctanode: %4d,%4d,%4d : %4d,%4d,%4d   (%.8x,%.8x,%.8x)\r\n", o.x, o.y, o.z, o.x+size, o.y+size, o.z+size, c->faces[0], c->faces[1], c->faces[2]);
            if (isentirelysolid(*c))
                engine->addStaticCube(vec(o.x+size/2, o.y+size/2, o.z+size/2), vec(size/2));
            else if (!isempty(*c))
            {
                // Not fully solid, create convex shape with the verts
                // TODO: Optimize, use addStaticCube when rectangular
                printf("Not fully solid nor empty\r\n");
                vvec vv[8];
                bool usefaces[8];
                int vertused = calcverts(*c, o.x, o.y, o.z, size, vv, usefaces);
                std::vector<vec> vecs;
                loopi(8) if(vertused&(1<<i))
                {
                    vec t = vv[i].tovec(o);
                    printf("vv: %f,%f,%f\r\n", t.x, t.y, t.z);
                    vecs.push_back(t);
                }
                assert(vecs.size() > 0);

                // Test for simple rectangular objects, which we sent as Cubes, not Convexes
                std::set<int> dimensionValues[3];
                int dimensionMins[3], dimensionMaxes[3];
                for (unsigned int j = 0; j < vecs.size(); j++)
                {
                    for (int k = 0; k < 3; k++)
                    {
                        dimensionValues[k].insert(vecs[j][k]);
                        if (j > 0)
                        {
                            dimensionMins[k] = min(dimensionMins[k], int(vecs[j][k]));
                            dimensionMaxes[k] = max(dimensionMaxes[k], int(vecs[j][k]));
                        } else {
                            dimensionMins[k] = vecs[j][k];
                            dimensionMaxes[k] = vecs[j][k];
                        }
                    }
                }
                for (int k = 0; k < 3; k++)
                    if (dimensionValues[k].size() > 2)
                    {
                        Logging::log(Logging::WARNING, "Adding as Convex\r\n");
                        engine->addStaticConvex(vecs);
                        return;
                    }
                Logging::log(Logging::WARNING, "Adding as Cube\r\n");
                engine->addStaticCube(vec(
                    (dimensionMins[0]+dimensionMaxes[0])/2,
                    (dimensionMins[1]+dimensionMaxes[1])/2,
                    (dimensionMins[2]+dimensionMaxes[2])/2
                ), vec(
                    (dimensionMaxes[0]-dimensionMins[0])/2,
                    (dimensionMaxes[1]-dimensionMins[1])/2,
                    (dimensionMaxes[2]-dimensionMins[2])/2
                ));
            }
        } else {
            loopOctree(c->children, size, o);
        }
    }

    void loopOctree(cube* c, int size, ivec o)
    {
        for (int z = 0; z <= 1; z++)
            for (int y = 0; y <= 1; y++)
                for (int x = 0; x <= 1; x++)
                    processOctanode(&c[x+y*2+z*4], size/2, ivec(o.x + x*size/2, o.y + y*size/2, o.z + z*size/2));
    }

void finalizeWorldGeometry()
{
    REQUIRE_ENGINE

    if (engine->requiresStaticCubes())
    {
        renderprogress(0, "generating physics geometries");

        // Loop the octree and provide the physics engine with the cube info
        loopOctree(worldroot, worldsize, vec(0));
    }
}


// Simulation

void simulate(float seconds)
{
    REQUIRE_ENGINE

    engine->simulate(seconds);
}

};

