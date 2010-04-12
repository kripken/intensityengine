
// Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
// This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

// Convert vector forms into expanded forms

// Setters

function vectorize(vec) {
    if (vec.x === undefined || vec.y === undefined || vec.z === undefined) {
        eval(assert(' vec.x === undefined && vec.y === undefined && vec.z === undefined '));

        return { x: vec[0], y: vec[1], z: vec[2] };
    } else {
        return vec;
    }
}

CAPI.setExtentO = function(self, vec) {
    vec = vectorize(vec);
    log(DEBUG, format("CAPI.setExtentO: {0},{1},{2}", vec.x, vec.y, vec.z));
    CAPI.setExtentO_raw(self, vec.x, vec.y, vec.z);
};

CAPI.setDynentO = function(self, vec) {
    vec = vectorize(vec);
    log(DEBUG, format("CAPI.setDynentO: {0},{1},{2}", vec.x, vec.y, vec.z));
    CAPI.setDynentO_raw(self, vec.x, vec.y, vec.z);
};

CAPI.setDynentVel = function(self, vec) {
    vec = vectorize(vec);
    CAPI.setDynentVel_raw(self, vec.x, vec.y, vec.z);
};

CAPI.setDynentFalling = function(self, vec) {
    vec = vectorize(vec);
    CAPI.setDynentFalling_raw(self, vec.x, vec.y, vec.z);
};

// Getters

CAPI.getExtentO = function(self) {
    var vec = [ CAPI.getExtentO_raw(self, 0),
                CAPI.getExtentO_raw(self, 1),
                CAPI.getExtentO_raw(self, 2) ];

    log(INFO, format("CAPI.getExtentO: {0}", serializeJSON(vec)));

    return vec;
};

CAPI.getDynentO = function(self, index) {
    return [ CAPI.getDynentO_raw(self, 0),
             CAPI.getDynentO_raw(self, 1),
             CAPI.getDynentO_raw(self, 2) ];
};

CAPI.getDynentVel = function(self, index) {
    return [ CAPI.getDynentVel_raw(self, 0),
             CAPI.getDynentVel_raw(self, 1),
             CAPI.getDynentVel_raw(self, 2) ];
};

CAPI.getDynentFalling = function(self, index) {
    return [ CAPI.getDynentFalling_raw(self, 0),
             CAPI.getDynentFalling_raw(self, 1),
             CAPI.getDynentFalling_raw(self, 2) ];
};


CAPI.setAttachments = function(self, attachments) {
    CAPI.setAttachments_raw(self, attachments.join("|")); // Give Python the format it wants
}

// DEBUG stuff

var oldSetModelName = CAPI.setModelName;

CAPI.setModelName = function(self, model) {
    log(DEBUG, "CAPI.setModelName: " + model + "(" + typeof model + "), self=" + self);
    oldSetModelName(self, model);
};


// Mapping

Map = {
    textureReset: CAPI.textureReset,

    texture: function(type, _name, rot, xoffset, yoffset, scale) {
        rot = defaultValue(rot, 0);
        xoffset = defaultValue(xoffset, 0);
        yoffset = defaultValue(yoffset, 0);
        scale = defaultValue(scale, 1);
        CAPI.texture(type, _name, rot, xoffset, yoffset, scale);
    },

    mapmodelReset: CAPI.mapmodelReset,
    mapmodel: CAPI.mapmodel,

    autograss: CAPI.autograss,

    texLayer: CAPI.texLayer,

    setShader: CAPI.setShader,

    setShaderParam: function(_name, x, y, z, w) {
        x = defaultValue(x, 0);
        y = defaultValue(y, 0);
        z = defaultValue(z, 0);
        w = defaultValue(w, 0);
        CAPI.setShaderParam(_name, x, y, z, w);
    },

    materialReset: CAPI.materialReset,

    loadSky: CAPI.loadSky,

    fogColor: CAPI.fogColor,
    fog: CAPI.fog,
    shadowmapAngle: CAPI.shadowmapAngle,
    shadowmapAmbient: CAPI.shadowmapAmbient,
    skylight: CAPI.skylight,
    blurSkylight: CAPI.blurSkylight,
    ambient: CAPI.ambient,

    waterFog: CAPI.waterFog,
    waterColor: CAPI.waterColor,
    spinSky: CAPI.spinSky,
    cloudLayer: CAPI.cloudLayer,
    cloudScrollX: CAPI.cloudScrollX,
    cloudScrollY: CAPI.cloudScrollY,
    cloudScale: CAPI.cloudScale,
    skyTexture: CAPI.skyTexture,
    texScroll: CAPI.texScroll,

    preloadSound: function(_name, volume) {
        volume = defaultValue(volume, 100);
        CAPI.preloadSound(_name, volume);
    },

    preloadModel: CAPI.preloadModel,

    convertJP2toPNG: function(src, dest) {
        dest = defaultValue(dest, src.replace('.jp2', '.png'))
        return CAPI.convertJP2toPNG(src, dest);
    },

    convertPNGtoDDS: function(src, dest) {
        dest = defaultValue(dest, src.replace('.png', '.dds'))
        return CAPI.convertPNGtoDDS(src, dest);
    },

    combineImages: CAPI.combineImages,
};

// World

World = {
    isColliding: function(position, radius, ignore) {
        return CAPI.isColliding(position.x, position.y, position.z, radius, ignore ? ignore.uniqueId : -1);
    }
};

World.__defineGetter__('gravity', function () {
    return World._gravity;
});

World.__defineSetter__('gravity', function (value) {
    World._gravity = value;
    CAPI.setGravity(value);
});

//! The gravity to apply to things in this world. The default value,
//! identical to Cube 2, is 200.0. You can change this in your
//! own maps, and even change it dynamically during the game (note
//! that it isn't synchronized between clients and server, so
//! do that manually if you need it).
World.gravity = 200.0; // Sauer default. This sets the value in sauer.

// Libraries

Library = {
    //! Library.include('some/dir/')          - will      load packages/some/dir/Package.js'
    //! Library.include('some/dir/Module')    - will      load packages/some/dir/Module.js'
    //! Library.include('some/dir/Module.js') - will also load packages/some/dir/Module.js'
    //!
    //! Include system: the script is run in eval. This means that
    //!     x = 5;
    //! will be set on the global object, meaning that x is effectively 'exported'
    //! globally. However,
    //!     var x = 5;
    //! will NOT be exported. So, use 'var' to keep things private inside imported scripts.
    //! Note that
    //!     function x() {};
    //! will be treated as if using 'var' by JavaScript - such functions are entirely
    //! local, and NOT exported globally. To export functions globally, use
    //!     x = function() { };
    //! but not that this means x is a nameless function (the name won't appear in
    //! stack traces). To get around that, you can write the (cumbersome)
    //!     x = function x() { };
    //! (i.e., repeat 'x' twice).
    //!
    //! The include system will include a module no more than once. This means it is
    //! ok for each module to include all its dependencies; if they have already
    //! been loaded, nothing will happen.
    //!
    //! @param force If true, the module will be run even if it has been run before.
    //!              This is necessary, for example, if you include the same 'anims.js'
    //!              multiple times for a model's different versions (red/blue, etc.).
    include: function(_name, force) {
        if (_name[_name.length - 1] === '/') {
            _name += 'Package.js';
        } else if (_name.substring(_name.length-3) !== '.js') {
            _name += '.js';
        }

        if (force || !this.loadedModules[_name]) {
            this.loadedModules[_name] = true;
            CAPI.compile(CAPI.readFile(_name), 'Library.include: ' + _name);
        }
    },

    loadedModules: {},
};

/* Version with explicit _export() calls:
//        this.toExport = [];

        eval(CAPI.readFile(_name));

//        forEach(this.toExport, function(item) {
//            eval(format('this.{0} = {0};', item)); // Globalize the item
//        }, this.__global);
//
//        delete this.toExport; // GC it
    },

//    //! Will export the named item (or list of named items) globally.
//    //! This is implemented by saving the exported items and globalizing them
//    //! at the end of the include() call.
//    _export: function(stuff) {
//        if (typeof stuff === 'string') {
//            stuff = [stuff];
//        }
//
//        this.toExport.push.apply(this.toExport, stuff);
//    },
};

//Library.__global = this; // A convenient ref to the global object
*/

// Models

Model = {
    shadow: CAPI.modelShadow,
    collide: CAPI.modelCollide,
    perEntityCollisionBoxes: CAPI.modelPerEntityCollisionBoxes,
    ellipseCollide: CAPI.modelEllipseCollide,

    objLoad: CAPI.objLoad,
    objSkin: CAPI.objSkin,
    objBumpmap: CAPI.objBumpmap,
    objEnvmap: CAPI.objEnvmap,
    objSpec: CAPI.objSpec,
    alphatest: CAPI.mdlAlphatest,
    bb: CAPI.mdlBb,
    scale: CAPI.mdlScale,
    spec: CAPI.mdlSpec,
    glow: CAPI.mdlGlow,
    glare: CAPI.mdlGlare,
    ambient: CAPI.mdlAmbient,
    shader: CAPI.mdlShader,

    collisionsOnlyForTriggering: CAPI.mdlCollisionsOnlyForTriggering,

    trans: CAPI.mdlTrans,

    md5Dir: CAPI.md5Dir,
    md5Load: CAPI.md5Load,

    md5Skin: function(meshname, tex, masks, envmapmax, envmapmin) {
        masks = defaultValue(masks, '');
        envmapmax = defaultValue(envmapmax, 0);
        envmapmin = defaultValue(envmapmin, 0);
        CAPI.md5Skin(meshname, tex, masks, envmapmax, envmapmin);
    },

    md5Bumpmap: CAPI.md5Bumpmap,
    md5Envmap: CAPI.md5Envmap,
    md5Alphatest: CAPI.md5Alphatest,

    yaw: CAPI.modelYaw,
    pitch: CAPI.modelPitch,

    md5Tag: CAPI.md5Tag,
    md5Anim: function(anim, animfile, speed, priority) {
        speed = defaultValue(speed, 0);
        priority = defaultValue(priority, 0);
        CAPI.md5Anim(anim, animfile, speed, priority);
    },

    md5Animpart: CAPI.md5Animpart,
    md5Pitch: CAPI.md5Pitch,

    rdVert: CAPI.rdVert,
    rdTri: CAPI.rdTri,
    rdJoint: CAPI.rdJoint,
    rdLimitDist: CAPI.rdLimitDist,
    rdLimitRot: CAPI.rdLimitRot,

    envmap: CAPI.mdlEnvmap,
};


// User interface

UserInterface = {
    showMessage: CAPI.showMessage,

    showInputDialog: function(content, callback) {
        this.inputDialogCallback = callback;
        CAPI.showInputDialog(content);
    },
};


// Network

Network = {
    connect: CAPI.connect,
};


// New engine stuff

if (!CAPI.renderModelFixed) {
    log(WARNING, "Fixing CAPI.renderModel for version 1 and 2");

    var renderModelOld = CAPI.renderModel;
    var renderModel2Old = CAPI.renderModel2;

    CAPI.renderModel = function() {
        if (!renderModel2Old) {
            // No need to change anything
            renderModelOld.apply(this, arguments);
        } else {
            // CAPI expects new version
            var args = Array.prototype.slice.call(arguments);
            args.splice(8, 0, 0); // Add roll = 0
            renderModel2Old.apply(this, args);
        }
    };

    CAPI.renderModel2 = function() {
        if (!renderModel2Old) {
            // No renderModel2, so downconvert into renderModel
            var args = Array.prototype.slice.call(arguments);
            args.splice(8, 1); // Remove roll
            renderModelOld.apply(this, args);
        } else {
            renderModel2Old.apply(this, arguments);
        }
    };

    CAPI.renderModelFixed = true;
}

