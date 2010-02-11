
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
#include "client_system.h"
#include "fpsserver_interface.h"
#include "world_system.h"
#include "script_engine_manager.h"
#include "utility.h"
#include "fpsclient_interface.h"
#include "NPC.h"
#include "intensity_physics.h"


// WorldSystem
extern void removeentity(extentity* entity);
extern void addentity(extentity* entity);

using namespace boost;


//=========================
// Logic Entities
//=========================

int CLogicEntity::getUniqueId()
{
    switch (getType())
    {
        case LE_DYNAMIC:
            return LogicSystem::getUniqueId(dynamicEntity);
        case LE_STATIC:
            return LogicSystem::getUniqueId(staticEntity);
        case LE_NONSAUER:
            return uniqueId; // This can be made to work for the others, if we ensure that uniqueId is set. Would be faster
        default:
            assert(0 && "getting the unique ID of a NONE logic entity!\r\n");
            return -1;
    };
}

int CLogicEntity::getType()
{
    if (dynamicEntity != NULL)
        return LE_DYNAMIC;
    else if (staticEntity != NULL)
        return LE_STATIC;
    else if (nonSauer)
        return LE_NONSAUER;
    else
        return LE_NONE;
}

extern vector<mapmodelinfo> mapmodels; // KLUDGE

vec CLogicEntity::getOrigin()
{
    switch (getType())
    {
        case LE_DYNAMIC:
            return dynamicEntity->o;
        case LE_STATIC:
        {
            if (staticEntity->type == ET_MAPMODEL)
            {
                vec bbcenter, bbradius;
                model *m = theModel;
                if (m)
                {
                    LogicEntityPtr ptr = LogicSystem::getLogicEntity(getUniqueId());
                    assert(ptr.get());
                    m->collisionbox(0, bbcenter, bbradius, ptr.get());
                    rotatebb(bbcenter, bbradius, int(staticEntity->attr1));
                    bbcenter.add(staticEntity->o);
                    return bbcenter;
                } else {
                    Logging::log(Logging::WARNING, "Invalid mapmodel model\r\n");
                    return staticEntity->o;
                }
            } else
                return staticEntity->o;
        }
    };

    assert(0 && "getting the origin of a NONE or non-Sauer LogicEntity!");
    return vec(0,0,0);
}

float CLogicEntity::getRadius()
{
    switch (getType())
    {
        case LE_DYNAMIC:
            return 10;
        case LE_STATIC:
        {
            if (staticEntity->type == ET_MAPMODEL)
            {
                vec bbcenter, bbradius;
                model *m = theModel;
                if (m)
                {
                    LogicEntityPtr ptr = LogicSystem::getLogicEntity(getUniqueId());
                    assert(ptr.get());
                    m->collisionbox(0, bbcenter, bbradius, ptr.get());
                    rotatebb(bbcenter, bbradius, int(staticEntity->attr1));
                    bbcenter.add(staticEntity->o);
                    return bbradius.x + bbradius.y;
                } else {
                    Logging::log(Logging::WARNING, "Invalid mapmodel model, cannot find radius\r\n");
                    return 8;
                }

            } else
                return 8;
        }
    };

    assert(0 && "getting the radius of a NONE or non-Sauer LogicEntity!");
    return -1;
}

void CLogicEntity::setOrigin(vec &newOrigin)
{
    ScriptEngineManager::runScript("getEntity(" + Utility::toString(getUniqueId()) + ").position = [" +
        Utility::toString(newOrigin.x) + "," +
        Utility::toString(newOrigin.y) + "," +
        Utility::toString(newOrigin.z) + "]"
    );
}

int CLogicEntity::getAnimation()
{
    return animation;
//    int ret = python::extract<int>( scriptEntity.attr("animation") );
////    printf("Animation: %ld\r\n", ret);
//    return ret;
}

int CLogicEntity::getStartTime()
{
    return startTime;
//    int ret = python::extract<int>( scriptEntity.attr("start_time") );
////    printf("Start Time: %ld\r\n", ret);
//    return ret;
}

int CLogicEntity::getAnimationFrame()
{
    return 0; // Deprecated for now (can do doors in other ways anyhow)
//    int ret = python::extract<int>( scriptEntity.attr("get_animation_frame")() );
//    return ret;
}

std::string CLogicEntity::getClass()
{
    return scriptEntity->getPropertyString("_class");
}

model* CLogicEntity::getModel()
{
    // This is important as this is called before setupExtent.
    if ((!this) || (!staticEntity && !dynamicEntity))
        return NULL;

//    if (!theModel)
//        setModel(python::extract<std::string>( scriptEntity.attr("model_name") ));

    // Fallback to sauer mapmodel system, if not overidden (-1 or less in attr2 if so)
    if (staticEntity && staticEntity->type == ET_MAPMODEL && staticEntity->attr2 >= 0)
    {
        // If no such model, leave the current model, from modelName
        model* possible = loadmodel(NULL, staticEntity->attr2);
        if (possible && possible != theModel)
            theModel = possible;
    }

    return theModel;
}

void CLogicEntity::setModel(std::string name)
{
    // This is important as this is called before setupExtent.
    if ((!this) || (!staticEntity && !dynamicEntity))
        return;

    if (staticEntity)
        removeentity(staticEntity);

    if (name != "")
        theModel = loadmodel(name.c_str());

    Logging::log(Logging::DEBUG, "CLE:setModel: %s (%lu)\r\n", name.c_str(), (unsigned long)theModel);

    if (staticEntity)
    {
//        #ifdef SERVER
//            if (name != "" && staticEntity->type == ET_MAPMODEL)
//            {
//                scriptEntity->setProperty("attr2", -1); // Do not use sauer mmodel list
//            }
//        #endif

        // Refresh the entity in sauer
        addentity(staticEntity);
    }
}

void CLogicEntity::setAttachments(std::string _attachments)
{
    Logging::log(Logging::DEBUG, "CLogicEntity::setAttachments: %s\r\n", _attachments.c_str());

    // This is important as this is called before setupExtent.
    if ((!this) || (!staticEntity && !dynamicEntity))
        return;

    // Clean out old data
    for (int i = 0; attachments[i].tag; i++)
    {
        free((void*)attachments[i].tag);
        free((void*)attachments[i].name);
    }

    // Generate new data
    try
    {
        python::object attachmentData = python::object(_attachments);
        attachmentData = attachmentData.attr("split")("|");

        int numAttachments = python::extract<int>( attachmentData.attr("__len__")() );;

        if (_attachments == "") {
            numAttachments = 0; // Because splitting "" gives [""], i.e., one attachment of size 0
        }

        assert(numAttachments <= MAX_ATTACHMENTS);

        std::string tag;
        std::string name;

        // Parse the Python attachments, which are in form [ "tag,name", "tag',name'", etc. ]
        for (int i = 0; i < numAttachments; i++)
        {
            python::object currAttachment = attachmentData.attr("__getitem__")(i).attr("split")(",");
            tag = python::extract<std::string>( currAttachment.attr("__getitem__")(0) );
            if ( python::extract<int>(currAttachment.attr("__len__")()) == 2 )
                name = python::extract<std::string>( currAttachment.attr("__getitem__")(1) );
            else    
                name = ""; // No content specified for this tag

            //! Tags starting with a '*' indicate this is a position marker
            if (tag.length() >= 1 && tag[0] == '*')
            {
                //! Remove the '*' and add the marker address
                tag = tag.substr(1, tag.length()-1);
                attachments[i].pos = &attachmentPositions[i];
                attachmentPositions[i] = vec(0,0,0); // Initialize, as if the attachment doesn't exist in the model,
                                                     // we don't want NaNs and such causing crashes
            } else
                attachments[i].pos = NULL;

            attachments[i].tag = strdup(tag.c_str()); // XXX leak (both the old value, and never clean up this one on removing entity
            attachments[i].name = strdup(name.c_str()); // XXX leak
            //attachments[i].anim = ANIM_VWEP | ANIM_LOOP; // Will become important if/when we have animated attachments
            attachments[i].basetime = 0;

            Logging::log(Logging::DEBUG, "Adding attachment: %s - %s\r\n", attachments[i].name, attachments[i].tag);
        }

        attachments[numAttachments].tag  = NULL; // tag=null as well - probably not needed (order reversed with following line)
        attachments[numAttachments].name = NULL; // Null name element at the end, for sauer to know to stop

    } catch(python::error_already_set const &)
    {
        printf("Error in Python execution of setAttachments\r\n");
        PyErr_Print();
        assert(0 && "Halting on Python error");
    }
}

void CLogicEntity::setAnimation(int _animation)
{
    Logging::log(Logging::DEBUG, "setAnimation: %d\r\n", _animation);

    // This is important as this is called before setupExtent.
    if ((!this) || (!staticEntity && !dynamicEntity))
        return;

    Logging::log(Logging::DEBUG, "(2) setAnimation: %d\r\n", _animation);

    animation = _animation;
    startTime = lastmillis; // Utility::SystemInfo::currTime(); XXX Do NOT want the actual time! We
                            // need 'lastmillis', sauer's clock, which doesn't advance *inside* frames,
                            // because otherwise the starttime may be
                            // LATER than lastmillis, while sauer's animation system does lastmillis-basetime,
                            // leading to a negative number and segfaults in finding frame data
}

vec& CLogicEntity::getAttachmentPosition(std::string tag)
{
    // If last actual render - which actually calculated the attachment positions - was recent
    // enough, use that data
    if (abs(lastmillis - lastActualRenderMillis) < 500)
    {
        // TODO: Use a hash table. But, if just 1-4 attachments, then fast enough for now as is
        for (int i = 0; attachments[i].tag; i++)
        {
            if (attachments[i].tag == tag)
                return attachmentPositions[i];
        }
    }
    static vec missing; // Returned if no such tag, or no recent attachment position info. Note: Only one of these, static!
    missing = getOrigin();
    return missing;
}

void CLogicEntity::noteActualRender()
{
    lastActualRenderMillis = lastmillis;
}


//=========================
// LogicSystem
//=========================

LogicSystem::LogicEntityMap LogicSystem::logicEntities;

void LogicSystem::clear()
{
    Logging::log(Logging::DEBUG, "clear()ing LogicSystem\r\n");
    INDENT_LOG(Logging::DEBUG);

    // Removes existing logic entities, which also removes them from C++
    if (ScriptEngineManager::hasEngine())
    {
        ScriptEngineManager::getGlobal()->call("removeAllEntities");
        assert(logicEntities.size() == 0);

        // For client, remove player logic entity
        #ifdef CLIENT
            ClientSystem::clearPlayerEntity();
        #endif

        // Destroy the scripting engine, for a blank slate
        ScriptEngineManager::destroyEngine();
    }

    PhysicsManager::destroyEngine();
}

void LogicSystem::init()
{
    clear();

    ScriptEngineManager::createEngine();

    // Note: We do not create the physics engine here, as we need for scripting
    // to not only be set up, but to run the map script and so forth. So we
    // create the engine in game::startmap, which is run right after the map loads
}

void LogicSystem::registerLogicEntity(LogicEntityPtr newEntity)
{
    Logging::log(Logging::DEBUG, "C registerLogicEntity: %d\r\n", newEntity.get()->getUniqueId());
    INDENT_LOG(Logging::DEBUG);

    int uniqueId = newEntity.get()->getUniqueId();
    assert(logicEntities.find(uniqueId) == logicEntities.end());
    logicEntities.insert( LogicEntityMap::value_type( uniqueId, newEntity ) );

    newEntity.get()->scriptEntity = ScriptEngineManager::getGlobal()->call("getEntity", uniqueId);

    assert(newEntity.get()->scriptEntity.get()); // Cannot be NULL
    assert( ! ( newEntity.get()->scriptEntity->compare(ScriptEngineManager::getNull()) ) ); // Cannot be NULL

    newEntity.get()->scriptEntity->debugPrint();

    Logging::log(Logging::DEBUG, "C registerLogicEntity completes\r\n");
}

LogicEntityPtr LogicSystem::registerLogicEntity(physent* entity)
{
    if (getUniqueId(entity) < 0)
    {
        Logging::log(Logging::ERROR, "Trying to register an entity with an invalid unique Id: %d (D)\r\n", getUniqueId(entity));
        assert(0);
    }

    LogicEntityPtr newEntity(new CLogicEntity(entity));

    Logging::log(Logging::DEBUG, "adding physent %d\r\n", newEntity.get()->getUniqueId());

    registerLogicEntity(newEntity);

    return newEntity;
}

LogicEntityPtr LogicSystem::registerLogicEntity(extentity* entity)
{
    if (getUniqueId(entity) < 0)
    {
        Logging::log(Logging::ERROR, "Trying to register an entity with an invalid unique Id: %d (S)\r\n", getUniqueId(entity));
        assert(0);
    }

    LogicEntityPtr newEntity(new CLogicEntity(entity));

//    Logging::log(Logging::DEBUG, "adding entity %d : %d,%d,%d,%d\r\n", entity->type, entity->attr1, entity->attr2, entity->attr3, entity->attr4);

    registerLogicEntity(newEntity);

    return newEntity;
}

void LogicSystem::registerLogicEntityNonSauer(int uniqueId)
{
    LogicEntityPtr newEntity(new CLogicEntity(uniqueId));

    newEntity.get()->nonSauer = true; // Set as non-Sauer

    Logging::log(Logging::DEBUG, "adding non-Sauer entity %d\r\n", uniqueId);

    registerLogicEntity(newEntity);

//    return newEntity;
}

void LogicSystem::unregisterLogicEntity(LogicEntityPtr entity)
{
    assert(0); // Deprecated XXX

    Logging::log(Logging::DEBUG, "UNregisterLogicEntity: %d\r\n", entity.get()->getUniqueId());

    int uniqueId = entity.get()->getUniqueId();

    logicEntities.erase(uniqueId);

// TODO: Cleanup
//    if (newEntity.get()->scriptEntity) {
//        delete newEntity.get()->scriptEntity;
//    }
}

void LogicSystem::unregisterLogicEntityByUniqueId(int uniqueId)
{
    Logging::log(Logging::DEBUG, "UNregisterLogicEntity by UniqueID: %d\r\n", uniqueId);
    logicEntities.erase(uniqueId);
}

void LogicSystem::manageActions(long millis)
{
    Logging::log(Logging::INFO, "manageActions: %d\r\n", millis);
    INDENT_LOG(Logging::INFO);

    if (ScriptEngineManager::hasEngine())
        ScriptEngineManager::getGlobal()->call("manageActions",
            ScriptValueArgs().append(double(millis)/1000.0f).append(lastmillis)
        );

    Logging::log(Logging::INFO, "manageActions complete\r\n");
}

LogicEntityPtr LogicSystem::getLogicEntity(int uniqueId)
{
    LogicEntityMap::iterator iter = logicEntities.find(uniqueId);

    if (iter == logicEntities.end())
    {
        Logging::log(Logging::INFO, "(C++) Trying to get a non-existant logic entity %d\r\n", uniqueId);
        LogicEntityPtr NullEntity;
        return NullEntity;
    }

    return iter->second;
}

LogicEntityPtr LogicSystem::getLogicEntity(const extentity &extent)
{
    return getLogicEntity(extent.uniqueId);
}


LogicEntityPtr LogicSystem::getLogicEntity(physent* entity)
{
    return getLogicEntity(getUniqueId(entity)); // TODO: do this directly, without the intermediary getUniqueId, for speed?
}

int LogicSystem::getUniqueId(extentity* staticEntity)
{
    return staticEntity->uniqueId;
}

int LogicSystem::getUniqueId(physent* dynamicEntity)
{
    return dynamic_cast<fpsent*>(dynamicEntity)->uniqueId;
}

// TODO: Use this whereever it should be used
void LogicSystem::setUniqueId(extentity* staticEntity, int uniqueId)
{
    if (getUniqueId(staticEntity) >= 0)
    {
        Logging::log(Logging::ERROR, "Trying to set to %d a unique Id that has already been set, to %d (S)\r\n",
                                     uniqueId,
                                     getUniqueId(staticEntity));
        assert(0);
    }

    staticEntity->uniqueId = uniqueId;
}

// TODO: Use this whereever it should be used
void LogicSystem::setUniqueId(physent* dynamicEntity, int uniqueId)
{
    Logging::log(Logging::DEBUG, "Setting a unique ID: %d (of addr: %d)\r\n", uniqueId, dynamicEntity != NULL);

    if (getUniqueId(dynamicEntity) >= 0)
    {
        Logging::log(Logging::ERROR, "Trying to set to %d a unique Id that has already been set, to %d (D)\r\n",
                                     uniqueId,
                                     getUniqueId(dynamicEntity));
        assert(0);
    }

    dynamic_cast<fpsent*>(dynamicEntity)->uniqueId = uniqueId;
}

void LogicSystem::setupExtent(ScriptValuePtr scriptEntity, int type, float x, float y, float z, int attr1, int attr2, int attr3, int attr4)
{
    int uniqueId = scriptEntity->getPropertyInt("uniqueId");

    Logging::log(Logging::DEBUG, "setupExtent: %d,  %d : %f,%f,%f : %d,%d,%d,%d\r\n", uniqueId, type, x, y, z, attr1, attr2, attr3, attr4);
    INDENT_LOG(Logging::DEBUG);

    vector<extentity *> &ents = entities::getents();

    extentity &e = *(entities::newentity());
    ents.add(&e);

    e.type  = type;
    e.o     = vec(x,y,z);
    e.attr1 = attr1;
    e.attr2 = attr2;
    e.attr3 = attr3;
    e.attr4 = attr4;

    e.inoctanode = false; // This is not set by the constructor in sauer, but by those calling ::newentity(), so we also do that here

    // If this is a new ent for this map - i.e., we are not currently loading the map - then we must
    // do some inserting into the octaworld.
//    if (!WorldSystem::loadingWorld) // XXX Try both ways
    {
        extern void addentity(extentity* entity);
        addentity(&e);
    }

    LogicSystem::setUniqueId(&e, uniqueId);
    LogicSystem::registerLogicEntity(&e);
}

void LogicSystem::setupCharacter(ScriptValuePtr scriptEntity)
{
//    #ifdef CLIENT
//        assert(0); // until we figure this out
//    #endif

    int uniqueId = scriptEntity->getPropertyInt("uniqueId");

    Logging::log(Logging::DEBUG, "setupCharacter: %d\r\n", uniqueId);
    INDENT_LOG(Logging::DEBUG);

    fpsent* fpsEntity;

    int clientNumber = scriptEntity->getPropertyInt("clientNumber");
    Logging::log(Logging::DEBUG, "(a) clientNumber: %d\r\n", clientNumber);

    #ifdef CLIENT
        Logging::log(Logging::DEBUG, "client numbers: %d, %d\r\n", ClientSystem::playerNumber, clientNumber);

        if (uniqueId == ClientSystem::uniqueId)
        {
            scriptEntity->setProperty("clientNumber", ClientSystem::playerNumber);
        }
    #endif

    Logging::log(Logging::DEBUG, "(b) clientNumber: %d\r\n", clientNumber);

    assert(clientNumber >= 0);

    #ifdef CLIENT
    // If this is the player. There should already have been created an fpsent for this client,
    // which we can fetch with the valid client #
    Logging::log(Logging::DEBUG, "UIDS: in ClientSystem %d, and given to us%d\r\n", ClientSystem::uniqueId, uniqueId);

    if (uniqueId == ClientSystem::uniqueId)
    {
        Logging::log(Logging::DEBUG, "This is the player, use existing clientnumber for fpsent (should use player1?) \r\n");

        fpsEntity = dynamic_cast<fpsent*>( FPSClientInterface::getPlayerByNumber(clientNumber) );

        // Wipe clean the uniqueId set for the fpsent, so we can re-use it.
        fpsEntity->uniqueId = -77;
    }
    else
    #endif
    {
        Logging::log(Logging::DEBUG, "This is a remote client or NPC, do a newClient for the fpsent\r\n");

        // This is another client, perhaps NPC. Connect this new client using newClient
        fpsEntity =  dynamic_cast<fpsent*>( FPSClientInterface::newClient(clientNumber) );
    }

    // Register with the C++ system.

    LogicSystem::setUniqueId(fpsEntity, uniqueId);
    LogicSystem::registerLogicEntity(fpsEntity);
}

void LogicSystem::setupNonSauer(ScriptValuePtr scriptEntity)
{
    int uniqueId = scriptEntity->getPropertyInt("uniqueId");

    Logging::log(Logging::DEBUG, "setupNonSauer: %d\r\n", uniqueId);
    INDENT_LOG(Logging::DEBUG);

    LogicSystem::registerLogicEntityNonSauer(uniqueId);
}

void LogicSystem::dismantleExtent(ScriptValuePtr scriptEntity)
{
    int uniqueId = scriptEntity->getPropertyInt("uniqueId");

    Logging::log(Logging::DEBUG, "Dismantle extent: %d\r\n", uniqueId);

    extentity* extent = getLogicEntity(uniqueId)->staticEntity;

    removeentity(extent);
    extent->type = ET_EMPTY;

//    entities::deleteentity(extent); extent = NULL; // For symmetry with the newentity() this should be here, but sauer does it
                                                     // in clearents() in the next load_world.
}

void LogicSystem::dismantleCharacter(ScriptValuePtr scriptEntity)
{
    int clientNumber = scriptEntity->getPropertyInt("clientNumber");
    #ifdef CLIENT
    if (clientNumber == ClientSystem::playerNumber)
        Logging::log(Logging::DEBUG, "Not dismantling own client\r\n", clientNumber);
    else
    #endif
    {
        Logging::log(Logging::DEBUG, "Dismantling other client %d\r\n", clientNumber);

#ifdef SERVER
        fpsent* fpsEntity = dynamic_cast<fpsent*>( FPSClientInterface::getPlayerByNumber(clientNumber) );
        bool isNPC = fpsEntity->serverControlled;
#endif

        FPSClientInterface::clientDisconnected(clientNumber);

#ifdef SERVER
        if (isNPC)
            NPC::remove(clientNumber); // The server connections of NPCs are removed when they are dismantled -
                                       // they must be re-created manually in the new scenario, unlike players
#endif
    }
}

