
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


#include "ipc/ipc_channel.h"
#include "ipc/ipc_message.h"

#include "intensity_plugin.h"

#include "base/file_util.h"
#include "base/message_loop.h"
#include "ipc/ipc_message_utils.h"


extern void screenres(int *w, int *h);

namespace PluginListener
{

#define INTENSITY_CHANNEL "I1"

class Listener : public IPC::Channel::Listener {
public:
    virtual void OnMessageReceived(const IPC::Message& message)
    {
        IPC::MessageIterator iter(message);
        const std::string command = iter.NextString();
        printf("Processing command: %s\r\n", command.c_str());
        if (command == "setwindow")
        {
            int width = iter.NextInt();
            int height = iter.NextInt();

            screenres(&width, &height);
        } else {
            assert(0);
        }
    }
};

Listener *listener;
IPC::Channel *channel;

void setupComm()
{
    printf("setupComm\r\n");

    new MessageLoop(MessageLoop::TYPE_IO);
    printf("setupComm 2\r\n");

    listener = new Listener::Listener();
    channel = new IPC::Channel(INTENSITY_CHANNEL, IPC::Channel::MODE_CLIENT, listener);
    channel->Connect();
    printf("setupComm 3\r\n");
}

bool initialized = false;

void initialize()
{
    setupComm();
    initialized = true;
}

void frameTrigger()
{
    if (initialized)
        MessageLoop::current()->RunAllPending();
}

}

