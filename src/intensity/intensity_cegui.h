
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


#include "SDL.h"
#include "CEGUI.h"

//! Connect between Sauer and CEGUI

namespace IntensityCEGUI
{
    //! Initialize CEGUI, appropriately for Sauer
    void init();

    //! Called on exit. TODO: use this!
    void quit();

    //! Toggles showing the main menu, typically in response to 'escape'
    void toggleMainMenu();

    //! Draws the CEGUI GUI. Called once/frame
    void drawGui();

    void drawText(std::string str, int left, int top, int r, int g, int b, int a, int cursor, int maxwidth);

    //! Handle a keypress event with CEGUI
    bool handleKeypress(SDLKey sym, int unicode, bool isdown);

    //! Handle a mouse button up event with CEGUI
    bool handleMousebuttonupEvent(Uint8 SDLbutton);

    //! Handle a mouse button down event with CEGUI
    bool handleMousebuttondownEvent(Uint8 SDLbutton);

    void showLayout(std::string layout);


    // LogicEntity editing GUI

    struct EditedEntity
    {
        static LogicEntityPtr currEntity;

        typedef std::map< std::string, std::pair<std::string, std::string> > StateDataMap; // key -> gui_name, value

        static StateDataMap stateData;

        static std::vector<std::string> stateDataKeys;

        static unsigned int currStateDataIndex;
    };

    void showEditEntityGUI();

    struct QueuedMessage
    {
        static std::string title;
        static std::string text;
    };

    void showMessage(std::string title, std::string text, bool chat=false);

    void runLuaFunction(std::string command);
};

