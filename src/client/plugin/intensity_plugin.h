
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


#include "npapi.h"

#include "base/process.h"
#include "base/process_util.h"
#include "ipc/ipc_channel.h"
#include "ipc/ipc_message.h"

#define INTENSITY_CHANNEL "I1"

class PluginObject
{
    class Listener : public IPC::Channel::Listener {
    public:
        virtual void OnMessageReceived(const IPC::Message& message) { };
        virtual void OnChannelError()
            { printf("Channel error!\r\n"); };
    };

    bool initialized;
    base::ProcessHandle processHandle;
    IPC::Channel *channel;
    NPWindow *savedWindow;
    Listener *listener;
public:
    PluginObject() : initialized(false), channel(NULL), savedWindow(NULL),
        listener(NULL) { };
    bool setWindow(NPWindow *window);
protected:
    void initialize(NPWindow *window);
    void setupComm();
};

