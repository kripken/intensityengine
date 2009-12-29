
//=============================================================================
// Copyright (C) 2008 Alon Zakai ('Kripken') kripkensteiner@gmail.com
//
// This file is part of the Intensity Engine project,
//    http://www.intensityengine.com
//
// The Intensity Engine is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published by
// the Free Software Foundation, version 3.
//
// The Intensity Engine is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with the Intensity Engine.  If not, see
//     http://www.gnu.org/licenses/
//     http://www.gnu.org/licenses/agpl-3.0.html
//=============================================================================


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

