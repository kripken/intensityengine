
// Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
// This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

// weapon.cpp: all shooting and effects code, projectile management
#include "cube.h"
#include "game.h"

namespace game
{
    bool intersect(dynent *d, const vec &from, const vec &to)   // if lineseg hits entity bounding box
    {
        float dist;
        vec bottom(d->o), top(d->o);
        bottom.z -= d->eyeheight;
        top.z += d->aboveeye;
        return linecylinderintersect(from, to, bottom, top, d->radius, dist);
    }
};
