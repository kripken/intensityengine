
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

