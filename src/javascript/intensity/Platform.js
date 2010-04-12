
// Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
// This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

//! Global constants
Global = {
    //! Read this to know if the current script is running on the client. Always the opposite of SERVER.
    CLIENT : false,

    //! Read this to know if the current script is running on the server. Always the opposite of CLIENT.
    SERVER : false,

    //! Called once on initialization, to mark the running instance as a client. Sets SERVER, CLIENT.
    initAsClient : function() {
        this.CLIENT = true;
        this.SERVER = false;
    },

    //! Called once on initialization, to mark the running instance as a server. Sets SERVER, CLIENT.
    initAsServer : function() {
        this.SERVER = true;
        this.CLIENT = false;
    }
};

// Small convenience function, useful for default values for functions
defaultValue = function(value, def) {
    if (value === undefined) {
        return def;
    } else {
        return value;
    }
};

// Intensity Engine exception class
function IntensityError(message) {
    this.message = message;
    this.name = "IntensityError";
}

