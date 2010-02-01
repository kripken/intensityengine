
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


#include "python_wrap.h"

#include "intensity_plugin_listener.h"


using namespace boost;

extern void screenres(int *w, int *h);

namespace PluginListener
{

ClientChannel *channel;

void setupComm()
{
    channel = new ClientChannel();
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
    {
        std::string message = channel->read();
        if (message == "") return;

        printf("Seeing: %s\r\n", message.c_str());

        python::object split = python::object(message).attr("split")("|");
        std::string command = python::extract<std::string>(split[0]);
        printf("Processing command: %s\r\n", command.c_str());

        std::string temp;

        if (command == "setwindow")
        {
            temp = python::extract<std::string>(split[1]);
            int width = atoi(temp.c_str());
            temp = python::extract<std::string>(split[2]);
            int height = atoi(temp.c_str());
            printf("    %d,%d\r\n", width, height);
            screenres(&width, &height);
        } else {
            assert(0);
        }
    }
}

}

