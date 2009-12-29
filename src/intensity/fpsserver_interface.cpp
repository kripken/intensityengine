
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


//// Useful macro
//clientinfo* getclientinfo(int sender) { return (clientinfo *)getinfo(sender); };

int FPSServerInterface::getNumClients()
{
    return getnumclients();
}

namespace server
{
    extern std::string& getUsername(int clientNumber);
    extern int& getUniqueId(int clientNumber);
    extern void cleanworldstate(ENetPacket *packet);
}

std::string& FPSServerInterface::getUsername(int clientNumber)
{
    return server::getUsername(clientNumber);
}

int& FPSServerInterface::getUniqueId(int clientNumber)
{
    return server::getUniqueId(clientNumber);
}

void FPSServerInterface::changeMap(std::string name)
{
    game::changemap(name.c_str(), 1);
}

void FPSServerInterface::cleanWorldState(ENetPacket *packet)
{
    server::cleanworldstate(packet);
}

