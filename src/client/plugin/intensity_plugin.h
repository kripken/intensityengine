
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

#include "intensity_plugin_listener.h"

class IntensityPluginObject
{
    bool initialized;
    base::ProcessHandle processHandle;
    NPWindow *window_;
    NPP npp;
    double lastMouseMove;
    ServerChannel *channelIn, *channelOut;

public:
    IntensityPluginObject(NPP npp_) : initialized(false), window_(NULL), npp(npp_), lastMouseMove(0),
        channelIn(NULL), channelOut(NULL) { };
    bool setWindow(NPWindow *window);
    void onMouseMove(double x, double y);
    void onMouseButton(int button, bool down);
    void onKeyboard(int key, int unicode, bool down, bool isRepeat);
    std::string browserCommunicate(std::string data);
    void frameTrigger();

protected:
    void initialize(NPWindow *window);
    void setupComm();
};

