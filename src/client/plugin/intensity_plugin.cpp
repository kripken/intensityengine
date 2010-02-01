
/*
 *=============================================================================
 * Copyright (C) 2010 Alon Zakai ('Kripken') kripkensteiner@gmail.com
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


#include "intensity_plugin.h"
#include "intensity_plugin_listener.h"

#include "base/file_util.h"
#include "base/message_loop.h"



using namespace boost;

#define TO_STRING(type)                  \
std::string _toString(type val)          \
{                                        \
    std::stringstream ss;                \
    std::string ret;                     \
    ss << val;                           \
    return ss.str();                     \
}

TO_STRING(int)
TO_STRING(long)
TO_STRING(double)

ServerChannel *channel;

bool PluginObject::setWindow(NPWindow *window)
{
    printf("setWindow\r\n");

    if (!initialized)
        setupComm();

    printf("update window\r\n");

    savedWindow = window;

    printf("SetWindow: %d, %d\r\n", window->width, window->height);
    std::string message = "setwindow|" + _toString((int)window->width) + "|" + _toString((int)window->height);
    channel->write(message);

    printf("Sending: %s (%d)\r\n", message.c_str(), message.size());

    if (!initialized)
    {
        initialize(window);
        initialized = true;
    }

    return true;
}

void PluginObject::initialize(NPWindow *window)
{
    printf("initialize\r\n");

    // SDL_WINDOWID hack
   	char* buffer = new char[1000];
    sprintf(buffer, "SDL_WINDOWID=%lu",(unsigned long)(window->window));
    putenv(buffer);
    printf("env: %s\r\n", buffer);

    // Start engine in other process
    printf("Launching process in %s\r\n", INTENSITY_INSTALL_ROOT);
    file_util::SetCurrentDirectory(FilePath(INTENSITY_INSTALL_ROOT));
    std::vector<std::string> argv;
    #ifdef LINUX
        argv.push_back("./intensity_client.sh");
    #else
        argv.push_back("intensity_client.bat");
    #endif
    argv.push_back("-P"); // Tell child process it should talk to us
    base::LaunchApp(CommandLine(argv), false, false, &processHandle);

    delete[] buffer;
}

void PluginObject::setupComm()
{
    printf("setupComm\r\n");
    channel = new ServerChannel();
}

