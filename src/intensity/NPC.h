
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

    //! Removes a single NPC
    void remove(int clientNumber);
};

