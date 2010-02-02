
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


#include "intensity_plugin_listener.h"

#include "intensity_gui.h"


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
        while (true)
        {
            std::vector<std::string> parsed = channel->readParsed();
            if (parsed.size() == 0) break;

            std::string command = parsed[0];
            printf("Processing command: %s\r\n", command.c_str());

            if (command == "sw")
            {
                assert(parsed.size() == 3);
                int width = atoi(parsed[1].c_str());
                int height = atoi(parsed[2].c_str());
                printf("    %d,%d\r\n", width, height);
                screenres(&width, &height);
            } else if (command == "mm")
            {
                assert(parsed.size() == 3);
                double x = atof(parsed[1].c_str());
                double y = atof(parsed[2].c_str());
                printf("    %f,%f\r\n", x, y);
                IntensityGUI::injectMousePosition(x, y, true);
            } else if (command == "mb")
            {
                assert(parsed.size() == 3);
                int button = atoi(parsed[1].c_str());
                bool down = atoi(parsed[2].c_str());
                printf("    %d,%d\r\n", button, down);
                IntensityGUI::injectMouseClick(button, down);
            } else {
                assert(0);
            }
        }
    }
}

}

