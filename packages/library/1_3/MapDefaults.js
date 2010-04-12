
// Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
// This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

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

