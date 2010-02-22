
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

#include "message_system.h"
#include "utility.h"
#include "script_engine_manager.h"

#include "world_system.h"


// Kripken: These are bounding boxes for positioning in the octree, as opposed to bounding boxes that
// are actually used to check for collisions. These octree-positioning bounding boxes only determine
// in which octants we will sit, i.e., WHEN actual collisions will be checked. If you see a bounding
// box which looks ok but you can walk through it (or part of it) then it might simply be that the
// bounding box is fine, but sauer doesn't know that a certain world area actually contains that
// entity.
///////////////////bool getentboundingbox(extentity &e, ivec &o, ivec &r)


//! Set by load_world when started, unset when finished
bool WorldSystem::loadingWorld = false;


void WorldSystem::placeInWorld(int entityUniqueId, int locationEntityUniqueId)
{
    assert(0); // Be wary of this code, in particular see the XXX below about converting physent to dynent
    Logging::log(Logging::DEBUG, "Placing entity in world\r\n");
    INDENT_LOG(Logging::DEBUG);

    LogicEntityPtr entity = LogicSystem::getLogicEntity(entityUniqueId);

    assert(entity.get() != NULL);
    assert(entity.get()->getType() == CLogicEntity::LE_DYNAMIC);

    LogicEntityPtr locationEntity = LogicSystem::getLogicEntity(locationEntityUniqueId);

    assert(locationEntity.get()->getType() == CLogicEntity::LE_STATIC); // Might be anything static, but probably a worldMarker

    entity.get()->dynamicEntity->o   = locationEntity.get()->staticEntity->o;
    entity.get()->dynamicEntity->yaw = locationEntity.get()->staticEntity->attr1;

    if (!entinmap((dynent*)entity.get()->dynamicEntity, true)) // XXX This conversion to dynent might be bad.
        Logging::log(Logging::ERROR, "Cannot place entity %d in world at %d\r\n",
                                     entity.get()->getUniqueId(),
                                     locationEntityUniqueId);
}

void WorldSystem::triggerCollide(LogicEntityPtr mapmodel, physent* d, bool ellipse)
{
    Logging::log(Logging::INFO, "triggerCollide: %lu, %lu\r\n", (unsigned long)mapmodel.get(), (unsigned long)d);

    if (d->type != ENT_PLAYER)
    {
//        Logging::log(Logging::INFO, "Non-player causing collide, so ignore\r\n");
        return; // No need to trigger collisions for cameras, lights, etc. TODO: ENT_AI?
    }

    if (!mapmodel.get() || mapmodel.get()->isNone())
    {
        Logging::log(Logging::ERROR, "Invalid mapmodel to trigger collide for\r\n");
        return; // Invalid or uninialized mapmodel
    }

    LogicEntityPtr colliderEntity = LogicSystem::getLogicEntity(d);
    if (!colliderEntity.get() || colliderEntity.get()->isNone())
    {
        Logging::log(Logging::INFO, "Invalid colliding entity to collide with\r\n");
        return; // Most likely a raycasting collision, or camera, etc. - not things we trigger events for
    }

    #ifdef SERVER
        mapmodel.get()->scriptEntity->call("onCollision", colliderEntity.get()->scriptEntity );
    #else // CLIENT
        mapmodel.get()->scriptEntity->call("clientOnCollision", colliderEntity.get()->scriptEntity );
    #endif
}

int numExpectedEntities = 0;
int numReceivedEntities = 0;

void WorldSystem::setNumExpectedEntities(int num)
{
    numExpectedEntities = num;
    numReceivedEntities = 0;
}

void WorldSystem::triggerReceivedEntity()
{
    numReceivedEntities += 1;

    if (numExpectedEntities > 0)
    {
        float val = float(numReceivedEntities)/float(numExpectedEntities);
        val = clamp(val, 0.0f, 1.0f);
        std::string text = "received entity ";
        text += Utility::toString(numReceivedEntities) + "...";
        if (WorldSystem::loadingWorld) // Show message only during map loading, not when new clients log in
            renderprogress(val, text.c_str());
    }
}

void WorldSystem::runMapScript()
{
    REFLECT_PYTHON(run_map_script);

    run_map_script();

    // Some post-map script settings. TODO: Cleanup

    #ifdef CLIENT
        extern void setScreenScriptValues();
        setScreenScriptValues();
    #endif
}


// Convenience tools for entities

int getEntId(extentity *entity)
{
    vector<extentity *> &ents = entities::getents();
    int id = 0;
    while (ents[id] != entity)
    {
        id++;
        assert(id < ents.length());
    }

    return id;
}

extern void addentity(int id);
// Kripken: Version of this with a point instead of an ent #
void addentity(extentity* entity)
{
    addentity(getEntId(entity));
}

extern void removeentity(int id);
// Kripken: Version of this with a point instead of an ent #
void removeentity(extentity *entity)
{
    removeentity(getEntId(entity));
}


// AreaTrigger collisions

bool WorldSystem::triggeringCollisions = false;

//! Check for triggering collisions, i.e., to run trigger events on AreaTriggers
void WorldSystem::checkTriggeringCollisions(LogicEntityPtr entity)
{
    assert(entity->isDynamic());

    WorldSystem::triggeringCollisions = true;
    collide(entity->dynamicEntity, vec(0,0,0));
    WorldSystem::triggeringCollisions = false;
}

