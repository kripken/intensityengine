
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

#ifdef WIN32
    #include <math.h>
#endif

#include "cube.h"
#include "engine.h"
#include "game.h"

#include "message_system.h"
#include "editing_system.h"
#include "targeting.h"
#include "client_system.h"
#include "utility.h"
#include "script_engine_manager.h"



// Kripken:
// sel.corner: The face corner the mouse pointer is closest to.
//             For a face from above, 0 is to the origin, and 2 is across from 1 (so, fill one row, then fill
//             the next row, starting from the same side)
// cx, cxs, cy, cys: Seems related to the  difference between selections and cursor hoverings.
//                   Some examples values: 0,2,0,2 for one cube, 0,2,0,4 for 1x2 and 0,4,0,4 for 2x2.
// o: Current cube origin (if mapsize is 1024, then stuff like 0,0,512
// s: Selection size, in cubes. So 1,1,1 is exactly one cube, of the current gridsize
// grid: gridsize. So if mapsize is 1024, and we are on the largest cube size (1/8 of the map), we have 512
// orient: orientation of the face. Values: if 0,0,0 is in the lower left, and the cube extends away from us
//         in the other axis, then:
//              0: facing us (negative X)
//              1: away from us (positive X)
//              2: left (negative Y)
//              3: right (positive Y)
//              4: down (negative Z)
//              5: up (positive Z)
//      (note that orientation/2 is a dimension, one of x,y,z)
// dir: -1 seems to be extrude, +1 to push back into. For pushing edges, 1 is into the cube, -1 is back out
// mode: 1 == extrude/push back cube(s), 2 == push a corner
// local: whether initiated here (if so, notify others)
/////////////////////void mpeditface(int dir, int mode, selinfo &sel, bool local)



// 1.0 is to place it exactly on worldpos
#define FAR_PLACING_FACTOR 0.9

namespace EditingSystem
{
    // Globals

    bool madeChanges = false;

#ifdef CLIENT
    // Saved mouse position
    int savedMousePosTime = -1;
    vec savedMousePos;

    void save_mouse_pos()
    {
        savedMousePosTime = Utility::SystemInfo::currTime();
        savedMousePos = TargetingControl::worldPosition;
        Logging::log(Logging::DEBUG, "Saved mouse pos: %f,%f,%f (%d)\r\n", savedMousePos.x, savedMousePos.y, savedMousePos.z,
                                                                           savedMousePosTime);
    }

    COMMAND(save_mouse_pos, "");
#endif


std::vector<std::string> entityClasses;

void prepareentityclasses()
{
    entityClasses.clear();

    ScriptValuePtr classes = ScriptEngineManager::runScript("listEntityClasses()");

    for (int i = 0; i < classes->getProperty("length")->getInt(); i++)
    {
        entityClasses.push_back(classes->getProperty( Utility::toString(i) )->getString());
    }
}

COMMAND(prepareentityclasses, "");

ICOMMAND(numentityclasses, "", (), { intret(entityClasses.size()); });

bool validateEntityClass(std::string _class)
{
    prepareentityclasses();

    return std::find(entityClasses.begin(), entityClasses.end(), _class) != entityClasses.end();
}

void newEntity(std::string _class, std::string stateData)
{
    // TODO: Place at worldpos instead? But player->o is what sauer does, seemingly in the code
    #ifdef CLIENT
        vec farPosition;

        Logging::log(Logging::DEBUG, "Considering saved mouse pos: %f,%f,%f (%d, %d)\r\n", savedMousePos.x, savedMousePos.y, savedMousePos.z,
                                                                           savedMousePosTime, Utility::SystemInfo::currTime());

        // Use saved position, if exists and saved recently
//        if (savedMousePosTime != -1 && Utility::SystemInfo::currTime() - savedMousePosTime < 15000)
//        {
            farPosition = savedMousePos;
            savedMousePosTime = -1;
//        } else
//            farPosition = TargetingControl::worldPosition;

        farPosition.mul(FAR_PLACING_FACTOR);
        vec closePosition = ClientSystem::playerLogicEntity->getOrigin();
        closePosition.mul(1 - FAR_PLACING_FACTOR);
        closePosition.add(farPosition);

        if (stateData == "") stateData = "{}";

        MessageSystem::send_NewEntityRequest(_class,
                                             closePosition.x,
                                             closePosition.y,
                                             closePosition.z,
                                             stateData);
    #else // SERVER
        assert(0); // Where?
    #endif
}

void newentnoparams(char* _class)
{
    newEntity(_class);
}
COMMAND(newentnoparams, "s");


//----------------

int getWorldSize()
{
    return getworldsize();
}

void eraseGeometry()
{
    // Clear out map
    int halfSize = getworldsize()/2;
    loopi(2) loopj(2) loopk(2)
        deleteCube(i*halfSize, j*halfSize, k*halfSize, halfSize);
}

// Ensure that cube coordinates are valid. (x,y,z) can be any values in 0..getworldsize, and gridsize must
// be a cube size appropriate for that, i.e.,
//      512,512,512  ;  64
// is fine, as a cube of size 64 can indeed start there. But
//      1, 1, 1      ;  64
// is invalid, as only a cube of size 1 can be in that position.
bool checkCubeCoords(int x, int y, int z, int gridsize)
{
    int curr = 1;
    while (curr < getworldsize())
    {
        if (gridsize == curr)
            break;
        curr *= 2;
    }
    if (gridsize != curr) return false;

    if (gridsize*(x/gridsize) != x ) return false;
    if (gridsize*(y/gridsize) != y ) return false;
    if (gridsize*(z/gridsize) != z ) return false;

    if (x >= getworldsize()) return false;
    if (y >= getworldsize()) return false;
    if (z >= getworldsize()) return false;

    return true;
}

void createCube(int x, int y, int z, int gridsize)
{
    Logging::log(Logging::DEBUG, "createCube: %d,%d,%d  --  %d\r\n", x, y, z, gridsize);

    if (!checkCubeCoords(x, y, z, gridsize))
    {
        Logging::log(Logging::ERROR, "Bad cube coordinates to createCube: %d,%d,%d : %d\r\n", x, y, z, gridsize);
        return;
    }

    // We simulate creating a cube by extruding from another, using mpeditface. This works even if there is no cube there to extrude from.
    selinfo sel;
    // We can either extrude from an imaginary cube from below, or from above. If there is room below, then extrude from there
    if (z - gridsize >= 0)
    {
        sel.o = ivec(x, y, z - gridsize);
        sel.orient = 5; // up
    } else {
        assert(z + gridsize < getworldsize());
        sel.o = ivec(x, y, z + gridsize);
        sel.orient = 4; // down
    }

    sel.s = ivec(1, 1, 1);
    sel.grid = gridsize;

    // Does it matter?
    sel.corner = 1;
    sel.cx = 0;
    sel.cxs = 2;
    sel.cy = 0;
    sel.cys = 2;

    mpeditface(-1, 1, sel, true);
}

void deleteCube(int x, int y, int z, int gridsize)
{
    if (!checkCubeCoords(x, y, z, gridsize))
    {
        Logging::log(Logging::ERROR, "Bad cube coordinates to createCube: %d,%d,%d : %d\r\n", x, y, z, gridsize);
        return;
    }

    selinfo sel;
    sel.o = ivec(x, y, z);
    sel.s = ivec(1, 1, 1);
    sel.grid = gridsize;

    // Does it matter?
    sel.orient = 5;
    sel.corner = 1;
    sel.cx = 0;
    sel.cxs = 2;
    sel.cy = 0;
    sel.cys = 2;

    mpdelcube(sel, true);
}

void setCubeTexture(int x, int y, int z, int gridsize, int face, int texture)
{
    if (!checkCubeCoords(x, y, z, gridsize))
    {
        Logging::log(Logging::ERROR, "Bad cube coordinates to setCubeTexture: %d,%d,%d : %d\r\n", x, y, z, gridsize);
        return;
    }

    assert(face >= -1 && face < 6);

    selinfo sel;
    sel.o = ivec(x, y, z);
    sel.s = ivec(1, 1, 1);
    sel.grid = gridsize;

    // Does it matter?
    sel.orient = face != -1 ? face : 5;
    sel.corner = 1;
    sel.cx = 0;
    sel.cxs = 2;
    sel.cy = 0;
    sel.cys = 2;

    mpedittex(texture, face == -1, sel, true);
}

void setCubeMaterial(int x, int y, int z, int gridsize, int material)
{
    if (!checkCubeCoords(x, y, z, gridsize))
    {
        Logging::log(Logging::ERROR, "Bad cube coordinates to setCubeMaterial: %d,%d,%d : %d\r\n", x, y, z, gridsize);
        return;
    }

    selinfo sel;
    sel.o = ivec(x, y, z);
    sel.s = ivec(1, 1, 1);
    sel.grid = gridsize;

    // Does it matter?
    sel.orient = 5;
    sel.corner = 1;
    sel.cx = 0;
    sel.cxs = 2;
    sel.cy = 0;
    sel.cys = 2;

    mpeditmat(material, 0, sel, true);
}


int cornerTranslators[6][4] =
    {
        /* 0 */ { 2, 3, 0, 1 },
        /* 1 */ { 3, 2, 1, 0 },
        /* 2 */ { 3, 1, 2, 0 },
        /* 3 */ { 1, 3, 0, 2 },
        /* 4 */ { 0, 1, 2, 3 },
        /* 5 */ { 0, 1, 2, 3 }
    };

void pushCubeCorner(int x, int y, int z, int gridsize, int face, int corner, int direction)
{
    if (!checkCubeCoords(x, y, z, gridsize))
    {
        Logging::log(Logging::ERROR, "Bad cube coordinates to pushCubeCorner: %d,%d,%d : %d\r\n", x, y, z, gridsize);
        return;
    }

    assert(face >= 0 && face < 6);
    assert(corner >= 0 && corner < 4);

    selinfo sel;
    sel.o = ivec(x, y, z);
    sel.s = ivec(1, 1, 1);
    sel.grid = gridsize;

    sel.orient = face;
    sel.corner = cornerTranslators[face][corner];
    sel.cx = 0;
    sel.cxs = 2;
    sel.cy = 0;
    sel.cys = 2;

    mpeditface(direction, 2, sel, true);
}

void considerForSmoothing(int x, int y, int z, int resolution, unsigned char* data)
{
    Logging::log(Logging::DEBUG, "considerForSmoothing: %d,%d,%d     %d\r\n", x, y, z, resolution);

    const int worldSize = getworldsize();
    const int resolutionFactor = worldSize/resolution;

    int rx = (x*resolution)/worldSize;
    int ry = (y*resolution)/worldSize;
    int rz = (z*resolution)/worldSize;

    if (rx == 0 || ry == 0 || rz == 0 || rx >= resolution-1 || ry >= resolution-1 || rz >= resolution-1)
        return; // We can't smooth if we don't have all 6 neighbors, to take into consideration

    // Try to perform a simple deformation of this cube that would make it blend smoothly with its neighbors

    int base = rx*resolution*resolution + ry*resolution + rz;

    int up_x = data[base + resolution*resolution];
    int up_y = data[base + resolution];
    int up_z = data[base + 1];
    int down_x = data[base - resolution*resolution];
    int down_y = data[base - resolution];
    int down_z = data[base - 1];

    Logging::log(Logging::DEBUG, "trying to apply smoothing: %d,%d,%d,%d,%d,%d\r\n", up_x, down_x, up_y, down_y, up_z, down_z);

    // Look for the case where we are a corner, i.e., 3 neighbors are empty and 3 are full, in the appropriate alignment

    // We need opposing sides to have opposite contents - one full, the other empty.
    if ( up_x + down_x == 1 && up_y + down_y == 1 && up_z + down_z == 1 &&
            up_x + down_x + up_y + down_y + up_z + down_z == 3 )
            // And to have three full directions and three empty
    {
        Logging::log(Logging::DEBUG, "smoothing is appropriate\r\n");

//        deleteCube(x, y, z, resolutionFactor);
//        createCube(x, y, z, resolutionFactor);

        // Under those assumptions, find the appropriate orientation to deform. There are eight cases, one for
        // each corner of a cube
        if (down_x && down_y && down_z)
            { loopi(8) pushCubeCorner(x, y, z, resolutionFactor, 5, 3, 1); }
        else if (down_x && down_y && up_z)
            { loopi(8) pushCubeCorner(x, y, z, resolutionFactor, 1, 2, 1); }
        else if (down_x && up_y && down_z)
            { loopi(8) pushCubeCorner(x, y, z, resolutionFactor, 1, 1, 1); }
        else if (down_x && up_y && up_z)
            { loopi(8) pushCubeCorner(x, y, z, resolutionFactor, 1, 3, 1); }
        else if (up_x && down_y && down_z)
            { loopi(8) pushCubeCorner(x, y, z, resolutionFactor, 0, 1, 1); }
        else if (up_x && down_y && up_z)
            { loopi(8) pushCubeCorner(x, y, z, resolutionFactor, 3, 2, 1); }
        else if (up_x && up_y && down_z)
            { loopi(8) pushCubeCorner(x, y, z, resolutionFactor, 0, 0, 1); }
        if (up_x && up_y && up_z)
            { loopi(8) pushCubeCorner(x, y, z, resolutionFactor, 0, 2, 1); }
        
    }
}

// Internal function for createMapFromRaw
void createCubeFromRaw(int x, int y, int z, int gridsize, int resolution, unsigned char* data, int smoothing)
{
    Logging::log(Logging::DEBUG, "createCubeFromRaw: %d,%d,%d    %d,%d\r\n", x, y, z, gridsize, resolution);
    INDENT_LOG(Logging::DEBUG);

    const int worldSize = getworldsize();
    const int resolutionFactor = worldSize/resolution;
    #define lookupRaw(rx, ry, rz) \

    int filled = 0, checked = 0;
    int ix, iy, iz, maxiz;
    unsigned char* currBase;

    ix = ((x*resolution)/worldSize) * resolution*resolution;

    for (int i = 0; i < gridsize; i += resolutionFactor)
    {
        iy = ((y*resolution)/worldSize) * resolution;

        for (int j = 0; j < gridsize; j += resolutionFactor)
        {
            // Inner loop - optimized as much as we can
            maxiz = ((z*resolution)/worldSize) + (gridsize/resolutionFactor);
            currBase = &data[ix + iy];
            for (iz = (z*resolution)/worldSize; iz < maxiz; iz++)
            {
                if (currBase[iz])
                    filled++;
            }

            checked += gridsize/resolutionFactor;

            // Every so often (hence in this loop, not the inner one), check if we can bail
            if (filled > 0 && filled < checked)
                break; // This will not be full or empty, it is in the middle somehow

            iy += resolution;
        }

        ix += resolution*resolution;
    }

    bool tooSmall = gridsize <= resolutionFactor; // If this small, we need to decide by veto - no recursing into, would be senseless

    Logging::log(Logging::DEBUG, "checked, toosmall, filled: %d,%d,%d\r\n", checked, tooSmall, filled);

    if (filled == 0 || (tooSmall && filled < checked/2) )
    {
        Logging::log(Logging::DEBUG, "createCubeFromRaw: empty\r\n");
        return; // Empty space, do nothing
    }
    else if (filled == checked || (tooSmall && filled >= checked/2) )
    {
        Logging::log(Logging::DEBUG, "createCubeFromRaw: full\r\n");

        // Create a single simple cube, with a default texture
        createCube(x, y, z, gridsize);
        setCubeTexture(x, y, z, gridsize, -1, 10);

        if (smoothing)
        {
            if (gridsize == resolutionFactor)
                considerForSmoothing(x, y, z, resolution, data); // This is a single cube, consider to smooth it
            else
            {
                // Consider smoothing the 8 corners
                considerForSmoothing(x, y, z, resolution, data);
                considerForSmoothing(x+gridsize-resolutionFactor, y, z, resolution, data);
                considerForSmoothing(x, y+gridsize-resolutionFactor, z, resolution, data);
                considerForSmoothing(x, y, z+gridsize-resolutionFactor, resolution, data);
                considerForSmoothing(x+gridsize-resolutionFactor, y+gridsize-resolutionFactor, z, resolution, data);
                considerForSmoothing(x+gridsize-resolutionFactor, y, z+gridsize-resolutionFactor, resolution, data);
                considerForSmoothing(x, y+gridsize-resolutionFactor, z+gridsize-resolutionFactor, resolution, data);
                considerForSmoothing(x+gridsize-resolutionFactor, y+gridsize-resolutionFactor, z+gridsize-resolutionFactor, resolution, data);
            }
        }
    }
    else
    {
        Logging::log(Logging::DEBUG, "createCubeFromRaw: partial\r\n");

        // Partially-filled space.

        // TODO: Simulated annealing attempt to fill it with a single deformed cube. If fail on that, then do the following.

        // Recuse into subcubes
        loopi(2) loopj(2) loopk(2)
            createCubeFromRaw(x + i*gridsize/2, y + j*gridsize/2, z + k*gridsize/2, gridsize/2, resolution, data, smoothing);
    }
}

int thread__resolution;
unsigned char* thread__data;
int thread__smoothing;

int thread__createMapFromRaw(void *unused)
{
    Logging::log(Logging::DEBUG, "createMapFromRaw thread: %d,%lu,%d\r\n", thread__resolution, thread__data, thread__smoothing);

    eraseGeometry();

    // Fill with new data
    int halfSize = getworldsize()/2;
    loopi(2) loopj(2) loopk(2)
        createCubeFromRaw(i*halfSize, j*halfSize, k*halfSize, halfSize, thread__resolution, thread__data, thread__smoothing);

    delete thread__data;

    return 0;
}

void createMapFromRaw(int resolution, double addr, int smoothing)
{
    thread__resolution = resolution;
    thread__data = new unsigned char[resolution*resolution*resolution]; // Make a copy, as addr points to something Python will collect now
    memcpy(thread__data, (void*)(unsigned long)addr, resolution*resolution*resolution);
    thread__smoothing = smoothing;

    SDL_Thread *thread = SDL_CreateThread(thread__createMapFromRaw, NULL);
    if ( thread == NULL )
        Logging::log(Logging::ERROR, "Unable to create createMapFromRaw thread: %s\n", SDL_GetError());
}

int pushing_needed(float max_height, float curr, int gridsize)
{
//    return (int)round( min(1.0f, (max_height - curr)/gridsize) * 8 );
    return (int)floor( (min(1.0f, (max_height - curr)/gridsize) * 8) + 0.5);
}

#define MIN_HEIGHTMAP_GRIDSIZE 16

void heightmapArea(int x, int y, int gridsize, float low_x_low_y, float low_x_high_y, float high_x_low_y, float high_x_high_y)
{
    float max_height = max(max(low_x_low_y, low_x_high_y), max(high_x_low_y, high_x_high_y));
    max_height = ceil(max_height / gridsize) * gridsize; // We start the cube at the maximum height (cube-aligned), then push down

    bool needLower = false;

    if (max_height - low_x_low_y > gridsize ||
        max_height - low_x_high_y > gridsize ||
        max_height - high_x_low_y > gridsize ||
        max_height - high_x_high_y > gridsize)
    {
        if (gridsize >= MIN_HEIGHTMAP_GRIDSIZE)
        {
            printf("maxxxx: %f       %f,%f,%f,%f       (%d)\r\n", max_height, low_x_low_y, low_x_high_y, high_x_low_y, high_x_high_y, gridsize);
            // We cannot push all corners down enough for this to work - we have run into an alignment error. Subdivide.
            float low_x_mid_y = (low_x_low_y + low_x_high_y)/2;
            float mid_x_low_y = (low_x_low_y + high_x_low_y)/2;

            float high_x_mid_y = (high_x_low_y + high_x_high_y)/2;
            float mid_x_high_y = (low_x_high_y + high_x_high_y)/2;

            float mid_x_mid_y = (low_x_low_y + low_x_high_y + high_x_low_y + high_x_high_y)/4;

            heightmapArea(x,            y,            gridsize/2, low_x_low_y, low_x_mid_y, mid_x_low_y, mid_x_mid_y);
            heightmapArea(x+gridsize/2, y,            gridsize/2, mid_x_low_y, mid_x_mid_y, high_x_low_y, high_x_mid_y);
            heightmapArea(x,            y+gridsize/2, gridsize/2, low_x_mid_y, low_x_high_y, mid_x_mid_y, mid_x_high_y);
            heightmapArea(x+gridsize/2, y+gridsize/2, gridsize/2, mid_x_mid_y, mid_x_high_y, high_x_mid_y, high_x_high_y);

            return;
        } else
            needLower = true;
    }

    int z = (int)(max_height) - gridsize;

    createCube(x, y, z, gridsize);
    setCubeTexture(x, y, z, gridsize, -1, 5);

    if (needLower)
    {
        int lowerZ = z - gridsize;
        createCube(x, y, lowerZ, gridsize);
        setCubeTexture(x, y, lowerZ, gridsize, -1, 5);
    }

    int left;

    left = pushing_needed(max_height, low_x_low_y, gridsize);
    loopi(left)
        pushCubeCorner(x, y, z, gridsize, 5, 0, 1);

    left = pushing_needed(max_height, high_x_low_y, gridsize);
    loopi(left)
        pushCubeCorner(x, y, z, gridsize, 5, 1, 1);

    left = pushing_needed(max_height, low_x_high_y, gridsize);
    loopi(left)
        pushCubeCorner(x, y, z, gridsize, 5, 2, 1);

    left = pushing_needed(max_height, high_x_high_y, gridsize);
    loopi(left)
        pushCubeCorner(x, y, z, gridsize, 5, 3, 1);
}

float* thread__heightmapData;

int thread__createHeightmapFromRaw(void *unused)
{
    Logging::log(Logging::DEBUG, "createHeightmapFromRaw thread: %d,%lu\r\n", thread__resolution, thread__heightmapData);

    eraseGeometry();

    int resolution = thread__resolution;

    int world_size = getWorldSize();
    int gridsize = world_size / resolution;

    loopi(resolution-1)
        loopj(resolution-1)
        {
            // Calculate heights at all 4 corners of this square area
            float low_x_low_y = thread__heightmapData[i*resolution + j] * world_size;
            float low_x_high_y = thread__heightmapData[i*resolution + j+1] * world_size;
            float high_x_low_y = thread__heightmapData[(i+1)*resolution + j] * world_size;
            float high_x_high_y = thread__heightmapData[(i+1)*resolution + j+1] * world_size;

            heightmapArea(i*gridsize, j*gridsize, gridsize,
                          low_x_low_y, low_x_high_y, high_x_low_y, high_x_high_y);
        }

    delete thread__heightmapData;

    return 0;
}

void createHeightmapFromRaw(int resolution, double addr)
{
    thread__resolution = resolution;
    assert(sizeof(float) == 4); // This is what Python sends us
    thread__heightmapData = new float[resolution*resolution]; // Make a copy, as addr points to something Python will collect now
    memcpy(thread__heightmapData, (void*)(unsigned long)addr, resolution*resolution*4);

    SDL_Thread *thread = SDL_CreateThread(thread__createHeightmapFromRaw, NULL);
    if ( thread == NULL )
        Logging::log(Logging::ERROR, "Unable to create createHeightmapFromRaw thread: %s\n", SDL_GetError());
}

}


#ifdef CLIENT
    // Cubescript tools

    std::set<int> listTextures;

    void listtexcube(cube &c)
    {
        if (!c.children)
            loopi(6) listTextures.insert(c.texture[i]);
        else
            loopi(8) listtexcube(c.children[i]);
    }

    //! List the textures actually used
    void listtex()
    {
        listTextures.clear();
        loopi(8) listtexcube(worldroot[i]);
        for (std::set<int>::iterator it = listTextures.begin(); it != listTextures.end(); it++)
        {
            extern Slot dummyslot;
            Slot &currSlot = lookuptexture(*it, false);
            if (&currSlot != &dummyslot)
                printf("%d : %s\r\n", *it, currSlot.sts[0].name);
        }
    }

    COMMAND(listtex, "");


    std::vector<int> massReplaceLookup;

    void massreplacetexcube(cube &c, boost::python::dict lookup)
    {
        if (!c.children) loopi(6)
        {
            int oldIndex = c.texture[i];
            int newIndex = boost::python::extract<int>(lookup.attr("get")(oldIndex, -1));
            if (newIndex != -1)
            {
//                printf("Replacing %d ==> %d\r\n", oldIndex, newIndex);
                c.texture[i] = newIndex;
            }
        }

        if (c.children)
            loopi(8) massreplacetexcube(c.children[i], lookup);
    }

    //! See prepare_texture_replace in Python
    void massreplacetex(char *filename)
    {
        massReplaceLookup.clear();

        REFLECT_PYTHON( prepare_texture_replace );
        boost::python::list currTextures;
        extern vector<Slot> slots;
        for (int i = 0; i < slots.length(); i++)
            currTextures.append(slots[i].sts[0].name);
        boost::python::dict lookup = boost::python::extract<boost::python::dict>(prepare_texture_replace(std::string(filename), currTextures));

        loopi(8) massreplacetexcube(worldroot[i], lookup);
        allchanged();
    }

    COMMAND(massreplacetex, "s");

#endif


// Debugging
void debugcube(cube &c, int size, int x, int y, int z)
{
    if (!c.children)
    {
        printf("%4d,%4d,%4d : %4d,%4d,%4d   (%.8x,%.8x,%.8x)\r\n", x, y, z, x+size, y+size, z+size, c.faces[0], c.faces[1], c.faces[2]);
        if (c.ext && c.ext->ents)
        {
            printf("   ents: %d,%d\r\n", c.ext->ents->mapmodels.length(), c.ext->ents->other.length());
            printf("   %d   %d,%d,%d   %d    %d,%d,%d : %d,%d,%d\r\n",
                c.ext->ents->distance,
                c.ext->ents->o.x,
                c.ext->ents->o.y,
                c.ext->ents->o.z,
                c.ext->ents->size,
                c.ext->ents->bbmin.x,
                c.ext->ents->bbmin.y,
                c.ext->ents->bbmin.z,
                c.ext->ents->bbmax.x,
                c.ext->ents->bbmax.y,
                c.ext->ents->bbmax.z
            );
            if (c.ext->ents->mapmodels.length() > 0)
                for (int i = 0; i < c.ext->ents->mapmodels.length(); i++)
                {
                    printf("            %d : %d\r\n", i, c.ext->ents->mapmodels[i]);
//                    const vector<extentity *> &ents = entities::getents();
//                    printf("         %d\r\n", LogicSystem::getLogicEntity(ents[c.ext->ents->mapmodels[i]])->getUniqueId());
                }
        }
    } else {
        debugcube(c.children[0], size/2, 0, 0, 0);
        debugcube(c.children[0], size/2, size/2, 0, 0);
        debugcube(c.children[0], size/2, 0, size/2, 0);
        debugcube(c.children[0], size/2, size/2, size/2, 0);
        debugcube(c.children[0], size/2, 0, 0, size/2);
        debugcube(c.children[0], size/2, size/2, 0, size/2);
        debugcube(c.children[0], size/2, 0, size/2, size/2);
        debugcube(c.children[0], size/2, size/2, size/2, size/2);
    }
}

//! List the textures actually used
void debugoctree()
{
    debugcube(worldroot[0], worldsize/2, 0, 0, 0);
    debugcube(worldroot[1], worldsize/2, worldsize/2, 0, 0);
    debugcube(worldroot[2], worldsize/2, 0, worldsize/2, 0);
    debugcube(worldroot[3], worldsize/2, worldsize/2, worldsize/2, 0);
    debugcube(worldroot[4], worldsize/2, 0, 0, worldsize/2);
    debugcube(worldroot[5], worldsize/2, worldsize/2, 0, worldsize/2);
    debugcube(worldroot[6], worldsize/2, 0, worldsize/2, worldsize/2);
    debugcube(worldroot[7], worldsize/2, worldsize/2, worldsize/2, worldsize/2);
}

COMMAND(debugoctree, "");

//CModule.run_cubescript("debugoctree");

