
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

#include "cube.h"
#include "engine.h"
#include "game.h"

#include "intensity_physics.h"
#include "intensity_physics_sauer.h"


void SauerPhysicsEngine::init()
{
}

void SauerPhysicsEngine::destroy()
{
}

void SauerPhysicsEngine::addStaticPolygon(std::vector<vec> vertexes)
{
}

/*
struct SauerDynamic
{
    float radius;
    SauerDynamic(float _radius) : radius(_radius) { };
};
*/

physicsHandle SauerPhysicsEngine::addDynamicSphere(float mass, float radius)
{
    assert(0);
    return 0;
}

void SauerPhysicsEngine::removeDynamic(physicsHandle handle)
{
    assert(0);
}

void SauerPhysicsEngine::setDynamicPosition(physicsHandle handle, const vec& position)
{
    assert(0);
}

void SauerPhysicsEngine::setDynamicVelocity(physicsHandle handle, const vec& velocity)
{
    assert(0);
}

void SauerPhysicsEngine::getDynamic(physicsHandle handle, vec& position, vec& velocity)
{
    assert(0);
}

namespace game
{
    extern vector<fpsent *> players;
    extern void otherplayers(int curtime);
    extern void moveControlledEntities();
}

void SauerPhysicsEngine::simulate(float seconds)
{
    #ifdef CLIENT
        game::otherplayers(curtime); // Server doesn't need smooth interpolation of other players
    #endif

    game::moveControlledEntities();

    loopv(game::players)
    {
        fpsent* fpsEntity = game::players[i];
        LogicEntityPtr entity = LogicSystem::getLogicEntity(fpsEntity);
        if (!entity.get() || entity->isNone()) continue;

        #ifdef CLIENT
            // Ragdolls
            int anim = entity->getAnimation();
            if (fpsEntity->ragdoll && !(anim&ANIM_DYING))
            {
                cleanragdoll(fpsEntity);
            }
            if (fpsEntity->ragdoll && (anim&ANIM_DYING))
            {
                moveragdoll(fpsEntity);
            }
        #endif
    }
}

