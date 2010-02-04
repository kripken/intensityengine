
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
#include "utility.h"
#include "script_engine_manager.h"
#include "editing_system.h"
#include "message_system.h"

#include "intensity_gui.h"


extern float cursorx, cursory;

namespace EditingSystem
{
    extern std::vector<std::string> entityClasses;
}

namespace IntensityGUI
{
int keyRepeatDelay, keyRepeatInterval;

void setKeyRepeat(int delay, int interval)
{
    keyRepeatDelay = delay;
    keyRepeatInterval = interval;
    SDL_EnableKeyRepeat(keyRepeatDelay, keyRepeatInterval);
}

SVAR(message_title, "");
SVAR(message_content, "");
void showCubeGuiMessage(std::string title, std::string content)
{
    setsvar("message_title", title.c_str());
    setsvar("message_content", content.c_str());
    showgui("message");
}

void showMessage(std::string title, std::string content, int originClientNumber)
{
    if (title == "")
        conoutf(content.c_str());
    else
    {
//        if (originClientNumber == ClientSystem::playerNumber)
//            content = "Me: " + content;

        showCubeGuiMessage(title, content);
    } 
}

SVAR(input_title, "");
SVAR(input_content, "");
SVAR(input_data, "");
void showInputDialog(std::string title, std::string content)
{
    setsvar("input_title", title.c_str());
    setsvar("input_content", content.c_str());
    setsvar("input_data", "");
    showgui("input_dialog");
}

void input_callback(char *input)
{
    ScriptEngineManager::getGlobal()->getProperty("UserInterface")->call("inputDialogCallback", input);
}

COMMAND(input_callback, "s");


// Entity classes dialog support

ICOMMAND(getentityclass, "i", (int *index), {
    std::string ret = EditingSystem::entityClasses[*index];
    assert( Utility::validateAlphaNumeric(ret, "_") ); // Prevent injections
    result(ret.c_str());
});



#if 0
    std::string command = "newgui instances [\n";

    int numInstances = boost::python::extract<int>(instances.attr("__len__")());

    for (int i = 0; i < numInstances; i++)
    {
        boost::python::object instance = instances[i];
        std::string instance_id = boost::python::extract<std::string>(instance.attr("__getitem__")("instance_id"));
        std::string event_name = boost::python::extract<std::string>(instance.attr("__getitem__")("event_name"));

        command += "    guibutton \"" + event_name + "\" \"connect_to_instance " + instance_id + "\"\n";
    }

    command += "    guibar\n";
    command += "    guibutton \"back\"  [ cleargui 1 ]\n";
    command += "]\n";
#endif



    bool canQuit()
    {
        if ( !EditingSystem::madeChanges )
            return true;

        // Changes were made, show a warning dialog
        showgui("can_quit");
        return false;
    }

    void injectMousePosition(float x, float y, bool immediate)
    {
        if (immediate)
        {
            float curr_x, curr_y;
            g3d_cursorpos(curr_x, curr_y);
            float xrel = x - curr_x;
            float yrel = y - curr_y;
            xrel *= max(screen->w, screen->h);
            yrel *= max(screen->w, screen->h);
//printf("curr: %f, %f\r\n", curr_x, curr_y);
//printf("next: %f, %f\r\n", x, y);
//printf("RELS: %f, %f       \r\n", xrel, yrel);
            if(!g3d_movecursor(0, 0))
            {
                mousemove(xrel, yrel);
                SDL_WarpMouse(screen->w / 2, screen->h / 2);
            }
            cursorx = x;
            cursory = y;
            return;
        }

/*
            float curr_x, curr_y;
            g3d_cursorpos(curr_x, curr_y);
            float xrel = x - curr_x;
            float yrel = y - curr_y;
            xrel *= 1000;
            yrel *= 1000;
printf("rels: %f, %f        %f,%f\r\n", xrel, yrel, x, curr_x);
            if(!g3d_movecursor(xrel, yrel))
                mousemove(xrel, yrel);
*/

        g3d_resetcursor(); // now at 0.5,0.5
        float curr_x, curr_y;
        float factor = 400;
        int iters = 0;
        do
        {
            g3d_cursorpos(curr_x, curr_y);
            //printf("A %d : (%f,%f) vs (%f,%f): %f,%f\r\n", iters, x, y, curr_x, curr_y, factor*(x - curr_x), factor*(y - curr_y));
            g3d_movecursor(factor*(x - curr_x), factor*(y - curr_y));
            iters++;
            //printf("B %d : (%f,%f) vs (%f,%f)\r\n", iters, x, y, curr_x, curr_y);
        } while (fabs(x-curr_x) + fabs(y-curr_y) > 0.005 && iters < 1000);
        assert(iters < 1000);
    }

    void injectMouseClick(int button, bool down)
    {
        SDL_Event event;
        event.type = down ? SDL_MOUSEBUTTONDOWN : SDL_MOUSEBUTTONUP;
        event.button.button = button;
        event.button.state = down;
        pushevent(event);
    }

    void injectKeyPress(int sym, int unicode, bool down, bool isRepeat)
    {
        if (isRepeat && keyRepeatDelay == 0) return; // Suppress repeat

        SDL_Event event;
        event.type = down ? SDL_KEYDOWN : SDL_KEYUP;
        event.key.keysym.sym = (SDLKey)sym;
        event.key.state = down ? SDL_PRESSED : !SDL_PRESSED;
        event.key.keysym.unicode = unicode;
        pushevent(event);
    }
}


// Private edit mode stuff

void request_private_edit_mode()
{
    MessageSystem::send_RequestPrivateEditMode();
}

COMMAND(request_private_edit_mode, "");

void private_edit_mode()
{
    intret(ClientSystem::editingAlone);
}

COMMAND(private_edit_mode, "");

