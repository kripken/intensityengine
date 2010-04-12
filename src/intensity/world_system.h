
// Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
// This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

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

