
// Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
// This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

#include "cube.h"
#include "engine.h"
#include "game.h"

#include "fpsclient_interface.h"


int FPSClientInterface::numDynamicEntities()
{
    return game::numdynents();
}

dynent *FPSClientInterface::iterDynamicEntities(int i)
{
    return game::iterdynents(i);
}

extentity *FPSClientInterface::getEntity(int index)
{
    return entities::getents()[index];
}

dynent* FPSClientInterface::newClient(int clientNumber)
{
    return game::newclient(clientNumber);
}

dynent* FPSClientInterface::getPlayerByNumber(int clientNumber)
{
    return game::getclient(clientNumber);
}

int FPSClientInterface::getNumPlayers()
{
    return game::players.length();
}

dynent* FPSClientInterface::getClientPlayer()
{
#ifdef CLIENT
    return game::player1;
#else
    assert(0);
	return NULL;
#endif
}

void FPSClientInterface::clientDisconnected(int clientNumber)
{
    game::clientdisconnected(clientNumber);
}

void FPSClientInterface::spawnPlayer(dynent* player)
{
    game::spawnplayer(dynamic_cast<fpsent*>(player));
}

void FPSClientInterface::shootV(int gun, vec &from, vec &to, dynent *d, bool local, boost::python::object onHit)
{
assert(0);
//    game::ws.shootv(gun, from, to, dynamic_cast<fpsent*>(d), local, onHit);
}

#ifdef CLIENT
void FPSClientInterface::spawnDebris(int type, vec& v, int numdebris, vec& debrisvel, dynent* owner)
{
assert(0);
//    game::ws.spawn_debris(type, v, numdebris, debrisvel, owner);
}
#endif

namespace game
{
    extern void updatepos(fpsent *d);
}

void FPSClientInterface::updatePosition(fpsent *d)
{
    game::updatepos(d);
}

int FPSClientInterface::smoothmove()
{
    return game::smoothmove;
}

int FPSClientInterface::smoothdist()
{
    return game::smoothdist;
}

