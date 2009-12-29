
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


//! System utilities for the server (and not the client)


#include "fpsserver_interface.h"


//! We need a single 'dummy' client on the server, to which we relay position and message updates, as if it
//! were a real client.

#define DUMMY_SINGLETON_CLIENT_UNIQUE_ID -9000


//! System management for the serverside, and the connection between the Sauer system and the Python server.

struct ServerSystem
{
    //! Start a new map - create empty files serverside, then send to client
    static void newMap(std::string name);

    //! Set the current map to a specific one: prepare the map, then send it to all clients
    static void setMap(std::string name);

    //! Import a map that the user saves. 'path'
    static void importClientMap(std::string prefix /*!< path to the file, all but the extensions (.ogz, etc.etc.) */ ,
                                int updatingClientNumber );

    //! The client runs octarender::getcubeverts, which sets up cubeext's visible field, which is *critical* for physics.
    //! I.e. the rendering is tangled with physics in that sense. So, for the headless server, we do just that bit right here.
    //! If this is not called, then e.g. after editing some cubes, or just loading a map, you will see NPCs fall into the
    //! ground.
    static void generatePhysicsVisibilities();

    static void fatalMessageToClients(std::string message);

    static bool isRunningMap();
};

