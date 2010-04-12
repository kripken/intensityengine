
// Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
// This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

#include "cube.h"
#include "engine.h"
#include "game.h"

#include "intensity.h"
#include "client_system.h"

#include "character_render.h"


void CharacterRendering::render(fpsent* entity)
{
    Logging::log(Logging::INFO, "CharacterRendering::rendering %d\r\n", LogicSystem::getUniqueId(entity));
    INDENT_LOG(Logging::INFO);

    if (!ClientSystem::loggedIn) // If not logged in remotely, do not render, because entities lack all the fields like model_name
                                 // in the future, perhaps add these, if we want local rendering
    {
        Logging::log(Logging::INFO, "Not logged in remotely, so not rendering\r\n");
        return;
    }

    LogicEntityPtr logicEntity = LogicSystem::getLogicEntity(entity->uniqueId);

    if ( !logicEntity.get() )
    {
        Logging::log(Logging::INFO, "fpsent exists, but no logic entity yet for it, so not rendering\r\n");
        return;
    }

    ScriptValuePtr scriptEntity = logicEntity.get()->scriptEntity;

    if ( !scriptEntity->getPropertyBool("initialized") )
    {
        Logging::log(Logging::INFO, "Not initialized, so not rendering\r\n");
        return;
    }

    // Render client using model name, attachments, and animation information

    Logging::log(Logging::INFO, "Rendering %d, with o=(%f,%f,%f)\r\n", logicEntity.get()->getUniqueId(),
                                                                       logicEntity.get()->getOrigin().x,
                                                                       logicEntity.get()->getOrigin().y,
                                                                       logicEntity.get()->getOrigin().z);

    model* theModel = logicEntity.get()->getModel();
    if (theModel) {
        renderclient(entity,
                     theModel->name(),
                     logicEntity,
                     logicEntity.get()->attachments[0].name ? logicEntity.get()->attachments : NULL,
                     0,
                     logicEntity.get()->getAnimation(),
                     300, // Delay TODO: Figure out what this is
                     entity->lastaction,
                     entity->lastpain);
    } else {
        Logging::log(Logging::INFO, "No model, so no render.\r\n");
    }

    // INTENSITY: Class above head in edit mode
    if (editmode)
    {
        std::string _class = '@' + logicEntity.get()->getClass(); // '@' makes Sauer create a copy
        particle_text(entity->abovehead(), _class.c_str(), 16, 1);
    }

    Logging::log(Logging::INFO, "CharacterRendering::render complete.\r\n");
}


/*

// HACK: FIXME TODO   This is just to prevent warnings, for now
bool qwerty3a = (guns == NULL);
bool qwerty3b = msgsizelookup(0);
// END HACK: FIXME TODO   This is just to prevent warnings, for now


CharacterInfo::CharacterInfo(fpsent* _data) : data(_data)
{
    modelName = (char *)"mrfixit"; // TODO: Variate
};


void CharacterInfo::addAttachment(Attachment attachment)
{
    if (attachments.size() >= MAX_ATTACHMENTS)
    {
        Logging::log(Logging::WARNING,
                     "Attempt to add an attachment beyond the limit of the number of attachment ('%s','%s')\r\n",
                     attachment.name.c_str(), attachment.tag.c_str());
        return;
    }
 
    if (findAttachment(attachment.tag) != -1)
    {
        Logging::log(Logging::WARNING, "Attempt to add an attachment with an existing tag! ('%s')\r\n", attachment.tag.c_str());
        return;
    }

    attachments.push_back(attachment);
}

void CharacterInfo::removeAttachment(std::string tag)
{
    int index = findAttachment(tag);

    if (index == -1)
    {
        Logging::log(Logging::WARNING, "Attempt to remove a non-existing tag! ('%s')\r\n", tag.c_str());
        return;
    }

    attachments.erase(attachments.begin() + index);
}

int CharacterInfo::findAttachment(std::string tag)
{
    loopstdv(attachments)
        if (attachments[i].tag == tag)
            return i;

    return -1;
}
*/
