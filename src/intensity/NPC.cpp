
/*
 *=============================================================================
 * Copyright (C) 2008 Alon Zakai ('Kripken') kripkensteiner@gmail.com
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

