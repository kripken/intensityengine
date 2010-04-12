
// Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
// This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.


ApplicationManager = {
    //! Sets the actual instance of a (child class of) Application. This is the singleton instance that we will
    //! use.
    setApplicationClass: function(_class) {
        log(DEBUG, "ApplicationManager: setting ApplicationClass to " + _class);

        ApplicationManager.instance = new _class();

        // Do not run __init__ on client
        if (Global.SERVER) {
            ApplicationManager.instance.init();
        }

        log(DEBUG, "ApplicationManager: instance is " + ApplicationManager.instance);
    }
};

//! A definition of general settings for a specific application. This is an abstract base class
//! that defines the appropriate interface to use.

Application = Class.extend({
    //! Called after the system is initialized, and lets application-specific initialization begin.
    //! Note that such initialization might depend on the state of the database, for example, you
    //! might check if some central management LogicEntity already exists, and if not, create it.
    //! That entity then persists in the database, so certain initialization can not be done twice.
    //!
    //! This is run only on the server. It is run once when the application is started, which for
    //! now is once when the server is started.
    init: function() {
        log(WARNING, "Application.init: You should override this, and there is no need to call the ancestor. This should never run.");
    },

    //! Returns the class used for Player entities, i.e., that represent human players logging into this game.
    //! Note that this function needs to return the class name in string form (.__class__.__name__ is helpful for that).
    //! If your application does not have any special class for player characters, you do not need to do
    //! anything, this function will return "Player", which is a plain vanilla class for player characters.
    getPcClass: function() {
        return "Player";
    },

    //! Called on the client when surprisingly disconnected from the server, perhaps due to
    //! a network error, etc. A typical behaviour is to show an error and switch to a login screen.
    clientOnDisconnect: function() {
        log(WARNING, "Application.clientOnDisconnect: You should override this, and there is no need to call the ancestor. This should never run.");
    },

    //! A script that runs (on the server) when a logic entity moves off the map. This should not
    //! occur in general - the map should be set up so this is prevented - but if it does somehow
    //! happen, this can e.g. remove the entity, show an error, or something like that.
    onEntityOffMap: function(entity) {
        log(WARNING, format("{0} has fallen off the map.", entity.uniqueId));
//        if (entity.fallenOffMap === undefined || entity.fallenOffMap === false) {
//            log(WARNING, format("{0} has fallen off the map.", entity.unique_id));
//            entity.fallenOffMap = true; // Only do this ONCE
//            entity.clearActions();
//            entity.queueAction( new SingleCommandAction(removeEntity, entity) );
//        }
    },

    clientOnEntityOffMap: function(entity) {
        log(WARNING, format("{0} has fallen off the map.", entity.uniqueId));
    },

    //! Called when a player logs in and joins the game. This would be a good place to do some setup
    //! for the player, e.g., set position, team, etc. Such stuff might also be done in the
    //! player class itself.
    onPlayerLogin: function(player) {
    },

    //! Called when the forward/backward buttons are pressed. By default we do a normal movement
    //! @param move Forward or backward
    //! @param down If the button press is down or not
    performMovement: function(move, down) {
        getPlayerEntity().move = move;
    },

    //! Called when the left/right buttons are pressed. By default we do a normal strafe
    //! @param strafe Left or right
    //! @param down If the button press is down or not
    performStrafe: function(strafe, down) {
        getPlayerEntity().strafe = strafe;
    },

    //! Called when the 'jump' button is pressed. By default we do a normal jump
    performJump: function(down) {
        if (down) {
            getPlayerEntity().jump();
        }
    },

    //! XXX Not yet enabled
    performYaw: function(yaw, down) {
        getPlayerEntity().yawing = yaw;
    },

    //! XXX Not yet enabled
    performPitch: function(pitch, down) {
        getPlayerEntity().pitching = pitch;
    },

    //! Allows customization of the effects of mouse moving. The returned yaw, pitch are applied to
    //! the normal sauer camera system. You can set the return values to 0 and alter things otherwise if you want
    performMousemove: function(yaw, pitch) {
        return { yaw: yaw, pitch: pitch };
    },

    //! Called when the player clicks on a position in the world. This does the following: It calls
    //! clientClick(), which can contain user-defined behavior on the client. If this does not return True (i.e.,
    //! if it returns False or anything else, or nothing), then the server is contacted, and runs click(). By
    //! default clientClick does nothing, so the default behavior is to run the server's click() (somewhat like
    //! Second Life), but overriding clientClick can allow responsive behaviors that occur immediately on the client,
    //! and can override (or not override) actions on the server.
    //! @param button The mouse button, 1, 2 or 3
    //! @param down Whether this is a downclick on an upclick
    //! @param position The location where the click occurred
    //! @param entity Whether the click was on an entity, or undefined/null if not
    //! @param x Absolute x position on the screen (in [0,1])
    //! @param y Absolute y position on the screen (in [0,1])
    performClick: function(button, down, position, entity, x, y) {
        if (this.clientClick(button, down, position, entity, x, y) !== true) {
            var uniqueId = -1;
            if (entity) {
                uniqueId = entity.uniqueId;
            }
            MessageSystem.send(CAPI.DoClick, button, down, position.x, position.y, position.z, uniqueId);
        }
    },

    //! Called on the client when an entity is clicked. See performClick() for more details about how this fits in the big picture.
    clientClick: function(button, down, position, entity, x, y) {
    },

    //! Called on the server when an entity is clicked. See performClick() for more details about how this fits in the big picture.
    click: function(button, down, position, entity) {
    },

    //! Called when an 'action key' is pressed down or up. This occurs when a key is bound
    //! to actionkeyX in cubescript, where X is in the range 0 to 29. This lets people
    //! bind keys however they want, and games always deal with action keys 0-29.
    actionKey: function(index, down) {
    },

    getScoreboardText: function() {
        return [
            [-1, "No scoreboard text defined"],
            [-1, "This should be done in the Application"]
        ];
    },

    //! Override to show a different crosshair
    getCrosshair: function() {
        return "data/crosshair.png";
    },

    //! Called when a text message is sent from a client. Scripts can handle those if they want. The return value is
    //! true if the message was handled, in which case the server will do nothing else; if false, then the message
    //! will be relayed using the default method. The default is of course false.
    //!
    //! @param uniqueId The uid of the sender
    //! @param text The content of the message
    handleTextMessage: function(uniqueId, text) {
        return false;
    },
});

//! A simple Application class, used until replaced. This is just to prevent runtime errors in
//! the case that the Application is *not* replaced (it should be)
__DummyApplication = Application.extend({
    init: function() {
        log(WARNING, "(init) ApplicationManager.setApplicationClass was not called, this is the DummyApplication running.");
        this._super(); // Should not be done in actual Applications!
    },

    getPcClass: function() {
        log(WARNING, "(getPcClass) ApplicationManager.setApplicationClass was not called, this is the DummyApplication running.");
        return "Player";
    },

    clientOnDisconnect: function() {
        log(WARNING, "(clientOnDisconnect) ApplicationManager.setApplicationClass was not called, this is the DummyApplication running.");
        this._super(); // Should not be done in actual Applications!
    },

    onEntityOffMap: function(entity) {
        log(WARNING, "(onEntityOffMap) ApplicationManager.setApplicationClass was not called, this is the DummyApplication running.");
        this._super(entity); // Should not be done in actual Applications!
    },

    clientOnEntityOffMap: function(entity) {
        log(WARNING, "(onEntityOffMap) ApplicationManager.setApplicationClass was not called, this is the DummyApplication running.");
        this._super(entity); // Should not be done in actual Applications!
    },

    onPlayerLogin: function(player) {
        log(WARNING, "(onPlayerLogin) ApplicationManager.setApplicationClass was not called, this is the DummyApplication running.");
        this._super(player); // Should not be done in actual Applications!
    },

    performClick: function() {
        log(WARNING, "(performClick) ApplicationManager.setApplicationClass was not called, this is the DummyApplication running.");
        this._super.apply(this, arguments); // Should not be done in actual Applications!
    },

    clientClick: function() {
        log(WARNING, "(clientClick) ApplicationManager.setApplicationClass was not called, this is the DummyApplication running.");
        this._super.apply(this, arguments); // Should not be done in actual Applications!
    },

    click: function() {
        log(WARNING, "(click) ApplicationManager.setApplicationClass was not called, this is the DummyApplication running.");
        this._super.apply(this, arguments); // Should not be done in actual Applications!
    },

    actionKey: function(index, down) {
        log(WARNING, "(actionKey) ApplicationManager.setApplicationClass was not called, this is the DummyApplication running.");
    },
});

log(DEBUG, "Setting dummy application");
ApplicationManager.setApplicationClass(__DummyApplication);
//eval(assert(' ApplicationManager.instance.getPcClass() === "Player" '));

