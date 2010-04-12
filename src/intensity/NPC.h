
// Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
// This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

//! Non-Player Characters, aka Bots. Managed entirely on the server-side;
//! for players, NPCs appear as just other clients (like other players).
namespace NPC
{
    //! Creates a new NPC client, as if another client logged in (using
    //! a localconnect, and then creates a scripting entity for the NPC
    //! so as to enter the game.
    //! @param _class The class of the new NPC
    //! @return The newly created NPC entity
    ScriptValuePtr add(std::string _class);

    //! Removes a single NPC XXX - should not be used
    void remove(int clientNumber);
};

