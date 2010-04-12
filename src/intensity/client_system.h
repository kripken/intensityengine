
// Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
// This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

//! System management for the client: usernames, passwords, logging in, client numbers, etc.

struct ClientSystem
{
    //! A constant, essentially: the password sent to CEGUI when we have one stored for
    //! the player along with his/her password. If we get this dummy value back from
    //! CEGUI then the user wants to use the hashed password we have saved, and we
    //! send that to the server. We use this dummy instead of the hash, because (1)
    //! the hash is quite long, and (2) might contain values not valid for CEGUI's
    //! password field.
    static std::string   blankPassword; // Constant

    //! The client number of the PC. A copy of player1->clientnum, but nicer name
    static int           playerNumber;

    //! Convenient way to get at the player's logic entity
    static LogicEntityPtr playerLogicEntity;

    //! Whether logged in to a _remote_ server. There is no 'login' locally, just 'connecting'.
    static bool          loggedIn;

    //! Whether we are only connecting locally
    static bool          editingAlone;

    //! UniqueID of the player in the current module. Set in a successful response to
    //! logging in. When we then load a map, this is used to create the player's
    //! LogicEntity.
    static int           uniqueId;

    //! The current map being played. Set when we receive a map from the server (TODO:
    //! also when we load a cached map)
    static std::string   currMap;

    static std::string currTransactionCode;

    static std::string currHost;
    static int currPort;

    //! An identifier for the current scenario the client is active in. Used to check with the
    //! server, when the server starts a new scenario, to know when we are in sync or not
    static std::string currScenarioCode;

    // Functions

    //! Username stored in our config files, saved from last use
    static std::string getUsername();
    //! Hash of last password entered, saved in config file
    static std::string getHashedPassword();
    //! The visual password, i.e., what is sent to CEGUI. Might be blank, or blankPassword
    static std::string getVisualPassword();

    //! Connects to the server, at the enet level
    static void connect(std::string host, int port);

    //! After connected at the enet level, validate ourselves to the server using the transactionCode we received from the master server
    //!
    //! clientNumber: The client # the server gave to us. Placed in playerNumber.
    static void login(int clientNumber);

    //! Called upon a successful login to an instance. Sets logged in state to true, and the uniqueID as that received
    //! from the LoginResponse.
    static void finishLogin(bool local);

    //! Disconnects from the server and returns to the main menu
    static void doDisconnect();

    //! Marks the status as not logged in. Called on a disconnect from sauer's client.h:gamedisconnect()
    static void onDisconnect();

    //! Clears the player logic entity, when it has been invalidated by loading a new logic system
    static void clearPlayerEntity();

    //! Sends a just-saved map (all 3 parts: ogz, cfg, unique_ids) to server, at the end of worldio.cpp:save_world().
    //! The map is worked on in the user's dir/packages/base, as per sauer norms.
    static void sendSavedMap();

    //! Whether the scenario has actually started, i.e., we have received everything we need from the server to get going
    static bool scenarioStarted();

    //! Stuff done on each frame
    static void frameTrigger(int curtime);

    static void gotoLoginScreen();

    static void finishLoadWorld();

    static void prepareForNewScenario(std::string scenarioCode);

    //! Reads client-related settings (fullscreen, etc.) from the config file and applies them.
    //! Done before initializing the system, so that these settings are applied.
    static void handleConfigSettings();

    //! Check if this user has admin privileges, which allows entering edit mode and using the Sauer console (/slash)
    static bool isAdmin();

    static void addHUDImage(std::string tex, float centerX, float centerY, float widthInUniform, float heightInUniform);

    static void addHUDText(std::string text, float x, float y, float scale, int color);

    // x2 < 0 means it is a uniform width; ditto y2 for height
    static void addHUDRect(float x1, float y1, float x2, float y2, int color);

    //! Show the current HUD. Called once per frame
    static void drawHUD(int w, int h);

    //! Called once/frame, after drawing the HUD. Cleans up temporary structures.
    //! It is important to call this even if the HUD is not drawn (e.g. in edit
    //! mode).
    static void cleanupHUD();
};

