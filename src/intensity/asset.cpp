
// Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
// This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

#include "pch.h"
#include "engine.h"

#include "asset.h"
#include "logging.h"


// Statics
AssetManager::ManagerTypeMap AssetManager::managers;

// AssetManager

AssetManager::AssetManager(AssetType _assetType) : assetType(_assetType)
{
    assert(managers.find(assetType) == managers.end());
    managers[assetType] = this;
}

AssetManager* AssetManager::getManagerByType(AssetType assetType)
{
    return managers[assetType];
}

const AssetInfo AssetManager::getAssetInfo(const AssetId& id)
{
    AssetInfoMap::iterator iter = registeredAssets.find(id);

    if (iter == registeredAssets.end())
    {
        // We are missing the info
        Logging::log(Logging::ERROR, "Request for the info of a non-registered asset: %s\r\n", id.c_str());
        assert(0);
    }

    return iter->second;
}

void AssetManager::registerAssetInfo(const AssetId& id, const AssetInfo& info)
{
    Logging::log(Logging::DEBUG, "AssetManager::registerAssetInfo: %s\r\n", id.c_str());
    INDENT_LOG(Logging::DEBUG);

    if (registeredAssets.find(id) == registeredAssets.end())
        registeredAssets.insert( AssetInfoMap::value_type( id, info ) );
    else
        registeredAssets[id] = info;

    // If actions pend on this asset, continue the process and request it,
    // if we lack the current version

    AssetActionListMap::iterator assetIter = pendingActions.find(id);

    if (assetIter != pendingActions.end())
    {
        Logging::log(Logging::DEBUG, "Actions are pending for this asset, ensure that it will arrive or act immediately\r\n");

        if (checkLocalAssetPresence(id, true)) // Will send a request if necessary
        {
            triggerAssetReceived(id); // The asset is already here, run the actions right now
        }
    }
}


void AssetManager::queueAction(const AssetId& id, AssetActionPtr action)
{
    AssetActionListMap::iterator iter = pendingActions.find(id);

    if (iter == pendingActions.end())
    {
        // Add a new vector for that id
        iter = pendingActions.insert( AssetActionListMap::value_type( id, AssetActionList() ) ).first;
    }

    iter->second.push_back( QueuedAction(action, Utility::SystemInfo::currTime()) );
}

void AssetManager::removeAction(const AssetId& id, AssetActionPtr action)
{
    AssetActionListMap::iterator assetIter = pendingActions.find(id);

    assert(assetIter != pendingActions.end());

    AssetActionList& actionList = assetIter->second;

    for(AssetActionList::iterator actionIter = actionList.begin();
        actionIter != actionList.end();
        actionIter++)
    {
        if ((*actionIter).action == action)
        {
            actionList.erase(actionIter);
            break;
        }
    }

    if (actionList.empty())
    {
        // Remove the vector itself, nothing is left there
        pendingActions.erase(assetIter);
    }
}

bool AssetManager::checkLocalAssetPresence(const AssetId& id, bool takeAction)
{
    Logging::log(Logging::DEBUG, "AssetManager::checkLocalAssetPresence : %s\r\n", id.c_str());

    AssetInfoMap::iterator iter = registeredAssets.find(id);

    if (iter == registeredAssets.end())
    {
        // We are missing the info

        Logging::log(Logging::DEBUG, "AssetManager::checkLocalAssetPresence : missing Info\r\n", id.c_str());

        if (takeAction)
        {
            Logging::log(Logging::DEBUG, "AssetManager::checkLocalAssetPresence : requesting Info\r\n");

            requestAssetInfo(id);
        }

        return false;
    }

    AssetInfo& info = iter->second;
    AssetHash localHash = getLocalAssetHash(id);

    assert(info.hash != ""); // This is both an error, and will cause a bug since "" is what we get with there is no local file,
                             // so we would get a false misleading equality

    if (info.hash != localHash)
    {
        // We have an outdated version of this asset

        Logging::log(Logging::DEBUG, "AssetManager::checkLocalAssetPresence : incorrect hash: %s vs. %s\r\n",
            info.hash.c_str(), localHash.c_str()
        );

        if (takeAction)
        {
            Logging::log(Logging::DEBUG, "AssetManager::checkLocalAssetPresence : requesting asset\r\n");

            requestAsset(id);
        }

        return false;
    }

    Logging::log(Logging::DEBUG, "AssetManager::checkLocalAssetPresence : %s : YES\r\n", id.c_str());

    return true;
}

void AssetManager::actOnAsset(const AssetId& id, const AssetActionPtr action)
{
    // Check for asset presence, and take action if it isn't here
    if (!checkLocalAssetPresence(id, true))
    {
        // Asset is not present. We already took action to get it; queue the action meanwhile
        queueAction(id, action);
    } else {
        // The asset is present, act immediately
        action->act(getLocalAssetPath(id), 0);
    }
}

struct DeferredRemoval
{
    AssetId id;
    AssetActionPtr action;
    DeferredRemoval(AssetId _id, AssetActionPtr _action) : id(_id), action(_action) { };
};

void AssetManager::manageActions()
{
    std::vector<DeferredRemoval> deferredRemovals;

    for(AssetActionListMap::iterator assetIter = pendingActions.begin();
        assetIter != pendingActions.end();
        assetIter++)
    {
        const AssetId& id = assetIter->first;

        for(AssetActionList::iterator actionIter = assetIter->second.begin();
            actionIter != assetIter->second.end();
            actionIter++)
        {
            AssetActionPtr action = actionIter->action;
            float secondsElapsed = float(Utility::SystemInfo::currTime() - actionIter->time)/1000.0f;

            if (action->shouldAbandon(secondsElapsed))
                deferredRemovals.push_back( DeferredRemoval(id, action) );
        }
    }

    for (unsigned int i = 0; i < deferredRemovals.size(); i++)
        removeAction(deferredRemovals[i].id, deferredRemovals[i].action);
}

void AssetManager::manageAllActions()
{
    for(ManagerTypeMap::iterator iter = managers.begin();
        iter != managers.end();
        iter++)
        iter->second->manageActions();
}

void AssetManager::triggerAssetReceived(const AssetId& id)
{
    assert(checkLocalAssetPresence(id));

    // Run all actions
    AssetActionListMap::iterator assetIter = pendingActions.find(id);

    if (assetIter == pendingActions.end())
        return; // None remain that have not given up

    AssetActionList& actions = assetIter->second;
    std::string path = getLocalAssetPath(id);

    for(AssetActionList::iterator actionIter = actions.begin();
        actionIter != actions.end();
        actionIter++)
    {
        AssetActionPtr action = actionIter->action;
        float secondsElapsed = float(Utility::SystemInfo::currTime() - actionIter->time)/1000.0f;

        action->act(path, secondsElapsed);
    }

    // Remove the entire list of actions for this asset
    pendingActions.erase(assetIter);
}

