
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


// Useful metadata

Tools.replaceFunction('Map.textureReset', function() {
    Map.textures = []; //!< Will map texture index in engine to the texture name
});

Tools.replaceFunction('Map.texture', function(type, _name, rot, xoffset, yoffset, scale, forceindex) {
    rot = defaultValue(rot, 0);
    xoffset = defaultValue(xoffset, 0);
    yoffset = defaultValue(yoffset, 0);
    scale = defaultValue(scale, 1);
    forceindex = defaultValue(forceindex, 0);
    CAPI.texture(type, _name, rot, xoffset, yoffset, scale, forceindex);

    if ((type === '0' || type === 'c') && !forceindex > 0) {
        Map.textures[Map.textures.length] = arguments[1];
    }
}, false);

//// Materials

Map.materialReset();

Map.texture("water", "materials/GK_Water_01_cc.jpg", 0, 0, 0, 2);    // water surface
Map.texture("1", "materials/GK_Water_F_01_cc.jpg", 0, 0, 0, 2);     // waterfall
Map.texture("1", "materials/GK_Water_03_nm.jpg", 0, 0, 0, 2);        // water normals
Map.texture("1", "materials/GK_Water_01_dudv.jpg", 0, 0, 0, 2);     // water distortion
Map.texture("1", "materials/GK_Water_F_01_nm.jpg", 0, 0, 0, 2);    // waterfall normals
Map.texture("1", "materials/GK_Water_F_01_dudv.jpg", 0, 0, 0, 2); // waterfall distortion

Map.texture("lava", "materials/GK_Lava_01_cc.jpg", 0, 0, 0, 2);    // lava surface
Map.texture("1", "materials/GK_Lava_01_cc.jpg", 0, 0, 0, 2);     // lavafall
Map.texture("1", "materials/GK_Lava_01_nm.jpg", 0, 0, 0, 2);        // lava normals

//// Textures

Map.textureReset();
Map.texture("0", "freeseamless/1.jpg"); // Reserved - dummy
Map.texture("water", "golgotha/watr1.jpg");

