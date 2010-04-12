
// Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
// This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

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

