
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

#include "base/file_util.h"
#include "base/message_loop.h"


bool PluginObject::setWindow(NPWindow *window)
{
    printf("setWindow\r\n");

    if (!initialized)
    {
        initialize(window);
        initialized = true;
    }

    printf("update window\r\n");

    savedWindow = window;

    printf("SetWindow: %d, %d\r\n", window->width, window->height);
    IPC::Message* message = new IPC::Message(0, 2, IPC::Message::PRIORITY_HIGH);
    message->WriteString("setwindow");
    message->WriteInt(window->width);
    message->WriteInt(window->height);
    channel->Send(message);

    return true;
}

void PluginObject::initialize(NPWindow *window)
{
    printf("initialize\r\n");

    setupComm();

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

    new MessageLoop(MessageLoop::TYPE_IO);
    printf("setupComm 2\r\n");

    listener = new PluginObject::Listener();
    channel = new IPC::Channel(INTENSITY_CHANNEL, IPC::Channel::MODE_SERVER, listener);
    channel->Connect();
    printf("setupComm 3\r\n");

    MessageLoop::current()->RunAllPending();
    printf("setupComm 4\r\n");
}

