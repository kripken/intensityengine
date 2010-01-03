
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


Global.LIBRARY_VERSION = '1_3';

Library.include('library/' + Global.LIBRARY_VERSION + '/__CorePatches');

// Intensity version comparing

function isIntensityVersionAtLeast(test) {
    function makeComparable(str) {
        var values = str.split('.');
        var ret = 0, mul = 1;
        for (var i = values.length-1; i >= 0; i--) {
            eval(assert(' Math.floor(values[i]) < 100 '));
            ret += mul*Math.floor(values[i]);
            mul *= 100;
        }
        return ret;
    }
    return makeComparable(Global.version) >= makeComparable(test);
}


// Replacements for src/javascript stuff

_logicEntityClasses = {}; // Clear old classes, make room for new

Library.include('library/' + Global.LIBRARY_VERSION + '/LogicEntityClasses', true);
Library.include('library/' + Global.LIBRARY_VERSION + '/Actions', true);
Library.include('library/' + Global.LIBRARY_VERSION + '/Variables', true);
Library.include('library/' + Global.LIBRARY_VERSION + '/LogicEntity', true);
Library.include('library/' + Global.LIBRARY_VERSION + '/Animatable', true);
Library.include('library/' + Global.LIBRARY_VERSION + '/Character', true);
Library.include('library/' + Global.LIBRARY_VERSION + '/StaticEntity', true);
Library.include('library/' + Global.LIBRARY_VERSION + '/LogicEntityStore', true);
Library.include('library/' + Global.LIBRARY_VERSION + '/Application', true);

