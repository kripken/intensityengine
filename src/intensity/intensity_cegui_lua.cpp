
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


#include "algorithm"

#include "pch.h"
#include "engine.h"

#include "CEGUI.h"

#include "client_system.h"
#include "message_system.h"
#include "editing_system.h"

#include "intensity_cegui.h"
#include "intensity_cegui.pkg"
#include "targeting.h"
#include "script_engine_manager.h"


extern bool quittingQueued; // From main.cpp

namespace IntensityCEGUI
{

void luaNewMap()
{
//    execute("edittoggle"); // For now, we are allowed to make new maps not in edit mode
    execute("newmap 1");
}

void luaStartTest()
{
    execute("sp test"); // Load our test map
}

void luaQuit()
{
    quittingQueued = true; // Queue a quit. We cannot quit() here ourselves, as we are inside CEGUI and Lua right now...
}

void luaCharacterView()
{
    GuiControl::toggleCharacterViewing();
}

std::string luaGetUsername()
{
    return ClientSystem::getUsername();
}

std::string luaGetPassword()
{
    return ClientSystem::getVisualPassword();
}

void luaLogin(std::string username, std::string password)
{
    ClientSystem::connect( Utility::Config::getString("Network", "master_server", "NO-MASTER") );
}

// Some appearance editing functions

#define HORNS_TAG           "tag_horns"
#define ARMOR_TAG           "tag_armor"
#define WEAPON_TAG          "tag_weapon"
#define LEFTWEAPON_TAG      "tag_leftweapon"
#define WEAPON_PARTICLE_TAG "tag_weapon_particle"

void luaSetHorns(int id)
{
/*
    if (id == 0)
        GameData::playerPtr.get()->getCharacterInfo()->removeAttachment(HORNS_TAG);
    else
        GameData::playerPtr.get()->getCharacterInfo()->addAttachment(Attachment("mrfixit/horns", HORNS_TAG));
*/
}

/*void luaToggleColor()
{
    // Hmm
}*/

void setAttachment(int id, const char *ids[], const char *tag)
{
/*
    if (id == 0)
        GameData::playerPtr.get()->getCharacterInfo()->removeAttachment(tag);
    else
    {
        if (GameData::playerPtr.get()->getCharacterInfo()->findAttachment(tag) != -1)
            GameData::playerPtr.get()->getCharacterInfo()->removeAttachment(tag);

        GameData::playerPtr.get()->getCharacterInfo()->addAttachment(Attachment(ids[id-1], tag));
    }
*/
}

void luaSetArmor(int id)
{
    static const char *armors[] = { "mrfixit/armor/blue", "mrfixit/armor/green", "mrfixit/armor/yellow", NULL };

    setAttachment(id, armors, ARMOR_TAG);
}

void luaSetRightWeapon(int id)
{
    static const char *weps[] = { "vwep/pistol", "vwep/shotg", "vwep/chaing", "vwep/rocket", "vwep/rifle", "vwep/gl", NULL };

    setAttachment(id, weps, WEAPON_TAG);
}


void luaSetLeftWeapon(int id)
{
    static const char *shields[] = { "vwep/fist", NULL };

    setAttachment(id, shields, LEFTWEAPON_TAG);
}

void luaSetRightWeaponSparkle(int id)
{
    static const char *weps[] = { "particle/sparkly", NULL };

    setAttachment(id, weps, WEAPON_PARTICLE_TAG);
}

void luaToggleEdit()
{
    execute("edittoggle");
}


// Editing

std::string luaGetEditedUniqueId()
{
    if (EditedEntity::currEntity.get()->isNone())
        return "";
    else
        return Utility::toString(EditedEntity::currEntity.get()->getUniqueId());
}

std::string luaGetEditedClass()
{
    if (EditedEntity::currEntity.get()->isNone())
        return "";
    else
        return EditedEntity::currEntity.get()->getClass();
}

std::string lauGetCurrEditedEntityKey()
{
    if (EditedEntity::currStateDataIndex < EditedEntity::stateDataKeys.size())
        return EditedEntity::stateDataKeys[ EditedEntity::currStateDataIndex ];
    else
        return "";
}

std::string lauGetCurrEditedEntityGUIName()
{
    if (EditedEntity::currStateDataIndex < EditedEntity::stateDataKeys.size())
        return EditedEntity::stateData[ EditedEntity::stateDataKeys[ EditedEntity::currStateDataIndex ] ].first;
    else
        return "";
}

std::string lauGetCurrEditedEntityValue()
{
    if (EditedEntity::currStateDataIndex < EditedEntity::stateDataKeys.size())
    {
        std::string ret = EditedEntity::stateData[ EditedEntity::stateDataKeys[ EditedEntity::currStateDataIndex ] ].second;

        EditedEntity::currStateDataIndex += 1;

        return ret;
    } else {
        EditedEntity::currStateDataIndex = 0; // Reset, so this works the next time we start reading

        return "";
    }
}

void luaSetCurrEditedEntityValue(std::string key, std::string value)
{
    if (EditedEntity::currEntity.get()->scriptEntity->getProperty("stateData")->getPropertyString(key) != value)
    {
        // Send directly, the value is already in string form, which should be suitable.
        // XXX However, this assumes that wire form and state data form are the same. A more
        // correct approach is to use the 'from_data' and 'to_wire' converters. TODO: Find
        // an appropriate approach for that (do not want to explicitly write accessing those
        // converters here).

        std::string _class = EditedEntity::currEntity.get()->scriptEntity->getPropertyString("_class");

        int protocolId = ScriptEngineManager::getGlobal()->getProperty("MessageSystem")->call("toProtocolId",
            ScriptValueArgs().append(_class).append(key)
        )->getInt();

        MessageSystem::send_StateDataChangeRequest(
            EditedEntity::currEntity.get()->getUniqueId(),
            protocolId,
            value
        );
    }
}

void luaExecuteSauerCommand(std::string command)
{
    execute(command.c_str());
}

int luaGetSauerVariable(std::string variable)
{
    return getvar(variable.c_str());
}

void luaExecutePythonScript(std::string script)
{
    EXEC_PYTHON(script);
}

int luaEvaluatePythonScriptInteger(std::string script)
{
    return GET_PYTHON<int>(script);
}

float luaEvaluatePythonScriptFloat(std::string script)
{
    return GET_PYTHON<float>(script);
}

std::string luaEvaluatePythonScriptString(std::string script)
{
    return GET_PYTHON<std::string>(script);
}

std::string luaGetQueuedMessageTitle()
{
    return IntensityCEGUI::QueuedMessage::title;
}

std::string luaGetQueuedMessageText()
{
    return IntensityCEGUI::QueuedMessage::text;
}

// Python classes, in convenient list format
std::vector<std::string> classes;

void readClasses()
{
    if (classes.size() != 0)
        return;

    ScriptValuePtr entityClasses = ScriptEngineManager::getGlobal()->call("listEntityClasses");
    int numClasses = entityClasses->getPropertyInt("length");

    for (int i = 0; i < numClasses; i++)
        classes.push_back( entityClasses->getPropertyString(Utility::toString(i)) );

    sort( classes.begin(), classes.end() );
}

std::string lauGetCurrClass()
{
    static unsigned int currIndex = 0;

    readClasses();

    if (currIndex < classes.size())
    {
        currIndex++;
        return classes[currIndex-1];
    } else {
        currIndex = 0;
        return "";
    }
}

void queueEntityCreation(std::string _class)
{
    EditingSystem::QueuedEntity::_class = _class;
}


// Links a class name or tag name to the treeitem basis for all such entities (with that class or tag)
typedef std::map<std::string, CEGUI::TreeItem*> stringTreeItemMap;

void luaPopulateEntityList(CEGUI::Window* entityListWindow)
{
    CEGUI::Tree* theTree = (CEGUI::Tree*)entityListWindow->getChildRecursive("Editing/EntityList/Tree");
//    theTree->initialise();
    theTree->resetList();

    // Create classes, top-level
    readClasses();

    stringTreeItemMap classTreeItems;

    for (unsigned int i = 0; i < classes.size(); i++)
    {
        std::string name = classes[i];

        CEGUI::TreeItem* item = new CEGUI::TreeItem( name );
        item->setAutoDeleted(true);
        item->setTextColours(CEGUI::colour(0.0, 0.0, 0.0));
        theTree->addItem(item);

        classTreeItems.insert( stringTreeItemMap::value_type(name, item) );
    }

    // Insert entities at second level
    for(LogicSystem::LogicEntityMap::iterator iter = LogicSystem::logicEntities.begin();
        iter != LogicSystem::logicEntities.end();
        iter++)
    {
        int uniqueId = iter->first;
        ScriptValuePtr scriptEntity = iter->second.get()->scriptEntity;
        std::string className = scriptEntity->getPropertyString("_class");

        CEGUI::TreeItem* item = new CEGUI::TreeItem( Utility::toString(uniqueId) );
        item->setAutoDeleted(true);
        item->setTextColours(CEGUI::colour(0.0, 0.0, 0.0));
        classTreeItems[className]->addItem(item);
    }

    // Prune classes with 0 entities
    for (unsigned int i = 0; i < classes.size(); i++)
    {
        CEGUI::TreeItem *item = classTreeItems[classes[i]];
        if (item->getItemCount() == 0)
        {
            theTree->removeItem(item);
            // Note: No need to remove from classTreeItems
        }
    }

    // Create tags

    CEGUI::TreeItem* tagsItem = new CEGUI::TreeItem( "__tags__" );
    tagsItem->setAutoDeleted(true);
    tagsItem->setTextColours(CEGUI::colour(0.0, 0.333, 0.0));
    theTree->addItem(tagsItem);

    stringTreeItemMap tagTreeItems;

    for(LogicSystem::LogicEntityMap::iterator iter = LogicSystem::logicEntities.begin();
        iter != LogicSystem::logicEntities.end();
        iter++)
    {
        int uniqueId = iter->first;
        ScriptValuePtr scriptEntity = iter->second.get()->scriptEntity;
        ScriptValuePtr tags = scriptEntity->getProperty("tags");
        int numTags = tags->getPropertyInt("length");

        // Add item under each of its tags, creating the tag category if necessary
        for (int i = 0; i < numTags; i++)
        {
            std::string currTag = tags->getPropertyString( Utility::toString(i) );

            // Add the tag category if needed
            if (tagTreeItems.find(currTag) == tagTreeItems.end())
            {
                CEGUI::TreeItem* item = new CEGUI::TreeItem( currTag );
                item->setAutoDeleted(true);
                item->setTextColours(CEGUI::colour(0.0, 0.0, 0.0));
                tagsItem->addItem(item);

                tagTreeItems.insert( stringTreeItemMap::value_type(currTag, item) );
            }

            // Add the entity to the tag
            CEGUI::TreeItem* item = new CEGUI::TreeItem( Utility::toString(uniqueId) );
            item->setAutoDeleted(true);
            item->setTextColours(CEGUI::colour(0.0, 0.0, 0.0));
            tagTreeItems[currTag]->addItem(item);
        }
    }
}

void luaFocusOnEntity(int uniqueId)
{
    // Orient towards TODO: Move to WorldSystem or such
    LogicEntityPtr entity = LogicSystem::getLogicEntity(uniqueId);
    if (entity.get())
    {
        vec pos(entity.get()->getOrigin());
        pos.sub(player->o);
        vectoyawpitch(pos, player->yaw, player->pitch);

        // Target and Show details window
        TargetingControl::targetLogicEntity = entity;
        showEditEntityGUI();
    } else {
        IntensityCEGUI::showMessage("Manage Entities", "That entity no longer exists", false);
//        luaPopulateEntityList(...) TODO
    }
}

void luaPopulateSettings(CEGUI::Window* settingsWindow)
{
    CEGUI::Slider* slider;

    slider = (CEGUI::Slider*)settingsWindow->getChildRecursive("Settings/Main/Volume");
    int soundvol = getvar("soundvol");
    slider->setCurrentValue(float(soundvol)/255);

    slider = (CEGUI::Slider*)settingsWindow->getChildRecursive("Settings/Main/MusicVolume");
    int musicvol = getvar("musicvol");
    slider->setCurrentValue(float(musicvol)/255);

//printf("luaPopulateSettings: %d,%d\r\n", soundvol, musicvol);
}

void luaApplySoundSettings(CEGUI::Window* settingsWindow)
{
    CEGUI::Slider* slider;
    std::string command;

    slider = (CEGUI::Slider*)settingsWindow->getChildRecursive("Settings/Main/Volume");
    int soundvol = int(slider->getCurrentValue()*255);
    command = "soundvol " + Utility::toString(soundvol);
    execute(command.c_str());

    slider = (CEGUI::Slider*)settingsWindow->getChildRecursive("Settings/Main/MusicVolume");
    int musicvol = int(slider->getCurrentValue()*255);
    command = "musicvol " + Utility::toString(musicvol);
    execute(command.c_str());

//printf("luaApplySoundSettings: %d,%d\r\n", soundvol, musicvol);
}

}

