
// Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
// This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

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
    ~IntensityPluginObject();
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

