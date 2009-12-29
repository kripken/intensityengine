
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


#include "pch.h"
#include "engine.h"

#include "door.h"


#define DOOR_OPEN_ANGLE (3.14159f * 0.90f)

int         DoorManager::numFrames        = 30;
const char* DoorManager::animationCommand = "md2anim trigger 0 30 30";

void DoorManager::modifyInitialFrame(vert* firstVert, int numVerts)
{
    // Find lowest x
    float minX = firstVert[0].pos.x;
    loopi(numVerts)
        minX = min(minX, firstVert[i].pos.x);

    // Re-align
    loopi(numVerts)
        firstVert[i].pos.x -= minX;
}

void DoorManager::generateDoorAnimationFrame(int i, vert* firstVert, vert* currVert, int numVerts)
{
    float angle = ( float(i) * DOOR_OPEN_ANGLE ) / float(numFrames-1);

    loopj(numVerts)
    {
        currVert[j].pos  = firstVert[j].pos;
        currVert[j].pos.rotate_around_z(angle); // y is axis straight forward, z is down, x is to the right

        currVert[j].norm = firstVert[j].norm;
        currVert[j].norm.rotate_around_z(angle);

//        printf("Created a frame, %d   %f,%f,%f\r\n", i, currVert[j].pos.x,currVert[j].pos.y,currVert[j].pos.z);
    }
}

