
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

#include <cmath>

#ifndef WIN32
    #define ISNAN(x) std::isnan(x)
#else
    #define ISNAN(x) _isnan(x)
#endif

#ifdef CLIENT
    #include "client_engine_additions.h"
    #include "intensity_gui.h"
    #include "intensity_texture.h"
#endif

#ifdef SERVER
    #include "NPC.h"
#endif

#include "intensity_physics.h"

#define RETURN_VECTOR3(sauervec) \
    ScriptValuePtr __ret_position = ScriptEngineManager::getGlobal()->call("__new__", \
        ScriptValueArgs().append(ScriptEngineManager::getGlobal()->getProperty("Vector3")) \
            .append(sauervec.x) \
            .append(sauervec.y) \
            .append(sauervec.z) \
    ); \
    V8_RETURN_VALUE(__ret_position);


// Worldsystem
extern void removeentity(extentity* entity);
extern void addentity(extentity* entity);

// Embedded functions. We do the wrapping in this way, so it is fast (no
// unnecessary runtime conversions). This is still quite abstracted,
// however, and easy to switch to another engine from.

// XXX XXX XXX  This file contains embedded functions, i.e., the interface
// between trusted and untrusted code. It must be thoroughly and often
// audited for security, as well as functions it calls! XXX XXX XXX

// Logging - NOTE: This is not in the *_impl.h file, we do it manually.
// That is because logging is done before anything else - it is important
// during initialization

V8_FUNC_is(__script__log, { Logging::log_noformat(arg1, arg2); } );


//
// Normal CAPI
//

// General

V8_FUNC_NOPARAM(__script__currTime, { V8_RETURN_DOUBLE( Utility::SystemInfo::currTime() ); });

// Entity attribs

V8_FUNC_T(__script__setAnimation, i, { self.get()->setAnimation(arg2); } );
V8_FUNC_T(__script__getStartTime, , {
    V8_RETURN_INT( self.get()->getStartTime() );
});
V8_FUNC_T(__script__setModelName, s, {
    Logging::log(Logging::DEBUG, "__script__setModelName(%s)\r\n", arg2);
    self.get()->setModel(arg2);
} );
V8_FUNC_T(__script__setAttachments_raw, s, { self.get()->setAttachments(arg2); } );
V8_FUNC_T(__script__getAttachmentPosition, s, {
    vec& vposition = self->getAttachmentPosition(arg2);
    RETURN_VECTOR3(vposition);
});

V8_FUNC_T(__script__setCanMove, i, { self.get()->setCanMove(arg2); } );

// Entity management

//V8_FUNC_i(__script__registerLogicEntityNonSauer, { LogicSystem::registerLogicEntityNonSauer(arg1); } ); DEPRECATED
V8_FUNC_i(__script__unregisterLogicEntity, { LogicSystem::unregisterLogicEntityByUniqueId(arg1); } );

V8_FUNC_ii(__script__placeInWorld, { WorldSystem::placeInWorld(arg1, arg2); } );

V8_FUNC_Z(__script__setupExtent, idddiiii, { LogicSystem::setupExtent(self, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9); } );
V8_FUNC_Z(__script__setupCharacter, , { LogicSystem::setupCharacter(self); } );
V8_FUNC_Z(__script__setupNonSauer, , { LogicSystem::setupNonSauer(self); } );

V8_FUNC_Z(__script__dismantleExtent, , { LogicSystem::dismantleExtent(self); } );
V8_FUNC_Z(__script__dismantleCharacter, , { LogicSystem::dismantleCharacter(self); } );

// Sounds

#ifdef CLIENT
    V8_FUNC_sdddi(__script_playSoundByName, {
        vec loc(arg2, arg3, arg4);
        if (loc.x || loc.y || loc.z)
            playsoundname(arg1, &loc, arg5);
        else
            playsoundname(arg1);
    });
#endif

#ifdef CLIENT
V8_FUNC_s(__script__music, {
    assert( Utility::validateAlphaNumeric(arg1, "._/") );
    std::string command = "music \"";
    command += arg1;
    command += "\" [ run_script \"Sound.musicCallback()\" ]";
    execute(command.c_str());
});
#else
V8_FUNC_s(__script__music, {
    arg1 = arg1; // warning otherwise
});
#endif

VAR(tempNewSound, 0, 0, 65535);

#ifdef CLIENT
extern int preload_sound(char *name, int vol);
V8_FUNC_si(__script__preloadSound, {
    std::string str = "preloading sound '";
    str += arg1;
    str += "'...";
    renderprogress(0, str.c_str());

    arg2 = min(arg2, 100); // Do not let scripts set high volumes for griefing
    V8_RETURN_INT(preload_sound((char*)arg1, arg2));
});
#else
V8_FUNC_si(__script__preloadSound, {
    arg1 = arg1; arg2 = arg2; // warning otherwise
});
#endif

#ifdef CLIENT
V8_FUNC_i(__script__playSound, {
    playsound(arg1); // TODO: Sound position
});
#else
V8_FUNC_i(__script__playSound, {
    MessageSystem::send_SoundToClients(
        -1,
        arg1,
        -1
    );
});
#endif

// Extents

#define EXTENT_ACCESSORS(getterName, setterName, attribName) \
V8_FUNC_T(__script__##getterName, , { \
    extentity* e = self.get()->staticEntity; \
    assert(e); \
    V8_RETURN_INT(e->attribName); \
}); \
 \
V8_FUNC_T(__script__##setterName, i, { \
    extentity* e = self.get()->staticEntity; \
    assert(e); \
    if (!WorldSystem::loadingWorld) removeentity(e); /* Need to remove, then add, to the world on each change, if not during load. */ \
    e->attribName = arg2; \
    if (!WorldSystem::loadingWorld) addentity(e); \
}); \
 \
V8_FUNC_T(__script__FAST_##setterName, i, { /* Fast version - no removeentity/addentity. Use with care! */ \
    extentity* e = self.get()->staticEntity; \
    assert(e); \
    e->attribName = arg2; \
});

EXTENT_ACCESSORS(getAttr1, setAttr1, attr1);
EXTENT_ACCESSORS(getAttr2, setAttr2, attr2);
EXTENT_ACCESSORS(getAttr3, setAttr3, attr3);
EXTENT_ACCESSORS(getAttr4, setAttr4, attr4);

#define EXTENT_LE_ACCESSORS(getterName, setterName, attribName) \
V8_FUNC_T(__script__##getterName, , { \
    V8_RETURN_DOUBLE(self->attribName); \
}); \
 \
V8_FUNC_T(__script__##setterName, d, { \
    Logging::log(Logging::DEBUG, "ACCESSOR: Setting %s to %d\r\n", #setterName, arg2); \
    assert(self->staticEntity); \
    if (!WorldSystem::loadingWorld) removeentity(self->staticEntity); /* Need to remove, then add, to the octa world on each change. */ \
    self->attribName = arg2; \
    if (!WorldSystem::loadingWorld) addentity(self->staticEntity); \
});

EXTENT_LE_ACCESSORS(getCollisionRadiusWidth, setCollisionRadiusWidth, collisionRadiusWidth);
EXTENT_LE_ACCESSORS(getCollisionRadiusHeight, setCollisionRadiusHeight, collisionRadiusHeight);


//Add 'FAST' versions of accessors - no addeneity/removeentity. Good to change e.g. particle parameters


V8_FUNC_T(__script__getExtentO_raw, i, {
    extentity* e = self.get()->staticEntity;
    assert(e);
    assert(arg2 >= 0 && arg2 <= 2);

    Logging::log(Logging::INFO, "__script__getExtentO_raw(%d): %f\r\n", arg2, e->o[arg2]);

    V8_RETURN_DOUBLE(e->o[arg2]);
});

V8_FUNC_T(__script__setExtentO_raw, ddd, {
    extentity* e = self.get()->staticEntity;
    assert(e);

    removeentity(e); /* Need to remove, then add, to the octa world on each change. */
    e->o.x = arg2;
    e->o.y = arg3;
    e->o.z = arg4;
    addentity(e);
});


// Dynents

#define DYNENT_ACCESSORS(getterName, setterName, type_code, type_BOLD, attribName) \
V8_FUNC_T(__script__##getterName, , { \
    fpsent* e = (fpsent*)(self.get()->dynamicEntity); \
    assert(e); \
    V8_RETURN_##type_BOLD(e->attribName); \
}); \
 \
V8_FUNC_T(__script__##setterName, type_code, { \
    fpsent* e = dynamic_cast<fpsent*>(self.get()->dynamicEntity); \
    assert(e); \
    e->attribName = arg2; \
});

DYNENT_ACCESSORS(getMaxSpeed, setMaxSpeed, d, DOUBLE, maxspeed);
DYNENT_ACCESSORS(getRadius, setRadius, d, DOUBLE, radius);

DYNENT_ACCESSORS(getEyeHeight, setEyeHeight, d, DOUBLE, eyeheight);
DYNENT_ACCESSORS(getAboveeye, setAboveeye, d, DOUBLE, aboveeye);
DYNENT_ACCESSORS(getYaw, setYaw, d, DOUBLE, yaw);
DYNENT_ACCESSORS(getPitch, setPitch, d, DOUBLE, pitch);
DYNENT_ACCESSORS(getMove, setMove, i, INT, move);
DYNENT_ACCESSORS(getStrafe, setStrafe, i, INT, strafe);
DYNENT_ACCESSORS(getYawing, setYawing, i, INT, turn_move);
DYNENT_ACCESSORS(getPitching, setPitching, i, INT, look_updown_move);
DYNENT_ACCESSORS(getJumping, setJumping, b, BOOL, jumping);
DYNENT_ACCESSORS(getBlocked, setBlocked, b, BOOL, blocked);
DYNENT_ACCESSORS(getMapDefinedPositionData, setMapDefinedPositionData, i, INT, mapDefinedPositionData); // XXX Should be unsigned
DYNENT_ACCESSORS(getClientState, setClientState, i, INT, state);
DYNENT_ACCESSORS(getPhysicalState, setPhysicalState, i, INT, physstate);
DYNENT_ACCESSORS(getInWater, setInWater, i, INT, inwater);
DYNENT_ACCESSORS(getTimeInAir, setTimeInAir, i, INT, timeinair);

// For dynents, 'o' is at their head, not their feet like static entities. We make this uniform by
// letting scripting specify a feet position, and we work relative to their height - add to
// assignments, subtract from readings
V8_FUNC_T(__script__getDynentO_raw, i, {
    fpsent* d = dynamic_cast<fpsent*>(self.get()->dynamicEntity);
    assert(d);
    assert(arg2 >= 0 && arg2 <= 2);

    if (arg2 != 2) {
        V8_RETURN_DOUBLE(d->o[arg2]);
    } else {
        V8_RETURN_DOUBLE(d->o.z - d->eyeheight);// - d->aboveeye);
    }
});

V8_FUNC_T(__script__setDynentO_raw, ddd, {
    fpsent* d = dynamic_cast<fpsent*>(self.get()->dynamicEntity);
    assert(d);

    d->o.x = arg2;
    d->o.y = arg3;
    d->o.z = arg4 + d->eyeheight;// + d->aboveeye;

    // Also set 'newpos', otherwise this change may get overwritten
    d->newpos = d->o;

    d->resetinterp(); // No need to interpolate to last position - just jump

    Logging::log(Logging::INFO, "(%d).setDynentO(%f, %f, %f)\r\n", d->uniqueId, d->o.x, d->o.y, d->o.z);
});

V8_FUNC_T(__script__getDynentVel_raw, i, {
    fpsent* d = (fpsent*)(self.get()->dynamicEntity);
    assert(d);
    assert(arg2 >= 0 && arg2 <= 2);

    V8_RETURN_DOUBLE(d->vel[arg2]);
});

V8_FUNC_T(__script__setDynentVel_raw, ddd, {
    fpsent* d = dynamic_cast<fpsent*>(self.get()->dynamicEntity);
    assert(d);

    d->vel.x = arg2;
    d->vel.y = arg3;
    d->vel.z = arg4;
});

V8_FUNC_T(__script__getDynentFalling_raw, i, {
    fpsent* d = (fpsent*)(self.get()->dynamicEntity);
    assert(d);
    assert(arg2 >= 0 && arg2 <= 2);

    V8_RETURN_DOUBLE(d->falling[arg2]);
});

V8_FUNC_T(__script__setDynentFalling_raw, ddd, {
    fpsent* d = dynamic_cast<fpsent*>(self.get()->dynamicEntity);
    assert(d);

    d->falling.x = arg2;
    d->falling.y = arg3;
    d->falling.z = arg4;
});

// Geometry utilities

V8_FUNC_dddddd(__script__rayLos, {
    vec a(arg1, arg2, arg3);
    vec b(arg4, arg5, arg6);
    vec target;

    bool ret = raycubelos(a, b, target);
    V8_RETURN_BOOL(ret);
});

V8_FUNC_ddddddd(__script__rayPos, {
    vec o(arg1, arg2, arg3);
    vec ray(arg4, arg5, arg6);
    vec hitpos(0);

    V8_RETURN_DOUBLE(raycubepos(o, ray, hitpos, arg7, RAY_CLIPMAT|RAY_POLY));
});

V8_FUNC_dddd(__script__rayFloor, {
    vec o(arg1, arg2, arg3);
    vec floor(0);
    V8_RETURN_DOUBLE(rayfloor(o, floor, 0, arg4));
});

// Effects

#ifdef CLIENT

    V8_FUNC_idddddddiiii(__script__addDecal, {
        vec  center(arg2, arg3, arg4);
        vec  surface(arg5, arg6, arg7);
        bvec color(arg9, arg10, arg11);

        adddecal(arg1, center, surface, arg8, color, arg12);
    });

    VARP(blood, 0, 1, 1);

    V8_FUNC_iiidddidii(__script__particleSplash, {
        if (arg1 == PART_BLOOD && !blood) V8_RETURN_NULL;
        vec p(arg4, arg5, arg6);
        particle_splash(arg1, arg2, arg3, p, arg7, arg8, arg9, arg10);
    });

    V8_FUNC_ddddiiid(__script__particleFireball, {
        vec dest(arg1, arg2, arg3);
        particle_fireball(dest, arg4, arg5, arg6, arg7, arg8);
    });

    V8_FUNC_ddddddiiid(__script__particleFlare, {
        vec p(arg1, arg2, arg3);
        vec dest(arg4, arg5, arg6);
        particle_flare(p, dest, arg7, arg8, arg9, arg10);
    });

    V8_FUNC_iiddddddidi(__script__particleTrail, {
        vec from(arg3, arg4, arg5);
        vec to(arg6, arg7, arg8);
        particle_trail(arg1, arg2, from, to, arg9, arg10, arg11);
    });

    extern void regularflame(int type, const vec &p, float radius, float height, int color, int density = 3, float scale = 2.0f, float speed = 200.0f, float fade = 600.0f, int gravity = -15);
    V8_FUNC_idddddiidddi(__script__particleFlame, {
        regularflame(arg1, vec(arg2, arg3, arg4), arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12);
    });

    V8_FUNC_dddddddiiidddd(__script__addDynlight, {
        vec o(arg1, arg2, arg3);
        vec color(float(arg5)/255.0, float(arg6)/255.0, float(arg7)/255.0);
        vec initcolor(float(arg12)/255.0, float(arg13)/255.0, float(arg14)/255.0);

        LightControl::queueDynamicLight(o, arg4, color, arg8, arg9, arg10, arg11, initcolor, NULL);
    });

    V8_FUNC_idddidddi(__script__spawnDebris, {
        vec v(arg2, arg3, arg4);
        vec debrisvel(arg6, arg7, arg8);

        LogicEntityPtr owner = LogicSystem::getLogicEntity(arg9);
        assert(owner->dynamicEntity);
        FPSClientInterface::spawnDebris(arg1, v, arg5, debrisvel, (dynent*)(owner->dynamicEntity));
    });

    V8_FUNC_ddddii(__script__particleMeter, {
        vec s(arg1, arg2, arg3);

        particle_meter(s, arg4, arg5, arg6);
    });

    V8_FUNC_dddsiiidi(__script__particleText, {
        vec s(arg1, arg2, arg3);
        std::string safeString = std::string("@") + arg4; // Make sauer copy this, as it will not persist
        particle_text(s, safeString.c_str(), arg5, arg6, arg7, arg8, arg9);
    });

    V8_FUNC_ii(__script__clientDamageEffect, {
        dynamic_cast<fpsent*>(player)->damageroll(arg1);
        damageblend(arg2);
    });

    V8_FUNC_ddddi(__script__showHUDRect, { ClientSystem::addHUDRect(arg1, arg2, arg3, arg4, arg5); });

    V8_FUNC_sdddd(__script__showHUDImage, { ClientSystem::addHUDImage(arg1, arg2, arg3, arg4, arg5); });

    V8_FUNC_sdddi(__script__showHUDText, {
        // text, x, y, scale, color
        ClientSystem::addHUDText(arg1, arg2, arg3, arg4, arg5);
    });

#endif // CLIENT

// Messages

using namespace MessageSystem;
V8_FUNC_iiss(__script__PersonalServerMessage, { send_PersonalServerMessage(arg1, arg2, arg3, arg4); });
V8_FUNC_iiiiddd(__script__ParticleSplashToClients, { send_ParticleSplashToClients(arg1, arg2, arg3, arg4, arg5, arg6, arg7); });
V8_FUNC_idddsi(__script__SoundToClientsByName, { send_SoundToClientsByName(arg1, arg2, arg3, arg4, arg5, arg6); });
V8_FUNC_iis(__script__StateDataChangeRequest, { send_StateDataChangeRequest(arg1, arg2, arg3); });
V8_FUNC_iis(__script__UnreliableStateDataChangeRequest, { send_UnreliableStateDataChangeRequest(arg1, arg2, arg3); });
V8_FUNC_ii(__script__NotifyNumEntities, { send_NotifyNumEntities(arg1, arg2); });
V8_FUNC_iiiss(__script__LogicEntityCompleteNotification, { send_LogicEntityCompleteNotification(arg1, arg2, arg3, arg4, arg5); });
V8_FUNC_ii(__script__LogicEntityRemoval, { send_LogicEntityRemoval(arg1, arg2); });
V8_FUNC_iiisi(__script__StateDataUpdate, { send_StateDataUpdate(arg1, arg2, arg3, arg4, arg5); });
V8_FUNC_iiisi(__script__UnreliableStateDataUpdate, { send_UnreliableStateDataUpdate(arg1, arg2, arg3, arg4, arg5); });
V8_FUNC_iidddi(__script__DoClick, { send_DoClick(arg1, arg2, arg3, arg4, arg5, arg6); });
V8_FUNC_iissdddiiii(__script__ExtentCompleteNotification, { send_ExtentCompleteNotification(arg1, arg2, arg3,arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11); });


// TODO

// Projectiles

/*
// Consider doing this entirely in Scripting. Need a way to do position updates in scripting, i.e.,
// to supress the normal updates for an entity.
void shootv_helper(int gun, float ox, float oy, float oz, float dx, float dy, float dz, python::object onHit, int addr)
{
    vec origin(ox, oy, oz), destination(dx, dy, dz);

    // The following is the algorithm from weapon.h, or close to it. The idea is that, since world geometry doesn't change,
    // we can calculate IN ADVANCE whether the proejctile will hit the world. This is fine, but is *wrong* for dynamic
    // per-frame mapmodels. So perhaps TODO work out a fix for that.

    vec unitv;
    float dist = destination.dist(origin, unitv);
    unitv.div(dist);

    float barrier = raycube(origin, unitv, dist, RAY_CLIPMAT|RAY_ALPHAPOLY);
    if(barrier < dist)
    {
        destination = unitv;
        destination.mul(barrier);
        destination.add(origin);
    }
        
    #ifdef CLIENT //                    from    to
        FPSClientInterface::shootV(gun, origin, destination, (dynent*)addr, false, onHit);
    #else // SERVER
        FPSClientInterface::shootV(gun, origin, destination, (dynent*)addr, true,  onHit);
    #endif
}
*/

// File access

V8_FUNC_s(__script__readFile, {
    try
    {
        REFLECT_PYTHON( read_file_safely );

        boost::python::object data = read_file_safely(arg1);
        std::string text = boost::python::extract<std::string>(data);

        V8_RETURN_STRING( text.c_str() );
    }
    catch(boost::python::error_already_set const &)
    {
        printf("Error in Python execution of embedded read_file_safely\r\n");
        PyErr_Print();
        assert(0 && "Halting on Python error");
    }
});

// Mapping

extern void texturereset(int *n);
V8_FUNC_NOPARAM(__script__textureReset, {
    int num = 0;
    texturereset(&num);
});

extern void texture(char *type, char *name, int *rot, int *xoffset, int *yoffset, float *scale, int *forcedindex);
V8_FUNC_ssiiidi(__script__texture, {
    float arg6f = arg6;
    // XXX: arg7 may not be given, in which case it is undefined, and turns into 0.
    texture((char*)arg1, (char*)arg2, &arg3, &arg4, &arg5, &arg6f, &arg7);
});

extern void mapmodelreset();
V8_FUNC_NOPARAM(__script__mapmodelReset, {
    mapmodelreset();
});

extern void mmodel(char *name);
V8_FUNC_s(__script__mapmodel, {
    mmodel((char*)arg1);
});

extern void autograss(char *name);
V8_FUNC_s(__script__autograss, {
    autograss((char*)arg1);
});

extern void texlayer(int *layer, char *name, int *mode, float *scale);
#ifdef CLIENT
V8_FUNC_i(__script__texLayer, {
    int dummy1 = 0;
    float dummy2 = 0;
    texlayer(&arg1, (char*)"", &dummy1, &dummy2);
});
#else
V8_FUNC_i(__script__texLayer, {
    arg1 = arg1; // warning otherwise
});
#endif

#ifdef CLIENT
V8_FUNC_s(__script__setShader, {
    std::string command = "setshader ";
    command += arg1;
    assert( Utility::validateAlphaNumeric(arg1) );
    execute(command.c_str());
});
#else
V8_FUNC_s(__script__setShader, {
    arg1 = arg1; // warning otherwise
});
#endif

#ifdef CLIENT
V8_FUNC_sdddd(__script__setShaderParam, {
    std::string command = "setshaderparam ";
    command += arg1;
    assert( Utility::validateAlphaNumeric(arg1) );
    command += " " + Utility::toString((float)arg2);
    command += " " + Utility::toString((float)arg3);
    command += " " + Utility::toString((float)arg4);
    command += " " + Utility::toString((float)arg5);
    execute(command.c_str());
});
#else
V8_FUNC_sdddd(__script__setShaderParam, {
    arg1 = arg1; // warning otherwise
    arg2 = arg2; // warning otherwise
    arg3 = arg3; // warning otherwise
    arg4 = arg4; // warning otherwise
    arg5 = arg5; // warning otherwise
});
#endif

extern void materialreset();
V8_FUNC_NOPARAM(__script__materialReset, {
    materialreset();
});

#ifdef CLIENT
V8_FUNC_s(__script__loadSky, {
    std::string command = "loadsky ";
    command += arg1;
    assert( Utility::validateAlphaNumeric(arg1, "/_<>:.,") );
    execute(command.c_str());
});
#else
V8_FUNC_s(__script__loadSky, {
    arg1 = arg1; // warning otherwise
});
#endif

#ifdef CLIENT
V8_FUNC_s(__script__fogColor, {
    std::string command = "fogcolour ";
    command += arg1;
    assert( Utility::validateAlphaNumeric(arg1) );
    execute(command.c_str());
});
#else
V8_FUNC_s(__script__fogColor, {
    arg1 = arg1; // warning otherwise
});
#endif

#ifdef CLIENT
V8_FUNC_i(__script__fog, {
    std::string command = "fog ";
    command += Utility::toString(arg1);
    execute(command.c_str());
});
#else
V8_FUNC_i(__script__fog, {
    arg1 = arg1; // warning otherwise
});
#endif

#ifdef CLIENT
V8_FUNC_i(__script__waterFog, {
    std::string command = "waterfog ";
    command += Utility::toString(arg1);
    execute(command.c_str());
});
#else
V8_FUNC_i(__script__waterFog, {
    arg1 = arg1; // warning otherwise
});
#endif

#ifdef CLIENT
V8_FUNC_iii(__script__waterColor, {
    std::string command = "watercolour ";
    command += Utility::toString(arg1) + " ";
    command += Utility::toString(arg2) + " ";
    command += Utility::toString(arg3) + " ";
    execute(command.c_str());
});
#else
V8_FUNC_iii(__script__waterColor, {
    arg1 = arg1; arg2 = arg2; arg3 = arg3; // warning otherwise
});
#endif

#ifdef CLIENT
V8_FUNC_d(__script__spinSky, {
    std::string command = "spinsky ";
    command += Utility::toString(arg1);
    execute(command.c_str());
});
#else
V8_FUNC_d(__script__spinSky, {
    arg1 = arg1; // warning otherwise
});
#endif

#ifdef CLIENT
V8_FUNC_s(__script__cloudLayer, {
    std::string command = "cloudlayer ";
    assert(Utility::validateRelativePath(arg1));
    command += Utility::toString(arg1);
    execute(command.c_str());
});
#else
V8_FUNC_s(__script__cloudLayer, {
    arg1 = arg1; // warning otherwise
});
#endif


#ifdef CLIENT
V8_FUNC_d(__script__cloudScrollX, {
    std::string command = "cloudscrollx ";
    command += Utility::toString(arg1);
    execute(command.c_str());
});
#else
V8_FUNC_d(__script__cloudScrollX, {
    arg1 = arg1; // warning otherwise
});
#endif

#ifdef CLIENT
V8_FUNC_d(__script__cloudScrollY, {
    std::string command = "cloudscrolly ";
    command += Utility::toString(arg1);
    execute(command.c_str());
});
#else
V8_FUNC_d(__script__cloudScrollY, {
    arg1 = arg1; // warning otherwise
});
#endif

#ifdef CLIENT
V8_FUNC_d(__script__cloudScale, {
    std::string command = "cloudscale ";
    command += Utility::toString(arg1);
    execute(command.c_str());
});
#else
V8_FUNC_d(__script__cloudScale, {
    arg1 = arg1; // warning otherwise
});
#endif

#ifdef CLIENT
V8_FUNC_i(__script__skyTexture, {
    std::string command = "skytexture ";
    command += Utility::toString(arg1);
    execute(command.c_str());
});
#else
V8_FUNC_i(__script__skyTexture, {
    arg1 = arg1; // warning otherwise
});
#endif

#ifdef CLIENT
V8_FUNC_dd(__script__texScroll, {
    std::string command = "texscroll ";
    command += Utility::toString(arg1) + " ";
    command += Utility::toString(arg2);
    execute(command.c_str());
});
#else
V8_FUNC_dd(__script__texScroll, {
    arg1 = arg1; arg2 = arg2; // warning otherwise
});
#endif

#ifdef CLIENT
V8_FUNC_i(__script__shadowmapAngle, {
    std::string command = "shadowmapangle ";
    command += Utility::toString(arg1);
    execute(command.c_str());
});
#else
V8_FUNC_i(__script__shadowmapAngle, {
    arg1 = arg1; // warning otherwise
});
#endif

#ifdef CLIENT
V8_FUNC_s(__script__shadowmapAmbient, {
    assert( Utility::validateAlphaNumeric(arg1, "x") ); // Allow e.g. 0xFFA033
    std::string command = "shadowmapambient ";
    command += arg1;
    execute(command.c_str());
});
#else
V8_FUNC_s(__script__shadowmapAmbient, {
    arg1 = arg1; // warning otherwise
});
#endif

#ifdef CLIENT
V8_FUNC_iii(__script__skylight, {
    std::string command = "skylight ";
    command += Utility::toString(arg1) + " " + Utility::toString(arg2) + " " + Utility::toString(arg3);
    execute(command.c_str());
});
#else
V8_FUNC_iii(__script__skylight, {
    arg1 = arg1; // warning otherwise
    arg2 = arg2; // warning otherwise
    arg3 = arg3; // warning otherwise
});
#endif

#ifdef CLIENT
V8_FUNC_i(__script__blurSkylight, {
    std::string command = "blurskylight ";
    command += Utility::toString(arg1);
    execute(command.c_str());
});
#else
V8_FUNC_i(__script__blurSkylight, {
    arg1 = arg1; // warning otherwise
});
#endif

#ifdef CLIENT
V8_FUNC_i(__script__ambient, {
    std::string command = "ambient ";
    command += Utility::toString(arg1);
    execute(command.c_str());
});
#else
V8_FUNC_i(__script__ambient, {
    arg1 = arg1; // warning otherwise
});
#endif

V8_FUNC_s(__script__preloadModel, { preloadmodel(arg1); });

V8_FUNC_s(__script__reloadModel, {
    extern void clearmodel(char *name);
    clearmodel((char*)arg1);
    try
    {
        loadmodel((char*)arg1);
    } catch (ScriptException& error)
    {
        ScriptValuePtr ret = ScriptEngineManager::createScriptObject();
        ret->setProperty("error", error.text);
        V8_RETURN_VALUE(ret);
    }
});

#ifdef USE_JPEG2000
#ifdef CLIENT
V8_FUNC_ss(__script__convertJP2toPNG, {
    assert(Utility::validateRelativePath(arg1));
    assert(Utility::validateRelativePath(arg2));
    IntensityTexture::convertJP2toPNG(arg1, arg2);
});
#else
V8_FUNC_ss(__script__convertJP2toPNG, {
    arg1 = arg1; arg2 = arg2; // warning otherwise
});
#endif
#endif

#ifdef CLIENT
V8_FUNC_ss(__script__convertPNGtoDDS, {
    assert(Utility::validateRelativePath(arg1));
    assert(Utility::validateRelativePath(arg2));
    IntensityTexture::convertPNGtoDDS(arg1, arg2);
});
#else
V8_FUNC_ss(__script__convertPNGtoDDS, {
    arg1 = arg1; arg2 = arg2; // warning otherwise
});
#endif

#ifdef CLIENT
V8_FUNC_sss(__script__combineImages, {
    assert(Utility::validateRelativePath(arg1));
    assert(Utility::validateRelativePath(arg2));
    assert(Utility::validateRelativePath(arg3));
    IntensityTexture::combineImages(arg1, arg2, arg3);
});
#else
V8_FUNC_sss(__script__combineImages, {
    arg1 = arg1; arg2 = arg2; arg3 = arg3; // warning otherwise
});
#endif

// HUD

#ifdef CLIENT
    V8_FUNC_NOPARAM(__script__getTargetPosition, {
        TargetingControl::determineMouseTarget(true); // Force a determination, if needed

        RETURN_VECTOR3(TargetingControl::targetPosition);
    });

    V8_FUNC_NOPARAM(__script__getTargetEntity, {
        TargetingControl::determineMouseTarget(true); // Force a determination, if needed

        if (TargetingControl::targetLogicEntity.get() && !TargetingControl::targetLogicEntity->isNone())
        {
            V8_RETURN_VALUE(TargetingControl::targetLogicEntity->scriptEntity);
        } else {
            V8_RETURN_NULL;
        }
    });
#endif

// World

V8_FUNC_ddddi(__script__isColliding, {
    vec position(arg1, arg2, arg3);
    V8_RETURN_BOOL( WorldSystem::isColliding(
        position,
        arg4,
        arg5 != -1 ? LogicSystem::getLogicEntity(arg5).get() : NULL)
    ); // TODO: Make faster, avoid this lookup
});

V8_FUNC_d(__script__setGravity, {
    extern float GRAVITY;
    GRAVITY = arg1;
});

V8_FUNC_ddd(__script__getMaterial, {
    V8_RETURN_INT(lookupmaterial(vec(arg1, arg2, arg3)));
});

// NPCs/bots

#ifdef SERVER
    V8_FUNC_s(__script__addNPC, {
        ScriptValuePtr ret = NPC::add(arg1);
        V8_RETURN_VALUE(ret);
    });

    V8_FUNC_T(__script__removeNPC, , {
        fpsent* fpsEntity = (fpsent*)self->dynamicEntity;
        NPC::remove(fpsEntity->clientnum);
    });
#endif

// Rendering

#ifdef CLIENT
    VARP(ragdoll, 0, 1, 1);
    static int oldThirdperson = -1;

    V8_FUNC_T(__script__renderModel2, siddddddii, {

        int anim = arg3;
        if (anim&ANIM_RAGDOLL)
        {
//            if (!ragdoll || loadmodel(mdl);
            fpsent* fpsEntity = (fpsent*)self->dynamicEntity;

            if (fpsEntity->clientnum == ClientSystem::playerNumber)
            {

                if (oldThirdperson == -1 && thirdperson == 0)
                {
                    oldThirdperson = thirdperson;
                    thirdperson = 1;
                }
            }

            if (fpsEntity->ragdoll || !ragdoll)
            {
                anim &= ~ANIM_RAGDOLL;
                self->scriptEntity->call("setLocalAnimation", anim); // Set new animation locally - in state data and C++
            }
        } else {
            if (self->dynamicEntity)
            {
                fpsent* fpsEntity = (fpsent*)self->dynamicEntity;

                if (fpsEntity->clientnum == ClientSystem::playerNumber && oldThirdperson != -1)
                {
                    thirdperson = oldThirdperson;
                    oldThirdperson = -1;
                }
            }
        }

        vec o(arg4, arg5, arg6);

        fpsent *fpsEntity = NULL;

        if (self->dynamicEntity)
            fpsEntity = dynamic_cast<fpsent*>(self->dynamicEntity);
        else
        {
            if (self->scriptEntity->hasProperty("renderingHashHint"))
            {
                static bool initialized = false;
                static fpsent* fpsEntitiesForRendering[1024];

                if (!initialized)
                {
                    for (int i = 0; i < 1024; i++)
                        fpsEntitiesForRendering[i] = new fpsent;

                    initialized = true;
                }

                int renderingHashHint = self->scriptEntity->getPropertyInt("renderingHashHint");
                renderingHashHint = renderingHashHint & 1023;
                assert(renderingHashHint >= 0 && renderingHashHint < 1024);
                fpsEntity = fpsEntitiesForRendering[renderingHashHint];
            }
        }
        rendermodel(NULL, arg2, anim, o, self, arg7, arg8, arg9, arg10, fpsEntity, self->attachments, arg11);
    });

    V8_FUNC_T(__script__renderModel3, siddddddiidddd, {
        vec o(arg4, arg5, arg6);
        fpsent *fpsEntity = NULL;
        if (self->dynamicEntity)
            fpsEntity = dynamic_cast<fpsent*>(self->dynamicEntity);
        quat rotation(arg12, arg13, arg14, arg15);
        rendermodel(NULL, arg2, arg3, o, self, arg7, arg8, arg9, arg10, fpsEntity, self->attachments, arg11, 0, 1, rotation);
    });

#endif

// GUI

#ifdef CLIENT
    V8_FUNC_s(__script__showMessage__, {
        IntensityGUI::showMessage("Script message", arg1);
    });

    V8_FUNC_s(__script__showInputDialog__, {
        IntensityGUI::showInputDialog("Script input", arg1);
    });

    V8_FUNC_i(__script__setDefaultThirdpersonMode, {
        // Only allow this to be done once
        if (ScriptEngineManager::engineParameters.count("setDefaultThirdpersonMode") == 0)
        {
            ScriptEngineManager::engineParameters["setDefaultThirdpersonMode"] = "set";
            thirdperson = arg1;
        } else
            Logging::log(Logging::WARNING, "Can only set default thirdperson mode once per map\r\n");
    });
#endif

// Network

#ifdef CLIENT
    V8_FUNC_si(__script__connect__, {
        ClientSystem::connect(arg1, arg2);
    });
#endif

// Camera

#ifdef CLIENT
    V8_FUNC_ddddddd(__script__forceCamera__, {
        vec position(arg1, arg2, arg3);
        CameraControl::forceCamera(position, arg4, arg5, arg6, arg7);
    });

    V8_FUNC_NOPARAM(__script__getCamera__, {
        physent *camera = CameraControl::getCamera();

        ScriptValuePtr ret = ScriptEngineManager::createScriptObject();

        ret->setProperty("position", ScriptEngineManager::getGlobal()->call("__new__",
            ScriptValueArgs().append(ScriptEngineManager::getGlobal()->getProperty("Vector3"))
                .append(camera->o.x)
                .append(camera->o.y)
                .append(camera->o.z)
        ));
        ret->setProperty("yaw", camera->yaw);
        ret->setProperty("pitch", camera->pitch);
        ret->setProperty("roll", camera->roll);

        V8_RETURN_VALUE(ret);
    });
#endif

// Code

V8_FUNC_ss(__script__compile__, {
    ScriptEngineManager::runScriptNoReturn(arg1, arg2);
});

// Components

V8_FUNC_ss(__script__signalComponent__, {
    try
    {
        REFLECT_PYTHON( signal_signal_component );

        boost::python::object data = signal_signal_component(arg1, arg2);
        std::string stringData = boost::python::extract<std::string>(data);
        V8_RETURN_STRING( stringData.c_str() );
    } catch(boost::python::error_already_set const &)
    {
        printf("Error in signalling python component initialization\r\n");
        PyErr_Print();
        assert(0 && "Halting on Python error");
    }
});

// Models

#define ADD_CS_d(arg) \
    command += Utility::toString(arg); \
    command += " ";

#define ADD_CS_s(arg) \
    assert( Utility::validateAlphaNumeric(arg, "._/<>-:") ); \
    command += "\""; \
    command += arg; \
    command += "\" ";

#define CUBESCRIPT_i(name, cmd) \
V8_FUNC_i(__script__##name, { \
    std::string command = #cmd; \
    command += " "; \
    ADD_CS_d(arg1); \
    execute(command.c_str()); \
});

#define CUBESCRIPT_ii(name, cmd) \
V8_FUNC_ii(__script__##name, { \
    std::string command = #cmd; \
    command += " "; \
    ADD_CS_d(arg1); \
    ADD_CS_d(arg2); \
    execute(command.c_str()); \
});

#define CUBESCRIPT_iii(name, cmd) \
V8_FUNC_iii(__script__##name, { \
    std::string command = #cmd; \
    command += " "; \
    ADD_CS_d(arg1); \
    ADD_CS_d(arg2); \
    ADD_CS_d(arg3); \
    execute(command.c_str()); \
});

#define CUBESCRIPT_iiiii(name, cmd) \
V8_FUNC_iiiii(__script__##name, { \
    std::string command = #cmd; \
    command += " "; \
    ADD_CS_d(arg1); \
    ADD_CS_d(arg2); \
    ADD_CS_d(arg3); \
    ADD_CS_d(arg4); \
    ADD_CS_d(arg5); \
    execute(command.c_str()); \
});

#define CUBESCRIPT_d(name, cmd) \
V8_FUNC_d(__script__##name, { \
    std::string command = #cmd; \
    command += " "; \
    ADD_CS_d(arg1); \
    execute(command.c_str()); \
});

#define CUBESCRIPT_dd(name, cmd) \
V8_FUNC_dd(__script__##name, { \
    std::string command = #cmd; \
    command += " "; \
    ADD_CS_d(arg1); \
    ADD_CS_d(arg2); \
    execute(command.c_str()); \
});

#define CUBESCRIPT_iid(name, cmd) \
V8_FUNC_iid(__script__##name, { \
    std::string command = #cmd; \
    command += " "; \
    ADD_CS_d(arg1); \
    ADD_CS_d(arg2); \
    ADD_CS_d(arg3); \
    execute(command.c_str()); \
});

#define CUBESCRIPT_ddd(name, cmd) \
V8_FUNC_ddd(__script__##name, { \
    std::string command = #cmd; \
    command += " "; \
    ADD_CS_d(arg1); \
    ADD_CS_d(arg2); \
    ADD_CS_d(arg3); \
    execute(command.c_str()); \
});

#define CUBESCRIPT_s(name, cmd) \
V8_FUNC_s(__script__##name, { \
    std::string command = #cmd; \
    command += " "; \
    ADD_CS_s(arg1); \
    execute(command.c_str()); \
});

#define CUBESCRIPT_sd(name, cmd) \
V8_FUNC_sd(__script__##name, { \
    std::string command = #cmd; \
    command += " "; \
    ADD_CS_s(arg1); \
    ADD_CS_d(arg2); \
    execute(command.c_str()); \
});

#define CUBESCRIPT_ss(name, cmd) \
V8_FUNC_ss(__script__##name, { \
    std::string command = #cmd; \
    command += " "; \
    ADD_CS_s(arg1); \
    ADD_CS_s(arg2); \
    execute(command.c_str()); \
});

#define CUBESCRIPT_ssdd(name, cmd) \
V8_FUNC_ssdd(__script__##name, { \
    std::string command = #cmd; \
    command += " "; \
    ADD_CS_s(arg1); \
    ADD_CS_s(arg2); \
    ADD_CS_d(arg3); \
    ADD_CS_d(arg4); \
    execute(command.c_str()); \
});

#define CUBESCRIPT_sssdd(name, cmd) \
V8_FUNC_sssdd(__script__##name, { \
    std::string command = #cmd; \
    command += " "; \
    ADD_CS_s(arg1); \
    ADD_CS_s(arg2); \
    ADD_CS_s(arg3); \
    ADD_CS_d(arg4); \
    ADD_CS_d(arg5); \
    execute(command.c_str()); \
});

#define CUBESCRIPT_sdddd(name, cmd) \
V8_FUNC_sdddd(__script__##name, { \
    std::string command = #cmd; \
    command += " "; \
    ADD_CS_s(arg1); \
    ADD_CS_d(arg2); \
    ADD_CS_d(arg3); \
    ADD_CS_d(arg4); \
    ADD_CS_d(arg5); \
    execute(command.c_str()); \
});

#define CUBESCRIPT_iiddddd(name, cmd) \
V8_FUNC_iiddddd(__script__##name, { \
    std::string command = #cmd; \
    command += " "; \
    ADD_CS_d(arg1); \
    ADD_CS_d(arg2); \
    ADD_CS_d(arg3); \
    ADD_CS_d(arg4); \
    ADD_CS_d(arg5); \
    ADD_CS_d(arg6); \
    ADD_CS_d(arg7); \
    execute(command.c_str()); \
});

CUBESCRIPT_i(modelShadow, mdlshadow);
CUBESCRIPT_i(modelCollide, mdlcollide);
CUBESCRIPT_i(modelPerEntityCollisionBoxes, mdlperentitycollisionboxes);
CUBESCRIPT_i(modelEllipseCollide, mdlellipsecollide);

CUBESCRIPT_s(objLoad, objload);

CUBESCRIPT_ss(objSkin, objskin);
CUBESCRIPT_ss(objBumpmap, objbumpmap);
CUBESCRIPT_ss(objEnvmap, objenvmap);
CUBESCRIPT_ss(objSpec, objspec);

CUBESCRIPT_d(mdlAlphatest, mdlalphatest);

CUBESCRIPT_ii(mdlBb, mdlbb);

CUBESCRIPT_i(mdlScale, mdlscale);
CUBESCRIPT_i(mdlSpec, mdlspec);
CUBESCRIPT_i(mdlGlow, mdlglow);
CUBESCRIPT_dd(mdlGlare, mdlglare);
CUBESCRIPT_i(mdlAmbient, mdlambient);

CUBESCRIPT_s(mdlShader, mdlshader);

CUBESCRIPT_i(mdlCollisionsOnlyForTriggering, mdlcollisionsonlyfortriggering);

CUBESCRIPT_ddd(mdlTrans, mdltrans);

CUBESCRIPT_s(md5Dir, md5dir);
CUBESCRIPT_ss(md5Load, md5load);

CUBESCRIPT_sssdd(md5Skin, md5skin);
CUBESCRIPT_ss(md5Bumpmap, md5bumpmap);
CUBESCRIPT_ss(md5Envmap, md5envmap);
CUBESCRIPT_sd(md5Alphatest, md5alphatest);

CUBESCRIPT_d(modelYaw, mdlyaw);
CUBESCRIPT_d(modelPitch, mdlpitch);

CUBESCRIPT_ss(md5Tag, md5tag);
CUBESCRIPT_ssdd(md5Anim, md5anim);

CUBESCRIPT_s(md5Animpart, md5animpart);
CUBESCRIPT_sdddd(md5Pitch, md5pitch);

CUBESCRIPT_ddd(rdVert, rdvert);
CUBESCRIPT_iii(rdTri, rdtri);
CUBESCRIPT_iiiii(rdJoint, rdjoint);
CUBESCRIPT_iid(rdLimitDist, rdlimitdist);
CUBESCRIPT_iiddddd(rdLimitRot, rdlimitrot);

CUBESCRIPT_dd(mdlEnvmap, mdlenvmap);

// Physics

V8_FUNC_s(__script__physicsCreateEngine, {
    PhysicsManager::createEngine(arg1);
});

V8_FUNC_dd(__script__physicsAddSphere, {
    physicsHandle ret = PhysicsManager::getEngine()->addSphere(arg1, arg2);
    V8_RETURN_INT(ret);
});

V8_FUNC_dddd(__script__physicsAddBox, {
    physicsHandle ret = PhysicsManager::getEngine()->addBox(arg1, arg2, arg3, arg4);
    V8_RETURN_INT(ret);
});

V8_FUNC_ddd(__script__physicsAddCapsule, {
    physicsHandle ret = PhysicsManager::getEngine()->addCapsule(arg1, arg2, arg3);
    V8_RETURN_INT(ret);
});

V8_FUNC_i(__script__physicsRemoveBody, {
    PhysicsManager::getEngine()->removeBody(arg1);
});

V8_FUNC_iddd(__script__physicsSetBodyPosition, {
    PhysicsManager::getEngine()->setBodyPosition(arg1, vec(arg2, arg3, arg4));
});

V8_FUNC_iddd(__script__physicsSetBodyVelocity, {
    PhysicsManager::getEngine()->setBodyVelocity(arg1, vec(arg2, arg3, arg4));
});

V8_FUNC_i(__script__physicsGetBodyPosition, {
    vec position;
    PhysicsManager::getEngine()->getBodyPosition(arg1, position);

    float ret[3];
    ret[0] = position.x;
    ret[1] = position.y;
    ret[2] = position.z;

    V8_RETURN_FARRAY(ret, 3);
});

V8_FUNC_i(__script__physicsGetBodyRotation, {
    quat rotation;
    PhysicsManager::getEngine()->getBodyRotation(arg1, rotation);

    float ret[4];
    ret[0] = rotation.x;
    ret[1] = rotation.y;
    ret[2] = rotation.z;
    ret[3] = rotation.w;
    V8_RETURN_FARRAY(ret, 4);
});

V8_FUNC_i(__script__physicsGetBodyVelocity, {
    vec velocity;
    PhysicsManager::getEngine()->getBodyVelocity(arg1, velocity);

    float ret[3];
    ret[0] = velocity.x;
    ret[1] = velocity.y;
    ret[2] = velocity.z;
    V8_RETURN_FARRAY(ret, 3);
});

V8_FUNC_iddd(__script__physicsSetLinearFactor, {
    vec factor(arg2, arg3, arg4);
    PhysicsManager::getEngine()->setLinearFactor(arg1, factor);
});

V8_FUNC_iddd(__script__physicsSetAngularFactor, {
    vec factor(arg2, arg3, arg4);
    PhysicsManager::getEngine()->setAngularFactor(arg1, factor);
});

V8_FUNC_iidddddd(__script__physicsAddConstraintP2P, {
    vec pivotA(arg3, arg4, arg5);
    vec pivotB(arg6, arg7, arg8);
    int ret = PhysicsManager::getEngine()->addConstraintP2P(arg1, arg2, pivotA, pivotB);
    V8_RETURN_INT(ret);
});

V8_FUNC_i(__script__physicsRemoveConstraint, {
    PhysicsManager::getEngine()->removeConstraint(arg1);
});

