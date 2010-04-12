
// Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
// This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

#include "cube.h"
#include "engine.h"
#include "game.h"

#include "targeting.h"

#include "intensity_physics.h"
#include "intensity_physics_sauer.h"


void SauerPhysicsEngine::init()
{
}

void SauerPhysicsEngine::destroy()
{
}

void SauerPhysicsEngine::setGravity(float g)
{
    extern float GRAVITY;
    GRAVITY = g;
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

bool SauerPhysicsEngine::isColliding(vec& position, float radius, CLogicEntity *ignore)
{
    physent tester;
    tester.reset();
    tester.type = ENT_BOUNCE;
    tester.o = position;
    tester.radius = tester.xradius = tester.yradius = radius;
    tester.eyeheight = radius;
    tester.aboveeye = radius;

    if (!collide(&tester, vec(0, 0, 0)))
    {
        extern physent *hitplayer; // physics.cpp

        if (ignore && ignore->isDynamic() && ignore->dynamicEntity == hitplayer)
        {
            // Try to see if the ignore was the sole cause of collision - move it away, test, then move it back
            vec save = ignore->dynamicEntity->o;
            avoidcollision(ignore->dynamicEntity, vec(1,1,1), &tester, 0.1f);
            bool ret = !collide(&tester, vec(0, 0, 0));
            ignore->dynamicEntity->o = save;
            return ret;
        } else
            return true;
    } else
        return false;
}

void SauerPhysicsEngine::rayCastClosest(vec &from, vec &to, float& hitDist, LogicEntityPtr& hitEntity, CLogicEntity* ignore)
{
    static LogicEntityPtr placeholderLogicEntity(new CLogicEntity());

    // Manually check if we are hovering, using ray intersections. TODO: Not needed for extents?
    float dynamicDist, staticDist;
    dynent* dynamicEntity;
    extentity* staticEntity;
    TargetingControl::intersectClosestDynamicEntity(from, to, ignore ? ignore->dynamicEntity : NULL, dynamicDist,  dynamicEntity);
    TargetingControl::intersectClosestMapmodel     (from, to,                                        staticDist, staticEntity);

    hitDist = -1;

    if (dynamicEntity == NULL && staticEntity == NULL)
    {
        hitDist = -1;
        hitEntity = placeholderLogicEntity;
    } else if (dynamicEntity != NULL && staticEntity == NULL)
    {
        hitDist = dynamicDist;
        hitEntity = LogicSystem::getLogicEntity(dynamicEntity);
    } else if (dynamicEntity == NULL && staticEntity != NULL)
    {
        hitDist = staticDist;
        hitEntity = LogicSystem::getLogicEntity(*staticEntity);
    } else if (staticDist < dynamicDist)
    {
        hitDist = staticDist;
        hitEntity = LogicSystem::getLogicEntity(*staticEntity);
    } else {
        hitDist = dynamicDist;
        hitEntity = LogicSystem::getLogicEntity(dynamicEntity);
    }
}

