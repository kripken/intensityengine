
// Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
// This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

//! A system for connecting remote assets (sounds, etc., that are server-residing)
//! that can arrive from multiple sources (/servers), each of which has its own
//! compact identification for it. That is, this is a system for 'on-demand'
//! asset management.
//!
//! Typical procedure (for the 'compact procedure', see below):
//!
//!     1. Server tells us about a resource we need, e.g., the mapfile that we
//!        should run. It gives us a AssetInfo, i.e., the ID of the
//!        asset (a string) and it's hash.
//!     2. We check if we have a copy of that asset. If so, we check the hash.
//!        If we don't have it or have an outdated version, we ask the server
//!        for the asset, and it sends us the file.
//!
//! Note that in this procedures events might lead to queuing the operation
//! on the asset, or forgoing it, as we wait for server clarifications. E.g.
//! a sound might be played a few milliseconds late, but playing it much later
//! probably doesn't make much sense.
//!


//! An identifier of a remote asset. This should be unique in a 'grid'/'universe',
//! i.e., unique among all the servers we work with. To facilitate that, a prefix
//! might contain a URL or such, to prevent overlaps, e.g.,
//! "intensityengine.com/maps/tideturner"
//! Currently our code assumes IDs are 1000 bytes or less.
typedef std::string AssetId;

//!
//! A hash of a remote asset, i.e., an identifier for it. We use hashes to check if we
//! have the latest version of a remote asset, i.e., if we have the same as the server.
//!
//! We use string hashes, 16 bytes (128 bits) in length, the same as UUIDs (purposefully).
//! For now, we use SHA256 and keep the first 16 bytes (TODO: Consider better methods).
//!
//! TODO: Save a hash calculation of large files, so we don't do this every time
//! we restart the client/server?
typedef std::string AssetHash;

//! Info about a remote asset. Currently only its hash.
struct AssetInfo
{
    AssetHash hash;
    std::string shortpath; //!< The path to the asset. This is a portable path, it
                           //!< should contain enough information to construct an
                           //!< entire path given the configuration setting, platform,
                           //!< etc.

    AssetInfo() { };
    AssetInfo(AssetHash _hash, std::string _shortpath) : hash(_hash), shortpath(_shortpath) { };
};

//! A general container for an action to perform on an asset.
class AssetAction
{
protected:
    //! The asset on which we will act
    AssetId assetId;
    
public:
    AssetAction(const AssetId& _assetId) : assetId(_assetId) { };
    virtual ~AssetAction() { };

    //! Called every frame. Override if you want to do something like show a message
    //! while the asset is being acquired. By default we do not abandon the action,
    //! of course.
    //! @param secondsElapsed How long has passed since this action was requested.
    //! This can be used to decide if we should abandon it.
    //! @return true if this action should be abandoned, i.e., too much time has
    //! passed and we can forget about it. Note that the actual asset may already
    //! be in transit, and the Manager may still download it in its entirety (to
    //! be ready for future requests).
    virtual bool shouldAbandon(float secondsElapsed) { return false; };

    //! Perform the action.
    //! @param path The path to the asset
    //! @param secondsElapsed How much time passed since the request for this action on
    //!        the asset was done, in seconds. If we waited a long time for the server,
    //!        we might do something different or nothing at all.
    virtual void act(std::string path, float secondsElapsed) = 0;
};

typedef boost::shared_ptr<AssetAction> AssetActionPtr;

//! A generic container for several asset managers. Each manager then has
//! a list of registered assets.
//!
//! This is the basis for both clients and servers. This might have been
//! done with multiple inheritance, but (1) multiple inheritance is messy,
//! and (20 in the future we may likely have distributed asset networks,
//! in which a manager functions as a client part of the time and as a
//! server another.
class AssetManager
{
//===============================
// General management
//===============================

public:
    //! A code identifying an asset type
    enum AssetType { RAWFILE, MAPFILE, SOUND };

protected:
    //! The code identifying the asset type this manager uses. This is used in
    //! the protocol messages
    AssetType assetType;

    typedef std::map<AssetType, AssetManager*> ManagerTypeMap;

    //! List of all managers
    static ManagerTypeMap managers;

protected:
    typedef std::map<AssetId, AssetInfo> AssetInfoMap;

    //! Ids and Infos for all registered assets
    AssetInfoMap registeredAssets;

public:
    //! Registers the asset manager type, so it can be accessed via getManagerbyType.
    //! Only one kind of manager can exist per type.
    AssetManager(AssetType _assetType);
    virtual ~AssetManager() { };

    //! Returns the appropriate manager for the asset type. The manager must have
    //! been created (the constructor will then have registered it so that it can
    //! be found here).
    static AssetManager* getManagerByType(AssetType assetType);

    //! Gets the Info of an asset. The default implemenation looks for registered
    //! assets. This can be overridden, e.g. if we parse the asset ID in order
    //! to determine its info. Another overridding option is to register the asset
    //! if it isn't already registered, which might make sense in some contexts.
    //! For example, the client might require explicit registration, while the
    //! server (which has a much larger library of potential assets) looks in
    //! some other listing of assets (database, file hierarchy, etc.) for
    //! non-currently-registered assets.
    virtual const AssetInfo getAssetInfo(const AssetId& id);

    //! Registers an asset of the appropriate type with this manager. This is generally
    //! called after the server informs us about an asset (its ID and hash). This either
    //! registers a new asset, or updates the info for an existing one (the asset may
    //! have been updated on the server, which then notified us).
    //!
    //! If there are any Actions pending on this particular asset, then this function
    //! will proceed to request the asset itself from the server. Note that actions
    //! may have given up, in which case we do not waste the bandwidth.
    void registerAssetInfo(const AssetId& id, const AssetInfo& info);

    //! Overidden with the actual implementation for getting the path to a local copy of
    //! an asset.
    virtual std::string getLocalAssetPath(const AssetId& id) = 0;

    //! Overidden with the actual implementation for getting the hash of a local copy of
    //! an asset. Returns an empty hash if there is no local copy
    //! Should only be used internally or by other managers.
    virtual AssetHash getLocalAssetHash(const AssetId& id) = 0;

    // Implement with creation of application-specific managers
    static void createManagers();

    static void destroyManagers() { }; // TODO

//=============================
// Clientside
//=============================

private:
    struct QueuedAction
    {
        AssetActionPtr action;
        int time;
        QueuedAction(AssetActionPtr _action, int _time) : action(_action), time(_time) { };
    };

    typedef std::vector<QueuedAction> AssetActionList;

    //! For each asset, a list of actions pending on its arrival
    typedef std::map< AssetId, AssetActionList > AssetActionListMap;

    //! A list of current Actions, for each asset they are pending on.
    AssetActionListMap pendingActions;

    void queueAction(const AssetId& id, AssetActionPtr action);

    void removeAction(const AssetId& id, AssetActionPtr action);

public:
    //! Overidden with the actual implementation of how to ask the server for an AssetInfo
    //! Should only be used internally or by other managers.
    virtual void requestAssetInfo(const AssetId& id) = 0;

    //! Overidden with the actual implementation of how to ask the server for an Asset
    //! Should only be used internally or by other managers.
    virtual void requestAsset(const AssetId& id) = 0;

    //! Checks whether an asset is present. A present asset is registered and we have
    //! the latest copy of it. This function can also 'take action', i.e., perform
    //! what is needed to make an asset be present - issue requests to the server
    //! for info and/or for the asset itself.
    //! This function can be overridden if we check presence in a non-standard way,
    //! e.g., see comments for getAssetInfo.
    //! (It might make sense to only send requests for Infos if they are too old,
    //! so e.g. you don't verify every single asset every time you login. Or send
    //! them in bulk in some manner.)
    //! Should only be used internally or by other managers.
    virtual bool checkLocalAssetPresence(const AssetId& id, bool takeAction=false);

    //! Acts on an asset, given it's ID. The corresponding AssetInfo should have been registered
    //! beforehand using registerAssetInfo; it not, then we ask the server for them using
    //! requestAssetInfo. Once we have the AssetInfo, we can check
    //! if the asset is present, and we have the latest version (as tested by the hash),
    //! then we immediately perform the action we are told to do.
    //! If the asset is not present, we ask the server for it, using requestAsset, and
    //! we return an empty path. We perform the action once it arrives (the action can decide
    //! depending on how much time elapsed meanwhile whether to actually carry out the operation
    //! or not).
    void actOnAsset(const AssetId& id, AssetActionPtr action);

    //! Manages actions. Runs shouldAbandon for each action, and so forth. This is called
    //! once per frame.
    void manageActions();

    //! Manages actions for all managers
    static void manageAllActions();

    //! Called when an asset is received. Runs the appropriate action (if any remain, they
    //! might have given up meanwhile).
    void triggerAssetReceived(const AssetId& id);

//=======================
// Sourceside
//=======================
};


//!=========================================
//! Compact protocol / per-source procedure
//!=========================================

//! For protocol messages that recur a lot, we use compact IDs, leading
//! to the following more lengthy procedure, but which is overall much
//! more compact bandwidth-wise:
//!
//!     1. Server optionally 'prepares' us with some info, namely
//!        AssetCompactInfo, i.e., a 'compact ID' (a small integer value)
//!        and the corresponding normal ID to which it corresponds, on that
//!        server (/source).
//!     2. We receive a server message to do something with an asset having
//!        compact ID such and such.
//!     3. Using a combination of that compact ID and the source it came from
//!        - the server - we try to find the normal ID. If we have a
//!        AssetCompactInfo then we know the normal ID and continue.
//!        Otherwise we ask the server for it.
//!     4. Given the normal ID, we try to find the hash, i.e., see if we have
//!        a AssetInfo for that ID. If we don't. we ask the server for it.
//!     5. We can now continue as in step 2 in the brief procedure, above.
//!
//! In a sense this is a generalization of the Sauer protocol for client IDs,
//! but to the multi-server case and more generic.


//! An ID for a remote asset that is compact, so that transmitting it over the wire
//! is cheap. Such CompactIds are source-specific, and such a compact ID only makes
//! sense if we know the source (which we do with network messages).
typedef int AssetCompactId;;

//! A source identifier for remote assets, e.g., a server (which can serve e.g. sound assets,
//! texture assets, etc.).
typedef std::string AssetSourceId;

class AssetMultisourceClient : public AssetManager
{
#if 0

    //! Registers a compact ID along with it's source, so that we know the true ID that it represents.
    //! In other words, in the future when we receive that compact ID from that source, then we
    //! know what true id it represents.
    void registerCompactId(AssetSource source, AssetCompactId compactId, AssetId id);

    //! Called when we wish to get an asset, and we know the asset through a combination
    //! of its source (typically a server) and a source-specific compact key. This is the case
    //! for most network protocol messages, which lets them use compact IDs instead of
    //! complete ones.
    //! @return NULL if we do not have that asset.
    AssetPtr getAsset(AssetSource source, AssetCompactId compactId);

    //! Called when we wish to get an asset using its full identifier.
    //! @return NULL if we do not have that asset.
    AssetPtr getAsset(AssetId id);
#endif
};

