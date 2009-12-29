
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
// along with the Intensity Engine.  If not = see
//     http://www.gnu.org/licenses/
//     http://www.gnu.org/licenses/agpl-3.0.html
//=============================================================================

GUN_FIST = 0;
GUN_SG = 1;
GUN_CG = 2;
GUN_RL = 3;
GUN_RIFLE = 4;
GUN_GL = 5;
GUN_PISTOL = 6;
GUN_FIREBALL = 7;
GUN_ICEBALL = 8;
GUN_SLIMEBALL = 9;
GUN_BITE = 10;
GUN_BARREL = 11;


//! XXX Deprecated
//! Shoots a fireball.
//! @param origin The position from which the fireball starts.
//! @param destination The target towards which the fireball goes, in a straight line.
//! @param onHit A callback function that is called when the fireball hits something. You can use this to set up an
//! explosion, deal damage, etc.
//! @param shooter The entity firing this fireball, which might be useful to prevent it hitting that entity (?).
function shootFireball(origin, destination, onHit, shooter) {
    log(ERROR, "Sending shooter inste d of cpp addr!");

    //! Convenience so that onHit can receive a Vector3
    function onHitHelper(x, y, z) {
        onHit( new Vector3(x, y, z) );
    }

    CAPI.shootvHelper(GUN_RL, origin.x, origin.y, origin.z, destination.x, destination.y, destination.z, onHitHelper, shooter);
}

