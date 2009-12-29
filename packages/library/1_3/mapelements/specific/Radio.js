
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


registerEntityClass(bakePlugins(WorldMarker, [{
    _class: 'Radio',
    shouldAct: true,

    radioURL: new StateString(),
    radioVolume: new StateInteger(),
    radioRadius: new StateInteger(),

    init: function() {
        this.radioURL = 'http://scfire-mtc-aa02.stream.aol.com:80/stream/1022';
        this.radioVolume = 25; // 25%
        this.radioRadius = 100;
    },

    clientActivate: function() {
        this.volumeTimer = new RepeatingTimer(0.25);
        this.playing = '';
    },

    clientAct: function(seconds) {
        if (this.volumeTimer.tick(seconds)) {
            var dist = this.position.subNew(getPlayerEntity().getCenter()).magnitude();
            var volume = Math.floor(this.radioVolume * clamp(1-dist/this.radioRadius, 0, 1));
            if (dist > this.radioRadius*1.5 || this.radioURL === '') { // 1.5, for a safe area even though we have 0 volume
                // Do not play
                if (this.playing !== '') {
                    CAPI.signalComponent('VLC', 'play|');
                    this.playing = '';
                }
            } else {
                // Play
                if (this.playing !== this.radioURL) {
                    CAPI.signalComponent('VLC', 'play|' + this.radioURL);
                    this.playing = this.radioURL;
                    this.lastVolume = -1;
                }
                if (volume !== this.lastVolume) {
                    CAPI.signalComponent('VLC', 'setvolume|' + volume);
                    this.lastVolume = volume;
                }
            }
        }
    },
}]));

if (!CAPI.signalComponent) {
    CAPI.signalComponent = function() {
        log(ERROR, "CAPI.signalComponent is not present in this build");
    }
}

