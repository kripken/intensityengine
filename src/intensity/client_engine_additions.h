
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


#include <vector>
#include <map>

// 'Sauer-C'-style looping
#define loopstdv(vec) for (unsigned int i = 0; i < vec.size(); i++)

// XXX This should always be higher than the values in iengine for other DECALS_ - check when sauer is updated!
#define DECAL_CIRCLE 3

// Camera stuff

extern void inc_camera();
extern void dec_camera();

//! Utilities for control of the camera

struct CameraControl
{
    //! How much the camera moves per iteration, as per sauer's system. Each frame, the camera is moved so-and-so iterations
    //! to be behind the PC, but only until it hits a wall
    static int cameraMoveDist;
    static int cameraMoveIters; //! How many iterations are used by the camera, each of cameraMoveDist size

    // Increments or decrements the distance of the camera from the PC (to zoom in or out on the PC)
    static void incrementCameraDist(int inc_dir);

    //! During character viewing, the camera spins around the PC, who starts by looking directly at us,
    //! and movement is not allowed. Useful for appearance changing,
    //! character creation, etc.
    static void prepareCharacterViewing();
    static void stopCharacterViewing();

    //! Forces the camera position for the next frame
    static void forceCamera(vec& position, float yaw, float pitch, float roll);

    //! Return the current camera
    static physent* getCamera();

    //! Allow for smooth camera movement, etc.
    static void positionCamera(physent* camera1);
};


//! GUI control utilities. Possibly worth merging into CameraControl

struct GuiControl
{
    //! When mouselooking, the mouse controls orientation as in an FPS
    static void toggleMouselook();
    static bool isMouselooking();

    //! See CameraControl
    static bool isCharacterViewing();
    static void toggleCharacterViewing();

    //! Key clicks
    static void menuKeyClickTrigger();

    // LogicEntity editing GUI
    struct EditedEntity
    {
        static LogicEntityPtr currEntity;
        typedef std::map< std::string, std::pair<std::string, std::string> > StateDataMap; // key -> gui_name, value
        static StateDataMap stateData;
        static std::vector<std::string> sortedKeys;
//        static unsigned int currStateDataIndex;
    };
};


//! Player input control and management

struct PlayerControl
{
    //! Keyboard controls not in sauer: turn left/right, look up/down
    static void handleExtraPlayerMovements(int millis);

    //! Main routine to handle a keypress. If we don't handle it, main() sends it to Sauer.
    static bool handleKeypress(SDLKey sym, int unicode, bool isdown);

    //! Main routine to handle a mouse click. If we don't handle it, main() sends it to Sauer.
    static bool handleClick(int button, bool up);

    //! Erases all the actions from the player's queue. Useful when cancel was pressed to stop actions, or a new action
    //! was decided upon, etc.
    static void flushActions();

    //! Shows the main menu (with quit etc. options)
    static void toggleMainMenu();
};


//! Management of dynamic lights

namespace LightControl
{
    // Adds an appropriately-looking light on the entity on which the mouse cursor is hovering
    extern void addHoverLight();

    extern void queueDynamicLight(const vec &o, float radius, const vec &color, int fade, int peak, int flags, float initradius, const vec &initcolor, physent *owner);

    extern void showQueuedDynamicLights();
};


//! Additional rendering capabilities above Sauer's

struct ExtraRendering
{
    //! For mapmodels that we want dynamic shadows - the same as under the characters - we do not
    //! render them in the usual path. We still utilize PVS and octaentity selection, but we lose
    //! hardware occlusion testing since the rendering is moved out, and it is moved out so that
    //! these mapmodels are rendered in the shadow pass, so that they cast shadows.
    //!
    //! These mapmodels are queued in renderva.cpp, and rendered in fpsrender.h
    //!
    //! Examples of mapmodels that might cast
    //! dynamic shadows are levels, switches, windmills, flags, etc. etc.
    static std::vector<extentity*> currShadowingMapmodels;

    //! Render the mapmodels that need to cast dynamic shadows
    static void renderShadowingMapmodels();
};

