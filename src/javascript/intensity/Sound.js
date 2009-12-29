
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


Sound = {
    //! If done on the server, a message is sent to clients to play the sound
    play: function(_name, position, volume, clientNumber) {
        position = defaultValue(position, new Vector3(0, 0, 0));
        volume = defaultValue(volume, 0);
            
        if (Global.CLIENT) {
            CAPI.playSoundByName(_name, position.x, position.y, position.z, volume);
        } else {
            // TODO: Do not send sound to clients if they are too far to hear it!

            // Warn if using non-compressed names
            if (_name.length > 2) {
                log(WARNING, format("Sending a sound '{0}' to clients using full string name. This should be done rarely, for bandwidth reasons.", _name));
            }

            clientNumber = defaultValue(clientNumber, MessageSystem.ALL_CLIENTS);
            MessageSystem.send(clientNumber, CAPI.SoundToClientsByName, position.x, position.y, position.z, _name, -1);
        }
    },

    playMusic: function(_name) {
        CAPI.music(_name);
    },

    setMusicHandler: function(func) {
        this.musicHandler = func;
        this.musicCallback(); // Start playing now
    },

    musicCallback: function() {
        if (this.musicHandler) {
            this.musicHandler();
        }
    },
};

