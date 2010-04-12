
// Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
// This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

#include "pch.h"
#include "engine.h"

#include "intensity_asset.h"
#include "message_system.h"
#include "logging.h"


#ifdef CLIENT
    #include "client_system.h"
#endif


// Low-level utilities

const AssetInfo getIntensityAssetInfo(const AssetId& id, AssetManager* manager)
{
    return AssetInfo(
        manager->getLocalAssetHash(id),
        id
    );
}

AssetHash getIntensityLocalAssetHash(const AssetId& id, AssetManager* manager)
{
    std::string path = manager->getLocalAssetPath(id);

    REFLECT_PYTHON( calculate_file_hash );

    std::string hash = python::extract<std::string>( calculate_file_hash(path) );

    Logging::log(Logging::DEBUG, "Calculated IntensityHash: %s ==== %s\r\n", id.c_str(), hash.c_str());

    return hash;
}

std::string getIntensityLocalAssetPath(const AssetId& id, AssetManager* manager)
{
    // The id is assumed to be the shortpath
    REFLECT_PYTHON_ALTNAME(os.path.join, os_path_join);
    REFLECT_PYTHON(get_asset_dir);

    python::object temp = os_path_join( get_asset_dir(), id );

    return python::extract<std::string>(temp);
}


// Client protocol

void IntensityAssetManager::requestAssetInfo(const AssetId& id)
{
    MessageSystem::send_AssetInfoRequest(assetType, id);
}

void IntensityAssetManager::requestAsset(const AssetId& id)
{
    MessageSystem::send_AssetRequest(assetType, id);
}

// General

AssetHash IntensityAssetManager::getLocalAssetHash(const AssetId& id)
{
    AssetHash hash = getIntensityLocalAssetHash(id, this);
    #ifdef SERVER
        assert(hash != ""); // Asset must exist
    #endif
    return hash;
}

std::string IntensityAssetManager::getLocalAssetPath(const AssetId& id)
{
    return getIntensityLocalAssetPath(id, this);
}

const AssetInfo IntensityAssetManager::getAssetInfo(const AssetId& id)
{
    #ifdef CLIENT
        return AssetManager::getAssetInfo(id); // Normal method
    #else // SERVER
        // Calculate Infos on the fly, no need to register
        return getIntensityAssetInfo(id, this);
    #endif
}


// Actions

void SubfileAction::act(std::string path, float secondsElapsed)
{
    Logging::log(Logging::DEBUG, "SubfileAction::act\r\n");

    parent->subfileNotification();
};

bool MapfileLoadAction::shouldAbandon(float secondsElapsed)
{
    Logging::log(Logging::DEBUG, "Waiting for mapfile... %.1f seconds\r\n", secondsElapsed);
    INDENT_LOG(Logging::DEBUG);

    if (!acted)
    {
        // Queue SubfileActions for both subfiles

        AssetId subId;

        subId = MapfileManager::getSubfileName(assetId + ".cfg");
        AssetManager::getManagerByType(AssetManager::RAWFILE)->actOnAsset(
            subId,
            AssetActionPtr( new SubfileAction(subId, this) )
        );

        subId = MapfileManager::getSubfileName(assetId + ".ogz");
        AssetManager::getManagerByType(AssetManager::RAWFILE)->actOnAsset(
            subId,
            AssetActionPtr( new SubfileAction(subId, this) )
        );

        acted = true; // Do not act twice!
    }

    return false; // Never give up
}

void MapfileLoadAction::act(std::string path, float secondsElapsed)
{
#ifdef CLIENT
    std::string basename = assetId;
    ClientSystem::currMap = basename;
    load_world( ClientSystem::currMap.c_str() );
#endif
}

void MapfileLoadAction::subfileNotification()
{
    Logging::log(Logging::DEBUG, "MapfileLoadAction::subfileNotification\r\n");

    // Hackish way to notice when all subfiles arrive
    AssetManager* manager = AssetManager::getManagerByType(AssetManager::MAPFILE);

    if (manager->checkLocalAssetPresence(assetId))
        manager->triggerAssetReceived(assetId);
}


// MapfileManager

std::string MapfileManager::getSubfileName(std::string name)
{
    return "base/" + name;
}

void MapfileManager::requestAssetInfo(const AssetId& id)
{
    AssetManager::getManagerByType(AssetManager::RAWFILE)->requestAssetInfo(getSubfileName(id + ".cfg"));
    AssetManager::getManagerByType(AssetManager::RAWFILE)->requestAssetInfo(getSubfileName(id + ".ogz"));
}

void MapfileManager::requestAsset(const AssetId& id)
{
    AssetManager::getManagerByType(AssetManager::RAWFILE)->requestAsset(getSubfileName(id + ".cfg"));
    AssetManager::getManagerByType(AssetManager::RAWFILE)->requestAsset(getSubfileName(id + ".ogz"));
}

AssetHash MapfileManager::getLocalAssetHash(const AssetId& id)
{
    return AssetManager::getManagerByType(AssetManager::RAWFILE)->getLocalAssetHash(getSubfileName(id + ".cfg")) +
           AssetManager::getManagerByType(AssetManager::RAWFILE)->getLocalAssetHash(getSubfileName(id + ".ogz"));
}

bool MapfileManager::checkLocalAssetPresence(const AssetId& id, bool takeAction)
{
    Logging::log(Logging::DEBUG, "MapfileManager::checkLocalAssetPresence : %s (action: %d)\r\n", id.c_str(), takeAction);
    INDENT_LOG(Logging::DEBUG);

    return AssetManager::getManagerByType(AssetManager::RAWFILE)->checkLocalAssetPresence(getSubfileName(id + ".cfg")) &&
           AssetManager::getManagerByType(AssetManager::RAWFILE)->checkLocalAssetPresence(getSubfileName(id + ".ogz"));
}


// Create instances

void AssetManager::createManagers()
{
    new MapfileManager();
    new RawFileManager();
}

