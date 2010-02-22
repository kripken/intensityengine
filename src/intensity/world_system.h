
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


// Various utilities for Sauer's world system
struct WorldSystem
{
    static bool loadingWorld;
    static void placeInWorld(int entityUniqueId, int locationEntityUniqueId);

    static void triggerCollide(LogicEntityPtr mapmodel, physent* d, bool ellipse);

    static void setNumExpectedEntities(int num);
    static void triggerReceivedEntity();

    //! Runs the startup script for the current map. Called from worldio.loadworld
    static void runMapScript();

    //! Used inside sauer to know if we are checking area triggering
    static bool triggeringCollisions;

    //! Check for triggering collisions, i.e., to run trigger events on AreaTriggers
    static void checkTriggeringCollisions(LogicEntityPtr entity);
};

