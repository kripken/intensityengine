
// Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
// This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

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

