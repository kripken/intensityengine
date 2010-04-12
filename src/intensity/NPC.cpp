
// Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
// This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

#include "cube.h"
#include "engine.h"
#include "game.h"

#include "fpsserver_interface.h"
#include "utility.h"
#include "script_engine_manager.h"

#include "NPC.h"

namespace NPC
{

ScriptValuePtr add(std::string _class)
{
    int clientNumber = localconnect(); // Local connect to the server

    FPSServerInterface::getUsername(clientNumber) = "Bot." + Utility::toString(clientNumber); // Also sets as valid ('logged in')

    Logging::log(Logging::DEBUG, "New NPC with client number: %d\r\n", clientNumber);

    // Create scripting entity (players do this when they log in, NPCs do it here
    return server::createScriptingEntity(clientNumber, _class);
}

void remove(int clientNumber)
{
    localdisconnect(true, clientNumber);
}

};

