
// Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
// This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

#include "cube.h"
#include "engine.h"


void render_progress(float amount, std::string text)
{
    renderprogress(amount, text.c_str());
}

bool load_world_oneparam(const char *mname)
{
    return load_world(mname, NULL);
}

