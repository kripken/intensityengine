
// Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
// This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

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

