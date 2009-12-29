
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

#include "asset.h"

//! Implements our network protocol for asset management
class IntensityAssetManager : public AssetManager
{
protected:
    virtual void requestAssetInfo(const AssetId& id);

    virtual void requestAsset(const AssetId& id);

    virtual AssetHash getLocalAssetHash(const AssetId& id);

public:
    IntensityAssetManager(AssetType _assetType) : AssetManager(_assetType) { };

    //! The shortpath is assumed to contain the path underneath the base asset path
    //!< (which is "/packages" currently). We append the shortpath
    //!< to the base asset path (which can differ on client and server).
    virtual std::string getLocalAssetPath(const AssetId& id);

    //! On the server, we directly access files under /packages, no need to register
    virtual const AssetInfo getAssetInfo(const AssetId& id);
};

//! Trivial action that does nothing. Useful to just get the file from the server.
class EmptyAssetAction : public AssetAction
{
    virtual void act(std::string path, float secondsElapsed) { };
};

//! A manager that does nothing but get raw files
class RawFileManager : public IntensityAssetManager
{
public:
    RawFileManager() : IntensityAssetManager(AssetManager::RAWFILE) { };
};

//! Loads and runs a mapfile
class MapfileLoadAction : public AssetAction
{
    //! Whether we already acted to acquire our subfiles
    bool acted;
public:
    MapfileLoadAction(const AssetId& _assetId) : AssetAction(_assetId), acted(false) { };

    //! The first time this is called (we use 'acted' to ensure singularity), we set up
    //! SubfileActions for our subfiles, so that they will arrive and we will know when
    //! they do.
    virtual bool shouldAbandon(float secondsElapsed);

    virtual void act(std::string path, float secondsElapsed);

    void subfileNotification();
};

//! An action that notifies a 'parent' asset when it arrives. Used in Mapfiles, which
//! consist of two 'subfiles', .ogz and .cfg.
//! Notifying the parent is done by calling it's subfileNotification function.
class SubfileAction : public AssetAction
{
    MapfileLoadAction* parent;
public:
    SubfileAction(const AssetId& _assetId, MapfileLoadAction* _parent) : AssetAction(_assetId), parent(_parent) { };

    virtual void act(std::string path, float secondsElapsed);
};

//! Manages mapfiles, i.e., combinations of .ogz and .cfg files.
//! A path to a mapfile is the prefix, everything but .ogz/.cfg
//!
//! TODO: Consider abstracting out the concept of 'subfiles'/
//! 'subresources', it appears this concept might be used
//! elsewhere eventually.
class MapfileManager : public IntensityAssetManager
{
protected:
    //! Requests asset info for the .ogz and .cfg files
    virtual void requestAssetInfo(const AssetId& id);

    //! Requests the .ogz and .cfg files
    virtual void requestAsset(const AssetId& id);

    //! Concatenates the hashes of the .ogz and .cfg files
    virtual AssetHash getLocalAssetHash(const AssetId& id);

    //! Checks for the presence of both .ogz and .cfg files
    virtual bool checkLocalAssetPresence(const AssetId& id, bool takeAction=false);

public:
    MapfileManager() : IntensityAssetManager(AssetManager::MAPFILE) { };

    //! Our subfiles are the .ogz and .cfg, and we need to add /base to them to get full identifiers/paths
    //! This is used by MapfileLoadAction
    static std::string getSubfileName(std::string name);
};

