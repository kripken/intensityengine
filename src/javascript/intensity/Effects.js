
// Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
// This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

//== Decals - sync with iengine.h

DECAL = {
    SCORCH: 0,
    BLOOD: 1,
    BULLET: 2,
    CIRCLE: 3,
};

//== Particles - sync with iengine.h

PARTICLE = {
    BLOOD: 0,
    WATER: 1,
    SMOKE: 2,
    STEAM: 3,
    FLAME: 4,
    FIREBALL1: 5,
    FIREBALL2: 6,
    FIREBALL3: 7,
    STREAK: 8,
    LIGHTNING: 9,
    EXPLOSION: 10,
    EXPLOSION_NO_GLARE: 11,
    SPARK: 12,
    EDIT: 13,
    MUZZLE_FLASH1: 14,
    MUZZLE_FLASH2: 15,
    MUZZLE_FLASH3: 16,
    TEXT: 17,
    METER: 18,
    METER_VS: 19,
    LENS_FLARE: 20
};

BOUNCER = {
    GRENADE: 0,
    GIBS: 1,
    DEBRIS: 2,
    BARRELDEBRIS: 3,
};

//! Effect 'namespace'
Effect = {

    //! Places a decal - a temporary texture, applied onto normal world geometry. For example, a scorch mark after an explosion, etc.
    //! @param _type The type of decal, a constant of form DECAL_*.
    //! @param position Where to place the decal.
    //! @param direction The direction the decal faces (the decal is 'painted' on in this direction?).
    //! @param radius The size of the decal.
    //! @param color The color of the decal: (R,G,B).
    //! @param info Other decal-specific information (see Sauerbraten).
    addDecal: function (_type, position, direction, radius, color, info) {
        color = defaultValue(color, 0xFFFFFF);
        info = defaultValue(info, 0);

        var rgb = parseABC(color);

        CAPI.addDecal(_type, position.x, position.y, position.z, direction.x, direction.y, direction.z, radius, rgb[0], rgb[1], rgb[2], info);
    },



    //== Dynamic lights

    //! Add a dynamic light (that can move and so forth).
    //! @param position Where to place the light.
    //! @param radius How farthe light casts.
    //! @param color The color of the light: (R,G,B).
    //! @param fade How fast this light fades (seconds?).
    //! @param peak See Sauer (?).
    //! @param flags See Sauer.
    //! @param initradius See Sauer.
    //! @param initcolor See Sauer.
    addDynamicLight: function (position, radius, color, fade, peak, flags, initradius, initcolor) {
        var rgbColor = parseABC(color);
        var rgbInitcolor = parseABC(initcolor);
        fade = defaultValue(fade, 0);
        peak = defaultValue(peak, 0);
        flags = defaultValue(flags, 0);
        initradius = defaultValue(initradius, 0);
        initcolor = defaultValue(initcolor, new Vector3(0,0,0));
        CAPI.addDynlight(position.x, position.y, position.z, radius, rgbColor[0], rgbColor[1], rgbColor[2], integer(fade*1000), integer(peak*1000), flags, initradius, rgbInitcolor[0], rgbInitcolor[1], rgbInitcolor[2]);
    },

    //== Particle splashes

    //! Show a 'splash' effect, one of various types of simple particle effects consisting of a certain amount of particles animated in some
    //! manner. If done on the server, a message is sent to clients to show the effect.
    //! @param _type The type of the effect, one of PARTICLE_*.
    //! @param num The number of particles to show.
    //! @param fade (?) How fast to fade the effect into nothingness, in seconds.
    //! @param position Where to apply the effect.
    splash: function (_type, num, fade, position, color, size, radius, gravity) {
        if (Global.CLIENT) {
            color = defaultValue(color, 0xFFFFFF);
            size = defaultValue(size, 1.0);
            radius = defaultValue(radius, 150);
            gravity = defaultValue(gravity, 2);
            CAPI.particleSplash(_type, num, integer(fade*1000), position.x, position.y, position.z, color, size, radius, gravity);
        } else {
            // On server, send message to clients
            MessageSystem.send(MessageSystem.ALL_CLIENTS, CAPI.ParticleSplashToClients, _type, num, integer(fade*1000), position.x, position.y, position.z); // TODO: last 4 parameters
        }
    },


    //== Fireballs

    //! Show a simple fireball-type effect, i.e., an explosion.
    //! @param position Where to show the effect.
    //! @param _max Maximum size
    //! @param _type The specific type of fireball effect, one of PARTICLE_*_FIREBALL.
    //! @param fade (?) How fast to fade the effect into nothingness, in seconds
    fireball: function (_type, position, _max, fade, color, size) {
        if (fade !== undefined) {
            fade = integer(fade*1000);
        } else {
            fade = -1;
        }
        color = defaultValue(color, 0xFFFFFF);
        size = defaultValue(size, 4.0);

        CAPI.particleFireball(position.x, position.y, position.z, _max, _type, fade, color, size);
    },

    //== Flare type effects

    //! Show a flare effect, i.e., an effect with a start and an end point. For example, a flash of light between two points.
    //! @param start Where to start the effect.
    //! @param end Where to end the effect.
    //! @param _type The specific type of flare effect, PARTICLE_*_FLARE, or PARTICLE_*_LIGHTNING (others?).
    //! @param fade How fast to fade the effect into nothingness, in seconds
    flare: function (_type, end, start, fade, color, size) {
        if (fade === undefined) {
            fade = 0;
        } else {
            fade = integer(fade*1000);
        }
        color = defaultValue(color, 0xFFFFFF);
        size = defaultValue(size, 0.28);
        CAPI.particleFlare(start.x, start.y, start.z, end.x, end.y, end.z, fade, _type, color, size);
    },

    trail: function (_type, fade, from, to, color, size, gravity) {
        color = defaultValue(color, 0xFFFFFF);
        size = defaultValue(size, 1.0);
        gravity = defaultValue(gravity, 20);
        CAPI.particleTrail(_type, integer(fade*1000), from.x, from.y, from.z, to.x, to.y, to.z, color, size, gravity);
    },

    flame: function(_type, position, radius, height, color, density, scale, speed, fade, gravity) {
        density = defaultValue(density, 3);
        scale = defaultValue(scale, 2.0);
        speed = defaultValue(speed, 200.0);
        if (fade === undefined) {
            fade = 600.0;
        } else {
            fade = integer(fade*1000);
        }
        gravity = defaultValue(gravity, -15);

        CAPI.particleFlame(_type, position.x, position.y, position.z, radius, height, color, density, scale, speed, fade, gravity);
    },

    //! Show some animated lightning between two positions.
    //! @param start Where the lightning starts from. This should be a *fixed* position, if possible, as the animation depends on it,
    //! and will flicker otherwise.
    //! @param end Where the lightning ends. This can be a movable position, the animation will not flicker even if so.
    lightning: function (start, end, fade, color, size) {
        Effect.flare(PARTICLE.LIGHTNING, end, start, fade, color, size);
    },

    //== Other effects

    //! Show a meters, a type of billboard (non-3D effect, always faces the viewer). This is like a progress bar, and
    //! can be used to show health and so forth.
    //! @param position Where to show the meter.
    //! @param progress A float value between 0 and 1, indicating how much 'progress' to show on the meter.
    //! @param blueVsRed If true, show blue progress on a red background (?), otherwise the opposite.
    //! @param fade How fast to fade the effect into nothingness, in seconds(?)
    meterBillboard: function (position, progress, blueVsRed, fade) {
        fade = defaultValue(fade, 0.001);
        if (blueVsRed) {
            CAPI.particleMeter(position.x, position.y, position.z, progress, PARTICLE_FIXED_BLUE_VS_RED_METER, integer(fade*1000));
        } else {
            CAPI.particleMeter(position.x, position.y, position.z, progress, PARTICLE_FIXED_RED_VS_BLUE_METER, integer(fade*1000));
        }
    },

    //! Create a complex visual fireball-type explosion, consisting of both appropriate particle effects, a dynamic light,
    //! a scorch mark on the nearby ground, and TODO: debris as well (needs API change in C++).
    //! @param position Where to show the explosion
    //! @param direction TODO: Add this parameter, see comments in function
    fireballExplosion: function (position) {
        addDecal(DECAL_SCORCH, position, new Vector3(0, 0, 1), 20); // TODO: direction (0,0,1) is wrong. Should depend on
                                                                    // projectile, needs to be -1* its direction I believe
        particleSplash(PARTICLE_YELLOW_SPARKS, 200, 0.3, position);

        particleFireball(PARTICLE_RED_EXPLOSION, position, 40);

        addDynamicLight(position, 46, new Vector3(2, 1.5, 1), 0.9, 100, 0, 20, new Vector3(1, 0.75, 0.5));
    },

    text: function(position, text, fade, color, size, gravity) {
        fade = defaultValue(fade, 2.0);
        color = defaultValue(color, 0xFFFFFF);
        size = defaultValue(size, 2.0);
        gravity = defaultValue(gravity, 0);
        CAPI.particleText(position.x, position.y, position.z, text, PARTICLE.TEXT, integer(fade*1000), color, size, gravity);
    },
    //    spawn_debris(position, 10, self) TODO

    //! @param roll How much to jar the camera by rolling it to one side
    //! @param color How much coloration to blend into the camera
    clientDamage: function(roll, color) {
        if (Global.SERVER) return;

        CAPI.clientDamageEffect(roll, color);
    },

    //! XXX Deprecated
    //! Generates some flying debris.
    //! @param position Where to start the flying debris.
    //! @param num How many pieces of debris to toss around.
    //! @param owner The entity responsible for this debris (so it doesn't hit that entity?).
    //! @param vel A velocity added to all debris, in addition to their own normal random velocities. Useful for debris flying around
    //! a moving position (?).
    debris: function(_type, position, num, owner, velocity) {
        velocity = defaultValue(velocity, new Vector3(0,0,0));
        CAPI.spawnDebris(_type, position.x, position.y, position.z, num, velocity.x, velocity.y, velocity.z, owner.uniqueId);
    }

};

