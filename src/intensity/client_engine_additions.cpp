
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

#include "client_system.h"
#include "script_engine_manager.h"
#include "client_engine_additions.h"
#include "utility.h"
#include "targeting.h"
#include "message_system.h"


//=========================
// Camera stuff
//=========================

#define MIN_CAMERA_MOVE_ITERS 8
VAR(CameraControl::cameraMoveDist, 5, 10, 200);                 // Distance camera moves per iteration
//VAR(CameraControl::cameraMoveIters, MIN_CAMERA_MOVE_ITERS, 14, 18); // Number of iterations to move camera DEPRECATED

VAR(cam_dist, 0, 50, 200);

void CameraControl::incrementCameraDist(int inc_dir)
{
    Logging::log(Logging::DEBUG, "changing camera increment: %d\r\n", inc_dir);

    cam_dist += (inc_dir * CameraControl::cameraMoveDist);

    if (ScriptEngineManager::hasEngine())
        ScriptEngineManager::getGlobal()->getProperty("Global")->setProperty("cameraDistance", cam_dist);
}

void inc_camera()
    { CameraControl::incrementCameraDist (+1); };
void dec_camera()
    { CameraControl::incrementCameraDist (-1); };

COMMAND(inc_camera, "");
COMMAND(dec_camera, "");


int saved_cam_dist; // Saved from before characterviewing, restored right after

void CameraControl::prepareCharacterViewing()
{
    player->pitch  = 0;
    camera1->pitch = 0;
    camera1->yaw   = camera1->yaw;

    saved_cam_dist = cam_dist;
    cam_dist = MIN_CAMERA_MOVE_ITERS*3;
}

void CameraControl::stopCharacterViewing()
{
    cam_dist = saved_cam_dist;
}

FVARP(cameraheight, 0, 10, 50); // How much higher than the player to set the camera
FVAR(smoothcamera, 0, 0.2, 100.0); // Smoothing factor for the smooth camera. 0 means no smoothing
FVARP(cameraavoid, 0, 0.33, 1); // 1 means the camera is 100% away from the closest obstacle (and therefore on the player). 0 means it is on that obstacle. 0.5 means it is midway between them.

physent forcedCamera;
bool useForcedCamera = false;
float forcedCameraFov = -1;
int savedThirdperson = -1;

void CameraControl::forceCamera(vec& position, float yaw, float pitch, float roll, float fov)
{
    useForcedCamera = true;
    forcedCamera.o = position;
    forcedCamera.yaw = yaw;
    forcedCamera.pitch = pitch;
    forcedCamera.roll = roll;
    forcedCameraFov = fov;

    // If we just switched to forced camera mode, save thirdperson state and go to third person
    // (We need third person so that we show the player's avatar as the camera moves. There is
    // currently no support for forcing the camera in first person mode, which would be tricky to do.)
    if (!thirdperson)
    {
        savedThirdperson = thirdperson;
        thirdperson = 1;
    }
}

physent* CameraControl::getCamera()
{
    return camera1;
}

void CameraControl::positionCamera(physent* camera1)
{
    Logging::log(Logging::INFO, "CameraControl::positionCamera\r\n");
    INDENT_LOG(Logging::INFO);

    if (useForcedCamera)
    {
        camera1->o = forcedCamera.o;
        camera1->yaw = forcedCamera.yaw;
        camera1->pitch = forcedCamera.pitch;
        camera1->roll = forcedCamera.roll;

        useForcedCamera = false; // Prepare for next frame

        return;
    }

    // Sync camera height to scripts, if necessary
    static float lastCameraHeight = -1;
    if (ScriptEngineManager::hasEngine() && lastCameraHeight != cameraheight)
    {
        lastCameraHeight = cameraheight;
        ScriptEngineManager::getGlobal()->getProperty("Global")->setProperty("cameraHeight", cameraheight);
    }

    // If we just left forced camera mode, restore thirdperson state
    if (savedThirdperson != -1)
    {
        thirdperson = savedThirdperson;
        savedThirdperson = -1;
    }

    float saved_camera_speed = camera1->maxspeed; // Kripken: need to save this, because camera1 =?= player1
    camera1->maxspeed = 50; // This speed determines the distance of the camera, so the Sauer way of tying it to the 
                            // player's speed is not completely general

    vec dir;
    vecfromyawpitch(camera1->yaw, camera1->pitch, -1, 0, dir);

    if (GuiControl::isCharacterViewing())
        camera1->o = player->o; // Start from player

    if(game::collidecamera()) 
    {
        vec cameraOrigin = camera1->o;
        if (thirdperson)
        {
            vec up(0, 0, 1);
            movecamera(camera1, up, cameraheight, 1);
            movecamera(camera1, up, clamp(cameraheight- camera1->o.dist(cameraOrigin), 0.0f, 1.0f), 0.1f); // Find distance to obstacle
        }

        vec cameraOrigin2 = camera1->o;
        movecamera(camera1, dir, cam_dist, 1);
        movecamera(camera1, dir, clamp(cam_dist - camera1->o.dist(cameraOrigin2), 0.0f, 1.0f), 0.1f); // Find distance to obstacle

        if (smoothcamera) {
            float intendedDist = camera1->o.dist(cameraOrigin2)*(1.0f-cameraavoid);
            static float lastDist = 5;
            float ACTUAL_DISTANCE_FACTOR = clamp(1.0f - (curtime/1000.0f)/smoothcamera, 0.0f, 1.0f);
            float actualDist = ACTUAL_DISTANCE_FACTOR*lastDist + (1-ACTUAL_DISTANCE_FACTOR)*intendedDist;

            // Start again, move to current distance
            camera1->o = cameraOrigin2;
            movecamera(camera1, dir, actualDist, 1);
            movecamera(camera1, dir, clamp(actualDist - camera1->o.dist(cameraOrigin2), 0.0f, 1.0f), 0.1f);
            lastDist = actualDist;
        }
    } else {
        camera1->o.z += cameraheight;
        camera1->o.add(vec(dir).mul(cam_dist));
    }

    camera1->maxspeed = saved_camera_speed;

    // Kripken: Smooth camera movement: We interpolate our the new calculated position with the old one, smoothly

    static fpsent actualCamera; // Need fpsent for new normalization functions
    static vec lastPlayerPosition;

    vec temp(actualCamera.o);
    temp.sub(camera1->o);

    actualCamera.normalize_yaw(camera1->yaw);
    actualCamera.normalize_pitch(camera1->pitch);

    float yawDelta = camera1->yaw - actualCamera.yaw;
    float pitchDelta = camera1->pitch - actualCamera.pitch;

    // Only interpolate if we are fairly close, otherwise this might be a new map, or we teleported, etc.
    if (smoothcamera && !GuiControl::isMouselooking() && temp.magnitude() < 50*player->radius && fabs(yawDelta) < 30.0f && fabs(pitchDelta) < 30.0f)
    {
        float ACTUAL_CAMERA_FACTOR = clamp(1.0f - (curtime/1000.0f)/smoothcamera, 0.0f, 1.0f);

        vec temp = player->o;
        temp.sub(lastPlayerPosition);
        actualCamera.o.add(temp); // Prevent camera stutter

        actualCamera.o.mul(ACTUAL_CAMERA_FACTOR);
        temp = camera1->o;
        temp.mul(1-ACTUAL_CAMERA_FACTOR);
        actualCamera.o.add(temp);

        actualCamera.yaw = ACTUAL_CAMERA_FACTOR*actualCamera.yaw + (1-ACTUAL_CAMERA_FACTOR)*camera1->yaw;
        actualCamera.pitch = ACTUAL_CAMERA_FACTOR*actualCamera.pitch + (1-ACTUAL_CAMERA_FACTOR)*camera1->pitch;

        camera1->o = actualCamera.o;
        camera1->yaw = actualCamera.yaw;
        camera1->pitch = actualCamera.pitch;

//        camera1->o.z += player->aboveeye + player->eyeheight;

    } else {
        actualCamera.o = camera1->o;
        actualCamera.yaw = camera1->yaw;
        actualCamera.pitch = camera1->pitch;
    }

    lastPlayerPosition = player->o;
}


//=========================
// GUI stuff
//=========================

bool _isMouselooking = true; // Default like sauer

bool GuiControl::isMouselooking()
    { return _isMouselooking; };


void GuiControl::toggleMouselook()
{
    if (_isMouselooking)
    {
        _isMouselooking = false;

        // Restore cursor to center
        g3d_resetcursor();
    } else {
        _isMouselooking = true;
    };
};

void mouselook() { GuiControl::toggleMouselook(); };

COMMAND(mouselook, "");

bool _isCharacterViewing = false;

bool GuiControl::isCharacterViewing()
    { return _isCharacterViewing; };

void GuiControl::toggleCharacterViewing()
{
    if (!_isCharacterViewing)
        CameraControl::prepareCharacterViewing();
    else
        CameraControl::stopCharacterViewing();

    _isCharacterViewing = !_isCharacterViewing;
}

void characterview() { GuiControl::toggleCharacterViewing(); };

COMMAND(characterview, "");

void GuiControl::menuKeyClickTrigger()
{
    playsound(S_MENUCLICK);
}

void menu_key_click_trigger() { GuiControl::menuKeyClickTrigger(); };

COMMAND(menu_key_click_trigger, "");


// Editing GUI statics
LogicEntityPtr GuiControl::EditedEntity::currEntity;
GuiControl::EditedEntity::StateDataMap GuiControl::EditedEntity::stateData;
std::vector<std::string> GuiControl::EditedEntity::sortedKeys;

SVAR(entity_gui_title, "");

VAR(num_entity_gui_fields, 0, 0, 13);

// Sets up a GUI for editing an entity's state data
void prepare_entity_gui()
{
    GuiControl::EditedEntity::stateData.clear();
    GuiControl::EditedEntity::sortedKeys.clear();

    GuiControl::EditedEntity::currEntity = TargetingControl::targetLogicEntity;
    if (GuiControl::EditedEntity::currEntity->isNone())
    {
        Logging::log(Logging::DEBUG, "No entity to show the GUI for\r\n");
        return;
    }

    int uniqueId = GuiControl::EditedEntity::currEntity->getUniqueId();
    ScriptValuePtr stateData = ScriptEngineManager::runScript(
        "getEntity(" + Utility::toString(uniqueId) + ").createStateDataDict()"
    );

    ScriptValuePtr keys = ScriptEngineManager::getGlobal()->call("keys", stateData);

    num_entity_gui_fields = keys->getPropertyInt("length");

    // Save the stateData, to see what changed and what didn't
    for (int i = 0; i < num_entity_gui_fields; i++)
    {
        std::string key = keys->getPropertyString( Utility::toString(i) );
        std::string guiName = ScriptEngineManager::getGlobal()->call(
            "__getVariableGuiName",
            ScriptValueArgs().append(uniqueId).append(key)
        )->getString();

        std::string value = stateData->getPropertyString(key);

        GuiControl::EditedEntity::stateData.insert(
            GuiControl::EditedEntity::StateDataMap::value_type(
                key,
                std::pair<std::string, std::string>(
                    guiName,
                    value
                )
            )
        );

//        std::string a = python::extract<std::string>( keys[i] );
//        std::string b = python::extract<std::string>( pythonStateData[keys[i]] );

        GuiControl::EditedEntity::sortedKeys.push_back( key );
    }

    sort( GuiControl::EditedEntity::sortedKeys.begin(), GuiControl::EditedEntity::sortedKeys.end() ); // So order is always the same

    for (int i = 0; i < num_entity_gui_fields; i++)
    {
        std::string key = GuiControl::EditedEntity::sortedKeys[i];
        std::string guiName = GuiControl::EditedEntity::stateData[key].first;
        std::string value = GuiControl::EditedEntity::stateData[key].second;

        std::string fieldName = "entity_gui_field_" + Utility::toString(i);
        std::string labelName = "entity_gui_label_" + Utility::toString(i);

        setsvar((char*)fieldName.c_str(), (char*)value.c_str());
        setsvar((char*)labelName.c_str(), (char*)guiName.c_str());
    }

    // Title
    std::string title = GuiControl::EditedEntity::currEntity->scriptEntity->getPropertyString("_class");
    title = Utility::toString(uniqueId) + ": " + title;

    setsvar((char*)"entity_gui_title", (char*)title.c_str());

    // Create the gui
    std::string command =
    "newgui entity [\n"
    "    guitext $entity_gui_title\n"
    "    guibar\n";

    for (int i = 0; i < num_entity_gui_fields; i++)
    {
        std::string sI = Utility::toString(i);
        std::string key = GuiControl::EditedEntity::sortedKeys[i];
        std::string value = GuiControl::EditedEntity::stateData[key].second;

        if (value.size() > 50)
        {
            Logging::log(Logging::WARNING, "Not showing field '%s' as it is overly large for the GUI\r\n", key.c_str());
            continue; // Do not even try to show overly-large items
        }

        command +=
    "    guilist [\n"
    "        guitext (get_entity_gui_label " + sI + ")\n"
    "        new_entity_gui_field_" + sI + " = (get_entity_gui_value " + sI + ")\n"
    "        guifield new_entity_gui_field_" + sI + " " + Utility::toString((int)value.size()+25) + " [set_entity_gui_value " + sI + " $new_entity_gui_field_" + sI + "] 0\n"
    "    ]\n";

        if ((i+1) % 10 == 0)
        {
            command +=
    "   guitab " + Utility::toString(i) + "\n";
        }
    }

    command +=
    "]";

//    printf("Command: %s\r\n", command.c_str());
    execute(command.c_str());
}

COMMAND(prepare_entity_gui, "");

void get_entity_gui_label(int *index)
{
    std::string ret = GuiControl::EditedEntity::stateData[GuiControl::EditedEntity::sortedKeys[*index]].first + ": ";
    result(ret.c_str());
}

COMMAND(get_entity_gui_label, "i");

void get_entity_gui_value(int *index)
{
    std::string ret = GuiControl::EditedEntity::stateData[GuiControl::EditedEntity::sortedKeys[*index]].second;
    result(ret.c_str());
}

COMMAND(get_entity_gui_value, "i");

void set_entity_gui_value(int *index, char *newValue)
{
    std::string key = GuiControl::EditedEntity::sortedKeys[*index];
    std::string oldValue = GuiControl::EditedEntity::stateData[key].second;

    if (oldValue != newValue)
    {
        GuiControl::EditedEntity::stateData[key].second = newValue;

        int uniqueId = GuiControl::EditedEntity::currEntity->getUniqueId();
        std::string naturalValue = ScriptEngineManager::getGlobal()->call("serializeJSON",
            ScriptEngineManager::getGlobal()->call("__getVariable",
                ScriptValueArgs().append(uniqueId).append(key)
            )->call("fromData", newValue)
        )->getString();

        ScriptEngineManager::runScript("getEntity(" + Utility::toString(uniqueId) + ")." + key + " = (" + naturalValue + ");");

/* Old hackish way
        std::string _class = GuiControl::EditedEntity::currEntity.get()->scriptEntity->getPropertyString("_class");

        int protocolId = ScriptEngineManager::getGlobal()->getProperty("MessageSystem")->call("toProtocolId",
            ScriptValueArgs().append(_class).append(key)
        )->getInt();

        MessageSystem::send_StateDataChangeRequest(
            GuiControl::EditedEntity::currEntity.get()->getUniqueId(),
            protocolId,
            newValue // XXX This assumes toWire and toData are the same!
        );
*/
    }
}

COMMAND(set_entity_gui_value, "is");


//
///
//// Physics
///
//

// Extra player movements
bool k_turn_left, k_turn_right, k_look_up, k_look_down;

#define dir(name,v,d,s,os) ICOMMAND(name, "D", (int *down),      \
    if (ClientSystem::scenarioStarted()) \
    { \
        PlayerControl::flushActions(); /* Stop current actions */         \
        s = *down!=0;                                                \
        dynamic_cast<fpsent*>(player)->v = s ? d : (os ? -(d) : 0);  \
    } \
);

dir(turn_left,  turn_move, -1, k_turn_left,  k_turn_right); // New turning motion
dir(turn_right, turn_move, +1, k_turn_right, k_turn_left);  // New pitching motion

dir(look_down, look_updown_move, -1, k_look_down, k_look_up);
dir(look_up,   look_updown_move, +1, k_look_up,   k_look_down);

#define script_dir(name,v,d,s,os) ICOMMAND(name, "D", (int *down),      \
    if (ClientSystem::scenarioStarted()) \
    { \
        PlayerControl::flushActions(); /* Stop current actions */         \
        s = *down!=0;                                                \
        ScriptEngineManager::getGlobal()->getProperty("ApplicationManager")->getProperty("instance")->call( \
            #v, \
            ScriptValueArgs().append(s ? d : (os ? -(d) : 0)).append(s) \
        );  \
    } \
);

//script_dir(turn_left,  performYaw, -1, k_turn_left,  k_turn_right); // New turning motion
//script_dir(turn_right, performYaw, +1, k_turn_right, k_turn_left);  // New pitching motion
// TODO: Enable these. But they do change the protocol (see Character.js), so forces everyone and everything to upgrade
//script_dir(look_down, performPitch, -1, k_look_down, k_look_up);
//script_dir(look_up,   performPitch, +1, k_look_up,   k_look_down);

// Old player movements
script_dir(backward, performMovement, -1, player->k_down,  player->k_up);
script_dir(forward,  performMovement,  1, player->k_up,    player->k_down);
script_dir(left,     performStrafe,    1, player->k_left,  player->k_right);
script_dir(right,    performStrafe,   -1, player->k_right, player->k_left);

ICOMMAND(jump, "D", (int *down), {
  if (ClientSystem::scenarioStarted())
  {
    PlayerControl::flushActions(); /* Stop current actions */         \
    ScriptEngineManager::getGlobal()->getProperty("ApplicationManager")->getProperty("instance")->call("performJump", *down);
  }
});


// Player movements control - keyboard stuff

void PlayerControl::handleExtraPlayerMovements(int millis)
{
    float delta = float(millis)/1000.0f;

    physent *mover;
//    if (GuiControl::isCharacterViewing()) // Buggy. Commenting this out gives a good enough result, actually:
//                                          // keys move *player*, and do mouseMove mode if you want to use the mouse to look around
//        mover = camera1;
//    else
        mover = player;

    // Turn if mouse is at borders

    float x, y;
    g3d_cursorpos(x, y);
    if (g3d_windowhit(true, false)) x = y = 0.5; // Do not scroll with mouse

    // Turning

    fpsent* fpsPlayer = dynamic_cast<fpsent*>(player);

    if (fpsPlayer->turn_move || fabs(x - 0.5) > 0.45)
        mover->yaw +=
            ClientSystem::playerLogicEntity.get()->scriptEntity->getPropertyFloat("facingSpeed")
            * (
                fpsPlayer->turn_move ? fpsPlayer->turn_move : (x > 0.5 ? 1 : -1)
            ) * delta;

    if (fpsPlayer->look_updown_move || fabs(y - 0.5) > 0.45)
        mover->pitch += 
            ClientSystem::playerLogicEntity.get()->scriptEntity->getPropertyFloat("facingSpeed")
            * (
                fpsPlayer->look_updown_move ? fpsPlayer->look_updown_move : (y > 0.5 ? -1 : 1)
            ) * delta;

    extern void fixcamerarange();
    fixcamerarange(); // Normalize and limit the yaw and pitch values to appropriate ranges
}

bool PlayerControl::handleKeypress(SDLKey sym, int unicode, bool isdown)
{
    assert(0);
    return false;
#if 0
    return IntensityCEGUI::handleKeypress(sym, unicode, isdown);
#endif
}

bool PlayerControl::handleClick(int button, bool up)
{
    assert(0);
    return false;
#if 0
    // Start with CEGUI
    if (up ? IntensityCEGUI::handleMousebuttonupEvent(button) : IntensityCEGUI::handleMousebuttondownEvent(button))
        return true;

    // In edit mode, aside from CEGUI, we let Sauer do everything.
    if (editmode)
        return false;

    // Then do our own stuff TODO: Make more generic and modifiable
    switch (button)
    {
        case 4 : // Hack, but there is no SDL notation for this! (Mousewheel up) TODO: Use the sauer keybindings for this
        {
            dec_camera();
            return true;
            break; // ha ha
        }
        case 5 : // Hack, but there is no SDL notation for this! (Mousewheel down)
        {
            inc_camera();
            return true;
            break; // ha ha
        }
        case SDL_BUTTON_LEFT:
        case SDL_BUTTON_MIDDLE:
        case SDL_BUTTON_RIGHT:
        {
            if (!TargetingControl::targetLogicEntity.get()->isNone())
            {
                Logging::log(Logging::DEBUG, "Clicked on entity: %d, %ld\r\n",
                                             TargetingControl::targetLogicEntity.get()->getUniqueId(),
                                             (long)TargetingControl::targetLogicEntity.get());

                // If a click has just completed, and there is a click action for the target, queue it for the player to perform
                if (up) {
                    TargetingControl::targetLogicEntity.get()->scriptEntity->call("performClick", button);
                }
            } else {
                if (up)
                {
                    ScriptEngineManager::getGlobal()->getProperty("ApplicationManager")->getProperty("instance")->call("performClick",
                        ScriptValueArgs().
                            append(button).
                            append( Utility::toString(TargetingControl::worldPosition.x) ).
                            append( Utility::toString(TargetingControl::worldPosition.y) ).
                            append( Utility::toString(TargetingControl::worldPosition.z) )
                    );
                }
            }

            return true; // NEW: We handle all clicks that are not in edit mode and are a button we recognize
                         // OLD: We 'handle' the click even if is just the down part - we don't want Sauer getting that,
                         // at least if this is the _start_ of a real click, i.e., is on an entity.

            break; // ha ha
        }
    }

    return false;
#endif
}

void PlayerControl::flushActions()
{
    ClientSystem::playerLogicEntity.get()->scriptEntity->getProperty("actionSystem")->call("clear");
}

void PlayerControl::toggleMainMenu()
{
    assert(0);
#if 0
    IntensityCEGUI::toggleMainMenu();
#endif
}


//==============================
// Light Control
//==============================

namespace LightControl
{

void addHoverLight()
{
    if (GuiControl::isMouselooking())
        return; // We don't need to mark anything if we are mouselooking. There is no cursor anyhow.

    vec color;

    if (!TargetingControl::targetLogicEntity.get())
    {
        Logging::log(Logging::WARNING, "targetLogicEntity is NULL\r\n");
        return;
    }

    switch (TargetingControl::targetLogicEntity.get()->getType())
    {
        case CLogicEntity::LE_DYNAMIC: color = vec(0.25f, 1.0f, 0.25f);  break;
        case CLogicEntity::LE_STATIC:  color = vec(0.25f, 0.25f, 1.0f);  break;
        case CLogicEntity::LE_NONE:    color = vec(1.0f, 1.0f, 0.5f);
    }

    vec   location;
    float radius;
    bool  needDecal;

    if (!TargetingControl::targetLogicEntity.get()->isNone())
    {    
        location = TargetingControl::targetLogicEntity.get()->getOrigin();
        radius   = TargetingControl::targetLogicEntity.get()->getRadius();
        needDecal = true;
    } else {
        location  = TargetingControl::worldPosition;
        radius    = 0; // 3
        needDecal = false;
    }

    // Add some light to mark the mouse - probably a bad idea for production though though TODO: Consider
    adddynlight(location, radius*2, color);

    if (needDecal)
    {
        // Calculate floor position, and draw a decal there
        vec floorNorm;
        float floorDist = rayfloor(location, floorNorm);
        adddecal(DECAL_CIRCLE, location.sub(vec(0,0,floorDist)), floorNorm, radius);
    }
}

// Queued dynamic lights - to be added for the next frame.
// We should really just create dynamic lights only during the
// weapon::addynlights() code, but this makes writing scripts
// somewhat messier. This approach might lead to lag of 1 frame,
// so livable for now.
struct queuedDynamicLight
{
    vec o;
    float radius;
    vec color;
    int fade, peak, flags;
    float initradius;
    vec initcolor;
    physent *owner;
};

std::vector<queuedDynamicLight> queuedDynamicLights;

void queueDynamicLight(const vec &o, float radius, const vec &color, int fade, int peak, int flags, float initradius, const vec &initcolor, physent *owner)
{
    queuedDynamicLight q;
    q.o = o;
    q.radius = radius;
    q.color = color;
    q.fade = fade;
    q.peak = peak;
    q.flags = flags;
    q.initradius = initradius;
    q.initcolor = initcolor;
    q.owner = owner;
    queuedDynamicLights.push_back(q);
}

void showQueuedDynamicLights()
{
    for (unsigned int i = 0; i < queuedDynamicLights.size(); i++)
    {
        queuedDynamicLight& q = queuedDynamicLights[i];
        adddynlight(q.o, q.radius, q.color, q.fade, q.peak, q.flags, q.initradius, q.initcolor, q.owner);
    }

    queuedDynamicLights.clear();
}

}

// Additional Rendering

std::vector<extentity*> ExtraRendering::currShadowingMapmodels;

void ExtraRendering::renderShadowingMapmodels()
{
    assert(0);
#if 0
    loopstdv(currShadowingMapmodels)
    {
        extentity *mapmodel = currShadowingMapmodels[i];
        model *theModel = LogicSystem::getLogicEntity(*mapmodel).get()->getModel();
        if(!theModel) continue;
        const char *mdlname = theModel->name(); //mapmodelname(mapmodel->attr2);

        int flags = MDL_LIGHT | MDL_CULL_VFC | MDL_CULL_DIST | MDL_CULL_OCCLUDED;

        if (theModel->translucent)
            flags |= MDL_TRANSLUCENT;
        else
            flags |= MDL_SHADOW; // flags |= MDL_DYNSHADOW; ?

        rendermodel(NULL,
                    mdlname,
                    ANIM_MAPMODEL | ANIM_LOOP, // FIXME: Shadowing mapmodels aren't generally per-frame calculated, but who knows,fix this
                    mapmodel->o,
                    LogicSystem::getLogicEntity(*mapmodel),
                    mapmodel->attr1,
                    0,
                    flags);
    }
#endif
}


// Mouse clicks

void mouseclick(int button, bool down)
{
    Logging::log(Logging::INFO, "mouse click: %d (down: %d)\r\n", button, down);

    if (! (ScriptEngineManager::hasEngine() && ClientSystem::scenarioStarted()) )
        return;

    TargetingControl::determineMouseTarget(true); // A click forces us to check for clicking on entities

    vec pos = TargetingControl::targetPosition;
    ScriptValuePtr position = ScriptEngineManager::getGlobal()->call("__new__",
        ScriptValueArgs().append(ScriptEngineManager::getGlobal()->getProperty("Vector3"))
            .append(pos.x)
            .append(pos.y)
            .append(pos.z)
    );

    ScriptValueArgs args;
    args.append(button).append(down).append(position);
    if (TargetingControl::targetLogicEntity.get() && !TargetingControl::targetLogicEntity->isNone())
        args.append(TargetingControl::targetLogicEntity->scriptEntity);
    else
        args.append(ScriptEngineManager::getNull());
    float x, y;
    g3d_cursorpos(x, y);
    args.append(x).append(y);

    ScriptEngineManager::getGlobal()->getProperty("ApplicationManager")->getProperty("instance")->call("performClick", args);
}

ICOMMAND(mouse1click, "D", (int *down), { mouseclick(1, *down!=0); });
ICOMMAND(mouse2click, "D", (int *down), { mouseclick(2, *down!=0); });
ICOMMAND(mouse3click, "D", (int *down), { mouseclick(3, *down!=0); });


// Other client actions - bind these to keys using cubescript (for things like a 'reload' key, 'crouch' key, etc. -
// specific to each game). TODO: Consider overlap with mouse buttons

void actionKey(int index, bool down)
{
    if (ScriptEngineManager::hasEngine())
    {
        ScriptValueArgs args;
        args.append(index).append(down);

        ScriptEngineManager::getGlobal()->getProperty("ApplicationManager")->getProperty("instance")->call("actionKey", args);
    }
}

#define ACTIONKEY(i) ICOMMAND(actionkey##i, "D", (int *down), { actionKey(i, *down!=0); });
ACTIONKEY(0);
ACTIONKEY(1);
ACTIONKEY(2);
ACTIONKEY(3);
ACTIONKEY(4);
ACTIONKEY(5);
ACTIONKEY(6);
ACTIONKEY(7);
ACTIONKEY(8);
ACTIONKEY(9);
ACTIONKEY(10);
ACTIONKEY(11);
ACTIONKEY(12);
ACTIONKEY(13);
ACTIONKEY(14);
ACTIONKEY(15);
ACTIONKEY(16);
ACTIONKEY(17);
ACTIONKEY(18);
ACTIONKEY(19);
ACTIONKEY(20);
ACTIONKEY(21);
ACTIONKEY(22);
ACTIONKEY(23);
ACTIONKEY(24);
ACTIONKEY(25);
ACTIONKEY(26);
ACTIONKEY(27);
ACTIONKEY(28);
ACTIONKEY(29); // 30 action keys should be enough for everybody (TODO: consider speed issues)

