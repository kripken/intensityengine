
// Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
// This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

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

