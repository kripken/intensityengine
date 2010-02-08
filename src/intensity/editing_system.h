
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


//! Additions and management for Sauer's remarkable and unique built-in editing system

namespace EditingSystem
{
    //! Whether this user made editing changes on the current map being run. If so, then a
    //! warning dialog should be shown before exiting or running another map, so that those
    //! changes are not lost.
    extern bool madeChanges;

    bool validateEntityClass(std::string _class);

    //! We queue entity creation commands, as we await a uniqueID from the server. That is,
    //! newent doesn't create them immediately, first we send a request to the server. In
    //! response to that request, we do finalizeQueuedEntity with the uniqueId. TODO:
    //! show some animation while user waits for server response, and prevent multiple
    //! queuing at the same time.
    void newEntity(std::string _class, std::string stateData="");

    #ifdef CLIENT
        struct QueuedEntity
        {
            static std::string _class;
        };
    #endif

    int getWorldSize();

    void eraseGeometry();

    void createCube(int x, int y, int z, int gridsize);

    void deleteCube(int x, int y, int z, int gridsize);

    void setCubeTexture(int x, int y, int z, int gridsize, int face, int texture);

    void setCubeMaterial(int x, int y, int z, int gridsize, int material);

    //! @param corner We use a simpler indexing than Sauer. For NEWS faces, when looking
    //! at the face head on, we have 0 1 / 2 3. For the top and bottom, 0 is toward
    //! the origin, and 1 is along the x axis (then 2 3 on the other row).
    //! @param direction 1 is into the cube, -1 is to the outside.
    void pushCubeCorner(int x, int y, int z, int gridsize, int face, int corner, int direction);

    void createMapFromRaw(int resolution, double addr, int smoothing);

    void createHeightmapFromRaw(int resolution, double addr);

    LogicEntityPtr getSelectedEntity();
};

