
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

#define EMBED_CAPI_FUNC(scriptName, cName, num_params) \
    module->setProperty(scriptName, engine->createFunction((NativeFunction)cName, num_params));

#define EMBED_CAPI_FUNC_STD(name, num) EMBED_CAPI_FUNC(#name, __script__##name, num);

// Entity attribs

EMBED_CAPI_FUNC("currTime", __script__currTime, 0);

// Entity attribs

EMBED_CAPI_FUNC("setAnimation", __script__setAnimation, 2);
EMBED_CAPI_FUNC("getStartTime", __script__getStartTime, 1);
EMBED_CAPI_FUNC("setModelName", __script__setModelName, 2);
EMBED_CAPI_FUNC("setAttachments_raw", __script__setAttachments_raw, 2);
EMBED_CAPI_FUNC("getAttachmentPosition", __script__getAttachmentPosition, 2);
EMBED_CAPI_FUNC("setCanMove", __script__setCanMove, 2);

// Entity management

//EMBED_CAPI_FUNC("registerLogicEntityNonSauer", __script__registerLogicEntityNonSauer, 1); DEPRECATED
EMBED_CAPI_FUNC("unregisterLogicEntity", __script__unregisterLogicEntity, 1);

EMBED_CAPI_FUNC("placeInWorld", __script__placeInWorld, 2);

EMBED_CAPI_FUNC("setupExtent", __script__setupExtent, 9);
EMBED_CAPI_FUNC("setupCharacter", __script__setupCharacter, 1);
EMBED_CAPI_FUNC("setupNonSauer", __script__setupNonSauer, 1);

EMBED_CAPI_FUNC("dismantleExtent", __script__dismantleExtent, 1);
EMBED_CAPI_FUNC("dismantleCharacter", __script__dismantleCharacter, 1);

// Sounds

#ifdef CLIENT
    EMBED_CAPI_FUNC("playSoundByName", __script_playSoundByName, 5);
#endif

EMBED_CAPI_FUNC("music", __script__music, 1);

EMBED_CAPI_FUNC("preloadSound", __script__preloadSound, 2);
EMBED_CAPI_FUNC("playSound", __script__playSound, 1);

// Extents

EMBED_CAPI_FUNC("getAttr1", __script__getAttr1, 1); EMBED_CAPI_FUNC("setAttr1", __script__setAttr1, 2);
                                                    EMBED_CAPI_FUNC("FAST_setAttr1", __script__FAST_setAttr1, 2);
EMBED_CAPI_FUNC("getAttr2", __script__getAttr2, 1); EMBED_CAPI_FUNC("setAttr2", __script__setAttr2, 2);
                                                    EMBED_CAPI_FUNC("FAST_setAttr2", __script__FAST_setAttr2, 2);
EMBED_CAPI_FUNC("getAttr3", __script__getAttr3, 1); EMBED_CAPI_FUNC("setAttr3", __script__setAttr3, 2);
                                                    EMBED_CAPI_FUNC("FAST_setAttr3", __script__FAST_setAttr3, 2);
EMBED_CAPI_FUNC("getAttr4", __script__getAttr4, 1); EMBED_CAPI_FUNC("setAttr4", __script__setAttr4, 2);
                                                    EMBED_CAPI_FUNC("FAST_setAttr4", __script__FAST_setAttr4, 2);

EMBED_CAPI_FUNC("getCollisionRadiusWidth", __script__getCollisionRadiusWidth, 1);
EMBED_CAPI_FUNC("setCollisionRadiusWidth", __script__setCollisionRadiusWidth, 2);

EMBED_CAPI_FUNC("getCollisionRadiusHeight", __script__getCollisionRadiusHeight, 1);
EMBED_CAPI_FUNC("setCollisionRadiusHeight", __script__setCollisionRadiusHeight, 2);

EMBED_CAPI_FUNC("getExtentO_raw", __script__getExtentO_raw, 2); EMBED_CAPI_FUNC("setExtentO_raw", __script__setExtentO_raw, 4);

// Dynents

EMBED_CAPI_FUNC("getMaxSpeed", __script__getMaxSpeed, 1); EMBED_CAPI_FUNC("setMaxSpeed", __script__setMaxSpeed, 2);
EMBED_CAPI_FUNC("getRadius", __script__getRadius, 1); EMBED_CAPI_FUNC("setRadius", __script__setRadius, 2);
EMBED_CAPI_FUNC("getEyeHeight", __script__getEyeHeight, 1); EMBED_CAPI_FUNC("setEyeHeight", __script__setEyeHeight, 2);
EMBED_CAPI_FUNC("getAboveeye", __script__getAboveeye, 1); EMBED_CAPI_FUNC("setAboveeye", __script__setAboveeye, 2);
EMBED_CAPI_FUNC("getYaw", __script__getYaw, 1); EMBED_CAPI_FUNC("setYaw", __script__setYaw, 2);
EMBED_CAPI_FUNC("getPitch", __script__getPitch, 1); EMBED_CAPI_FUNC("setPitch", __script__setPitch, 2);
EMBED_CAPI_FUNC("getMove", __script__getMove, 1); EMBED_CAPI_FUNC("setMove", __script__setMove, 2);
EMBED_CAPI_FUNC("getStrafe", __script__getStrafe, 1); EMBED_CAPI_FUNC("setStrafe", __script__setStrafe, 2);
EMBED_CAPI_FUNC("getYawing", __script__getYawing, 1); EMBED_CAPI_FUNC("setYawing", __script__setYawing, 2);
EMBED_CAPI_FUNC("getPitching", __script__getPitching, 1); EMBED_CAPI_FUNC("setPitching", __script__setPitching, 2);
EMBED_CAPI_FUNC("getJumping", __script__getJumping, 1); EMBED_CAPI_FUNC("setJumping", __script__setJumping, 2);
EMBED_CAPI_FUNC("getBlocked", __script__getBlocked, 1); EMBED_CAPI_FUNC("setBlocked", __script__setBlocked, 2);
EMBED_CAPI_FUNC("getMapDefinedPositionData", __script__getMapDefinedPositionData, 1); EMBED_CAPI_FUNC("setMapDefinedPositionData", __script__setMapDefinedPositionData, 2);
EMBED_CAPI_FUNC("getClientState", __script__getClientState, 1); EMBED_CAPI_FUNC("setClientState", __script__setClientState, 2);
EMBED_CAPI_FUNC("getPhysicalState", __script__getPhysicalState, 1); EMBED_CAPI_FUNC("setPhysicalState", __script__setPhysicalState, 2);
EMBED_CAPI_FUNC("getInWater", __script__getInWater, 1); EMBED_CAPI_FUNC("setInWater", __script__setInWater, 2);
EMBED_CAPI_FUNC("getTimeInAir", __script__getTimeInAir, 1); EMBED_CAPI_FUNC("setTimeInAir", __script__setTimeInAir, 2);


EMBED_CAPI_FUNC("getDynentO_raw", __script__getDynentO_raw, 2); EMBED_CAPI_FUNC("setDynentO_raw", __script__setDynentO_raw, 4);
EMBED_CAPI_FUNC("getDynentVel_raw", __script__getDynentVel_raw, 2); EMBED_CAPI_FUNC("setDynentVel_raw", __script__setDynentVel_raw, 4);
EMBED_CAPI_FUNC("getDynentFalling_raw", __script__getDynentFalling_raw, 2); EMBED_CAPI_FUNC("setDynentFalling_raw", __script__setDynentFalling_raw, 4);

// Geometry utilities

EMBED_CAPI_FUNC("rayLos", __script__rayLos, 6);
EMBED_CAPI_FUNC("rayPos", __script__rayPos, 7);
EMBED_CAPI_FUNC("rayFloor", __script__rayFloor, 4);

// Effects

#ifdef CLIENT
    EMBED_CAPI_FUNC("addDecal", __script__addDecal, 12);
    EMBED_CAPI_FUNC("particleSplash", __script__particleSplash, 6);
    EMBED_CAPI_FUNC("particleFireball", __script__particleFireball, 8);
    EMBED_CAPI_FUNC("particleFlare", __script__particleFlare, 10);
    EMBED_CAPI_FUNC("particleTrail", __script__particleTrail, 11);
    EMBED_CAPI_FUNC("particleFlame", __script__particleFlame, 12);
    EMBED_CAPI_FUNC("addDynlight", __script__addDynlight, 14);
    EMBED_CAPI_FUNC("spawnDebris", __script__spawnDebris, 9);
    EMBED_CAPI_FUNC("particleMeter", __script__particleMeter, 6);
    EMBED_CAPI_FUNC("particleText", __script__particleText, 9);
    EMBED_CAPI_FUNC("clientDamageEffect", __script__clientDamageEffect, 2);
    EMBED_CAPI_FUNC("showHUDRect", __script__showHUDRect, 5);
    EMBED_CAPI_FUNC("showHUDImage", __script__showHUDImage, 5);
    EMBED_CAPI_FUNC("showHUDText", __script__showHUDText, 5);

#endif

// Messages (TODO: Separate client/server ones)

EMBED_CAPI_FUNC("PersonalServerMessage", __script__PersonalServerMessage, 4);
EMBED_CAPI_FUNC("ParticleSplashToClients", __script__ParticleSplashToClients, 7);
EMBED_CAPI_FUNC("SoundToClientsByName", __script__SoundToClientsByName, 6);
EMBED_CAPI_FUNC("DoClick", __script__DoClick, 6);
EMBED_CAPI_FUNC("StateDataChangeRequest", __script__StateDataChangeRequest, 3);
EMBED_CAPI_FUNC("UnreliableStateDataChangeRequest", __script__UnreliableStateDataChangeRequest, 3);
EMBED_CAPI_FUNC("NotifyNumEntities", __script__NotifyNumEntities, 2);
EMBED_CAPI_FUNC("LogicEntityCompleteNotification", __script__LogicEntityCompleteNotification, 5);
EMBED_CAPI_FUNC("LogicEntityRemoval", __script__LogicEntityRemoval, 2);
EMBED_CAPI_FUNC("StateDataUpdate", __script__StateDataUpdate, 5);
EMBED_CAPI_FUNC("UnreliableStateDataUpdate", __script__UnreliableStateDataUpdate, 5);
EMBED_CAPI_FUNC("ExtentCompleteNotification", __script__ExtentCompleteNotification, 11);

// File access

EMBED_CAPI_FUNC("readFile", __script__readFile, 1);

// Mapping

EMBED_CAPI_FUNC("textureReset", __script__textureReset, 0);
EMBED_CAPI_FUNC("texture", __script__texture, 6);

EMBED_CAPI_FUNC("mapmodelReset", __script__mapmodelReset, 0);
EMBED_CAPI_FUNC("mapmodel", __script__mapmodel, 1);

EMBED_CAPI_FUNC("autograss", __script__autograss, 1);

EMBED_CAPI_FUNC("texLayer", __script__texLayer, 1);

EMBED_CAPI_FUNC("setShader", __script__setShader, 1);
EMBED_CAPI_FUNC("setShaderParam", __script__setShaderParam, 4);

EMBED_CAPI_FUNC("materialReset", __script__materialReset, 0);

EMBED_CAPI_FUNC("loadSky", __script__loadSky, 1);

EMBED_CAPI_FUNC("fogColor", __script__fogColor, 1);
EMBED_CAPI_FUNC("fog", __script__fog, 1);
EMBED_CAPI_FUNC_STD(waterFog, 1);
EMBED_CAPI_FUNC_STD(waterColor, 3);
EMBED_CAPI_FUNC_STD(spinSky, 1);
EMBED_CAPI_FUNC_STD(cloudLayer, 1);
EMBED_CAPI_FUNC_STD(cloudScrollX, 1);
EMBED_CAPI_FUNC_STD(cloudScrollY, 1);
EMBED_CAPI_FUNC_STD(cloudScale, 1);
EMBED_CAPI_FUNC_STD(skyTexture, 1);
EMBED_CAPI_FUNC_STD(texScroll, 2);
EMBED_CAPI_FUNC("shadowmapAmbient", __script__shadowmapAmbient, 1);
EMBED_CAPI_FUNC("shadowmapAngle", __script__shadowmapAngle, 1);
EMBED_CAPI_FUNC("skylight", __script__skylight, 3);
EMBED_CAPI_FUNC("blurSkylight", __script__blurSkylight, 1);
EMBED_CAPI_FUNC("ambient", __script__ambient, 1);

EMBED_CAPI_FUNC("preloadModel", __script__preloadModel, 1);
EMBED_CAPI_FUNC("reloadModel", __script__reloadModel, 1);

#ifdef USE_JPEG2000
EMBED_CAPI_FUNC("convertJP2toPNG", __script__convertJP2toPNG, 2);
#endif
EMBED_CAPI_FUNC("convertPNGtoDDS", __script__convertPNGtoDDS, 2);
EMBED_CAPI_FUNC("combineImages", __script__combineImages, 3);

// HUD

#ifdef CLIENT
    EMBED_CAPI_FUNC("getTargetPosition", __script__getTargetPosition, 0);
    EMBED_CAPI_FUNC("getTargetEntity", __script__getTargetEntity, 0);

#endif

// World

EMBED_CAPI_FUNC("isColliding", __script__isColliding, 3);
EMBED_CAPI_FUNC("setGravity", __script__setGravity, 1);
EMBED_CAPI_FUNC("getMaterial", __script__getMaterial, 3);

// NPCs/bots

#ifdef SERVER
    EMBED_CAPI_FUNC("addNPC", __script__addNPC, 1);
    EMBED_CAPI_FUNC("removeNPC", __script__removeNPC, 1);
#endif

// Rendering

#ifdef CLIENT
    EMBED_CAPI_FUNC("renderModel2", __script__renderModel2, 11);
    EMBED_CAPI_FUNC("renderModel3", __script__renderModel3, 15);
#endif

// GUI

#ifdef CLIENT
    EMBED_CAPI_FUNC("showMessage", __script__showMessage__, 1);
    EMBED_CAPI_FUNC("showInputDialog", __script__showInputDialog__, 1);
    EMBED_CAPI_FUNC_STD(setDefaultThirdpersonMode, 1);
#endif

// Network

#ifdef CLIENT
    EMBED_CAPI_FUNC("connect", __script__connect__, 2);
#endif

// Camera

#ifdef CLIENT
    EMBED_CAPI_FUNC("forceCamera", __script__forceCamera__, 7);
    EMBED_CAPI_FUNC("getCamera", __script__getCamera__, 0);
#endif

// Code

EMBED_CAPI_FUNC("compile", __script__compile__, 2);

// Components

EMBED_CAPI_FUNC("signalComponent", __script__signalComponent__, 2);

// Models

EMBED_CAPI_FUNC_STD(modelShadow, 1);
EMBED_CAPI_FUNC_STD(modelCollide, 1);
EMBED_CAPI_FUNC_STD(modelPerEntityCollisionBoxes, 1);
EMBED_CAPI_FUNC_STD(modelEllipseCollide, 1);

EMBED_CAPI_FUNC_STD(objLoad, 1);
EMBED_CAPI_FUNC_STD(objSkin, 2);
EMBED_CAPI_FUNC_STD(objBumpmap, 2);
EMBED_CAPI_FUNC_STD(objEnvmap, 2);
EMBED_CAPI_FUNC_STD(objSpec, 2);
EMBED_CAPI_FUNC_STD(mdlAlphatest, 1);
EMBED_CAPI_FUNC_STD(mdlBb, 1);
EMBED_CAPI_FUNC_STD(mdlScale, 1);
EMBED_CAPI_FUNC_STD(mdlSpec, 1);
EMBED_CAPI_FUNC_STD(mdlGlow, 1);
EMBED_CAPI_FUNC_STD(mdlGlare, 2);
EMBED_CAPI_FUNC_STD(mdlAmbient, 1);
EMBED_CAPI_FUNC_STD(mdlShader, 1);

EMBED_CAPI_FUNC_STD(mdlCollisionsOnlyForTriggering, 1);

EMBED_CAPI_FUNC_STD(mdlTrans, 3);

EMBED_CAPI_FUNC_STD(md5Dir, 1);
EMBED_CAPI_FUNC_STD(md5Load, 2);

EMBED_CAPI_FUNC_STD(md5Skin, 5);
EMBED_CAPI_FUNC_STD(md5Bumpmap, 2);
EMBED_CAPI_FUNC_STD(md5Envmap, 2);
EMBED_CAPI_FUNC_STD(md5Alphatest, 2);

EMBED_CAPI_FUNC_STD(modelYaw, 1);
EMBED_CAPI_FUNC_STD(modelPitch, 1);

EMBED_CAPI_FUNC_STD(md5Tag, 2);
EMBED_CAPI_FUNC_STD(md5Anim, 4);

EMBED_CAPI_FUNC_STD(md5Animpart, 1);
EMBED_CAPI_FUNC_STD(md5Pitch, 5);

EMBED_CAPI_FUNC_STD(rdVert, 3);
EMBED_CAPI_FUNC_STD(rdTri, 3);
EMBED_CAPI_FUNC_STD(rdJoint, 5);
EMBED_CAPI_FUNC_STD(rdLimitDist, 3);
EMBED_CAPI_FUNC_STD(rdLimitRot, 7);

EMBED_CAPI_FUNC_STD(mdlEnvmap, 2);

// Physics

EMBED_CAPI_FUNC_STD(physicsAddSphere, 2);
EMBED_CAPI_FUNC_STD(physicsAddBox, 4);
EMBED_CAPI_FUNC_STD(physicsRemoveBody, 1);
EMBED_CAPI_FUNC_STD(physicsSetBodyPosition, 4);
EMBED_CAPI_FUNC_STD(physicsSetBodyVelocity, 4);
EMBED_CAPI_FUNC_STD(physicsGetBody, 1);

