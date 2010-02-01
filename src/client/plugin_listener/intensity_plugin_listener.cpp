
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
#include "base/process.h"
#include "base/process_util.h"
#include "ipc/ipc_channel.h"
#include "ipc/ipc_message.h"

#include "intensity_plugin_listener.h"

#include "base/file_util.h"
#include "base/at_exit.h"
#include "base/message_loop.h"
#include "ipc/ipc_message_utils.h"


using namespace boost;

extern void screenres(int *w, int *h);

namespace PluginListener
{

base::AtExitManager g_at_exit_manager;

interprocess::managed_shared_memory *segment;
MyVector *instance;

void setupComm()
{
    segment = new interprocess::managed_shared_memory(
        interprocess::open_only,
        INTENSITY_CHANNEL
    );

    instance = segment->find<MyVector>("MyVector").first;
    assert(instance);
}

bool initialized = false;

void initialize()
{
    CommandLine::Init(0, NULL);

    InitLogging("listener_debug.log",
                logging::LOG_TO_BOTH_FILE_AND_SYSTEM_DEBUG_LOG,
                logging::DONT_LOCK_LOG_FILE,
                logging::APPEND_TO_OLD_LOG_FILE);

    DLOG(INFO) << "IntensityListener::initialize";

    setupComm();

    initialized = true;
}

void frameTrigger()
{
    if (initialized)
    {
        printf("Seeing: %d\r\n", (*instance)[0]);
    }
}

/*
        const std::string command = iter.NextString();
        printf("Processing command: %s\r\n", command.c_str());
        if (command == "setwindow")
        {
            int width = iter.NextInt();
            int height = iter.NextInt();
            printf("    %d,%d\r\n", width, height);
            screenres(&width, &height);
        } else {
            assert(0);
        }
    }

    virtual void OnChannelError()
        { printf("!Channel error!\r\n"); };
    }
*/

}

