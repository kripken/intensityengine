
// Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
// This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

// Should reflect ents.h
CLIENTSTATE = {
    ALIVE: 0,
    DEAD: 1, // unused by us
    SPAWNING: 2, // unused by us
    LAGGED: 3,
    EDITING: 4,
    SPECTATOR: 5
};

// Should reflect ents.h
PHYSICALSTATE = {
    FLOAT: 0,
    FALL: 1, 
    SLIDE: 2, 
    SLOPE: 3, 
    FLOOR: 4, 
    STEP_UP: 5, 
    STEP_DOWN: 6, 
    BOUNCE: 7
};

//! Either a Player Character (PC) or a Non-Player Character (NPC). A figure that has attachments, can move, etc.
Character = AnimatableLogicEntity.extend({
    _class: "Character",

    //! Characters correspond to the 'fpsent' Sauer type
    _sauerType: "fpsent",

    //== State variables

    //! The nickname of this player. Shown on the 'scoreboard', messages, etc.
    _name: new StateString(),

    //! How fast the entity can change facing (yaw and pitch), in degrees/second
    facingSpeed: new StateInteger(),

    //== dynent variables, reflected from Sauer

    //! The maximum movement speed. 20 is slow, 60 is quite fast.
    movementSpeed: new WrappedCFloat({ cGetter: 'CAPI.getMaxSpeed', cSetter: 'CAPI.setMaxSpeed' }),

    //! The yaw (azimuth, facing direction) of the character, in (float) degrees
    yaw: new WrappedCFloat({ cGetter: 'CAPI.getYaw', cSetter: 'CAPI.setYaw', customSynch: true }),

    //! The pitch (looking up or down) of the character, in (float) degrees
    pitch: new WrappedCFloat({ cGetter: 'CAPI.getPitch', cSetter: 'CAPI.setPitch', customSynch: true }),

    //! The *intention* to move. -1 is the same as a player pressing 'back'; 0 is not pressing; +1 is pressing 'forward'.
    move: new WrappedCInteger({ cGetter: 'CAPI.getMove', cSetter: 'CAPI.setMove', customSynch: true }),

    //! The *intention* to strafe. -1 is the same as a player pressing 'strafe left'; 0 is not pressing; +1 is pressing 'strafe right'.
    strafe: new WrappedCInteger({ cGetter: 'CAPI.getStrafe', cSetter: 'CAPI.setStrafe', customSynch: true }),

//    //! The *intention* to yaw.
//    yawing: new WrappedCInteger({ cGetter: 'CAPI.getYawing', cSetter: 'CAPI.setYawing', customSynch: true }),
// TODO: Enable these. But they do change the protocol, so forces everyone and everything to upgrade
//    //! The *intention* to pitch.
//    pitching: new WrappedCInteger({ cGetter: 'CAPI.getPitching', cSetter: 'CAPI.setPitching', customSynch: true }),

    //! The position of the character. This location is in Sauerbraten by convention at the top of the head.
    position: new WrappedCVector3({ cGetter: 'CAPI.getDynentO', cSetter: 'CAPI.setDynentO', customSynch: true }),

    //! The velocity of the character.
    velocity: new WrappedCVector3({ cGetter: 'CAPI.getDynentVel', cSetter: 'CAPI.setDynentVel', customSynch: true }),

    //! The falling velocity of the character.
    falling: new WrappedCVector3({ cGetter: 'CAPI.getDynentFalling', cSetter: 'CAPI.setDynentFalling', customSynch: true }),

    //! The radius of the character's bounding box.
    radius: new WrappedCFloat({ cGetter: 'CAPI.getRadius', cSetter: 'CAPI.setRadius' }),

    //! The distance from 'position' to the character's eyes (typically small), i.e., height of the character above its eyes.
    aboveEye: new WrappedCFloat({ cGetter: 'CAPI.getAboveeye', cSetter: 'CAPI.setAboveeye' }),

    //! The distance from the character's eyes (position-above_eye) to the character's feet (typically large),
    //! i.e., the height of the eyes above the ground.
    eyeHeight: new WrappedCFloat({ cGetter: 'CAPI.getEyeHeight', cSetter: 'CAPI.setEyeHeight' }),

    //! Whether on the last movement cycle this character was blocked by something, i.e.,
    //! the physics system has it colliding with an obstacle. Note that the floor doesn't count as
    //! an obstacle.
    blocked: new WrappedCBoolean({ cGetter: 'CAPI.getBlocked', cSetter: 'CAPI.setBlocked' }),

    //! Whether this entity can move
    canMove: new WrappedCBoolean({ cSetter: 'CAPI.setCanMove', clientSet: true }),

    //! Position protocol data specific to the current map, see fpsent. TODO: Make unsigned
    mapDefinedPositionData: new WrappedCInteger({
        cGetter: 'CAPI.getMapDefinedPositionData',
        cSetter: 'CAPI.setMapDefinedPositionData',
        customSynch: true
    }),

    //! Client state: editing, spectator, lagged, etc
    clientState: new WrappedCInteger({ cGetter: 'CAPI.getClientState', cSetter: 'CAPI.setClientState', customSynch: true }),

    //! Physical state: Falling, sliding, etc.
    physicalState: new WrappedCInteger({ cGetter: 'CAPI.getPhysicalState', cSetter: 'CAPI.setPhysicalState', customSynch: true }),

    //! Whether inside water or not
    inWater: new WrappedCInteger({ cGetter: 'CAPI.getInWater', cSetter: 'CAPI.setInWater', customSynch: true }),

    //! See dynent
    timeInAir: new WrappedCInteger({ cGetter: 'CAPI.getTimeInAir', cSetter: 'CAPI.setTimeInAir', customSynch: true }),

    //== dynent functions

    //! When called, sets the intention to jump in the air, immediately.
    jump: function() {
        CAPI.setJumping(this, true);
    },

    //== Main stuff

    init: function(uniqueId, kwargs) {
        log(DEBUG, "Character.init");

        this._super(uniqueId, kwargs);

        this._name = '-?-'; // Set by the server later

        //! The client number in our server, a unique identifier of currently connected clients. Can change between
        //! sessions, unlike unique_ids. Client numbers can be in general much smaller than unique_ids, and in that way
        //! can lessen bandwidth, especially since they are sent around quite a lot.
        //!
        //! -1 as the client number means that this client is not yet logged in, e.g., if it is a just-created NPC.
        //! in that case, we will do a localconnect in LogicSystem::setupCharacter
        this.clientNumber = kwargs !== undefined ? (kwargs.clientNumber !== undefined ? kwargs.clientNumber : -1) : -1; // XXX Needed? See activate.

        // Some reasonable defaults
        this.modelName = "stromar";
        this.eyeHeight = 14;
        this.aboveEye = 1;
        this.movementSpeed = 50;
        this.facingSpeed = 120;
        this.position = [512, 512, 550];
        this.radius = 3.0;
        this.canMove = true;
    },

    activate: function(kwargs) {
        log(DEBUG, "Character.activate");

        // If we have been given a client number as our parameters, apply that
        this.clientNumber = kwargs !== undefined ? (kwargs.clientNumber !== undefined ? kwargs.clientNumber : -1) : -1;
        eval(assert(' this.clientNumber >= 0 ')); // Must use newNPC!

        CAPI.setupCharacter(this); // Creates or connects with the fpsent for this entity, does C++ registration, etc.

        this._super(kwargs); // Done after setupCharacter, which gives us a client number, which activate then uses

        this._flushQueuedStateVariableChanges();

        log(DEBUG, "Character.activate complete");
    },

    clientActivate: function(kwargs) {
        this._super(kwargs);

        // If we have been given a client number as our parameters, apply that
        this.clientNumber = kwargs !== undefined ? (kwargs.clientNumber !== undefined ? kwargs.clientNumber : -1) : undefined;

        CAPI.setupCharacter(this); // Creates or connects with the fpsent for this entity, does C++ registration, etc.

        //! See usage in renderDynamic - this helps us not create the rendering args multiple times in each frame
        this.renderingArgsTimestamp = -1;
    },

    deactivate: function() {
//        print "Character.deactivate"

        CAPI.dismantleCharacter(this);

        this._super();
    },

    clientDeactivate: function() {
//        print "Character.client_deactivate"

        CAPI.dismantleCharacter(this);

        this._super();
    },

    //! Think and act for a time period of seconds seconds. If there are actions, do them, otherwise do some default, as specified in
    //! 'default_action'. This can further be overridden to do some checking even if there are pending actions, to see if there is
    //! something more important to be done which would justify cancelling them, and so forth.
    //! @param seconds The length of time to simulate.
    act: function(seconds) {
        if (this.actionSystem.isEmpty()) {
            this.defaultAction(seconds);
        } else {
            this._super(seconds);
        }
    },

    //! Overridden to provide some default activity when no pending action. For example, this could do some thinking and decide on
    //! the next action to be queued
    //! @param seconds The length of time to simulate.
    defaultAction: function(seconds) {
    },

    //! All characters are rendering using renderDynamic in scripting, which allows for easy
    //! extension (e.g., to utilize mapDefinedPositionData)
    //! @param HUDPass: If we are rendering the HUD right now
    //! @param needHUD: If this model should be shown as a HUD model (it is us, and we are in first person).
    renderDynamic: function(HUDPass, needHUD) {
        if (!this.initialized) {
            return;
        }

        if (!HUDPass && needHUD) return;

        if (this.renderingArgsTimestamp !== currTimestamp) {
            // Same naming conventions as in rendermodel.cpp in sauer

            var state = this.clientState;

            if (state == CLIENTSTATE.SPECTATOR || state == CLIENTSTATE.SPAWNING) {
                return;
            }

            var mdlname = HUDPass && needHUD ? this.HUDModelName : this.modelName;
            var yaw = this.yaw + 90;
            var pitch = this.pitch;
            var o = this.position.copy();
            if (HUDPass && needHUD && this.HUDModelOffset) o.add(this.HUDModelOffset);
            var basetime = this.startTime;
            var physstate = this.physicalState;
            var inwater = this.inWater;
            var move = this.move;
            var strafe = this.strafe;
            var vel = this.velocity.copy();
            var falling = this.falling.copy();
            var timeinair = this.timeInAir;
            var anim = this.decideAnimation(state, physstate, move, strafe, vel, falling, inwater, timeinair);
            var flags = this.getRenderingFlags();

            this.renderingArgs = this.createRenderingArgs(mdlname, anim, o, yaw, pitch, flags, basetime);
            this.renderingArgsTimestamp = currTimestamp;
        }

        this.getRenderModelFunc().apply(this, this.renderingArgs);
    },

    getRenderModelFunc: function() {
        return CAPI.renderModel;
    },

    createRenderingArgs: function(mdlname, anim, o, yaw, pitch, flags, basetime) {
        return [this, mdlname, anim, o.x, o.y, o.z, yaw, pitch, flags, basetime];
    },

    getRenderingFlags: function() {
        var flags = MODEL.LIGHT | MODEL.DYNSHADOW;
        if (this !== getPlayerEntity()) {
            flags |= MODEL.CULL_VFC | MODEL.CULL_OCCLUDED | MODEL.CULL_QUERY;
        }
//        flags |= MODEL.FULLBRIGHT; // TODO: For non-characters, use: flags |= MODEL.CULL_DIST;
        return flags;
    },  

    //! Select the animation to show for this character. Can be overridden
    //! if desired, but typically you would only override decideActionAnimation.
    decideAnimation: function(state, physstate, move, strafe, vel, falling, inwater, timeinair) {
        // Same naming conventions as in rendermodel.cpp in sauer

        var anim = this.decideActionAnimation();

        if (state == CLIENTSTATE.EDITING || state == CLIENTSTATE.SPECTATOR) {
            anim = ANIM_EDIT | ANIM_LOOP;
        } else if (state == CLIENTSTATE.LAGGED) {
            anim = ANIM_LAG | ANIM_LOOP;
        } else {
            if(inwater && physstate<=PHYSICALSTATE.FALL) {
                anim |= (((move || strafe) || vel.z+falling.z>0 ? ANIM_SWIM : ANIM_SINK) | ANIM_LOOP) << ANIM_SECONDARY;
            } else if (timeinair>250) {
                anim |= (ANIM_JUMP|ANIM_END) << ANIM_SECONDARY;
            } else if (move || strafe) 
            {
                if (move>0) {
                    anim |= (ANIM_FORWARD | ANIM_LOOP) << ANIM_SECONDARY;
                } else if (strafe) {
                    anim |= ((strafe>0 ? ANIM_LEFT : ANIM_RIGHT)|ANIM_LOOP) << ANIM_SECONDARY;
                } else if (move<0) {
                    anim |= (ANIM_BACKWARD | ANIM_LOOP) << ANIM_SECONDARY;
                }
            }

            if ( (anim & ANIM_INDEX) == ANIM_IDLE && ((anim >> ANIM_SECONDARY) & ANIM_INDEX)) {
                anim >>= ANIM_SECONDARY;
            }
        }

        if(!((anim >> ANIM_SECONDARY) & ANIM_INDEX)) {
            anim |= (ANIM_IDLE | ANIM_LOOP) << ANIM_SECONDARY;
        }

        return anim;
    },

    //! Select the 'action' animation to show. This does not take into account
    //! effects like lag, water, etc., which are handled in decideAnimation.
    //! By default this function simply returns this.animation. It can be
    //! overridden to do something more complex, e.g., taking into account
    //! map-specific information in mapDefinedPositionData.
    decideActionAnimation: function() {
        return this.animation;
    },

    //! @return The 'center' of the character, something like the center of gravity,
    //!         typically people and AI would aim at this point, and not at 'position',
    //!         which is the feet position. Override this function if the 'center'
    //!         is defined in a nonstandard way (default is 0.75*eyeheight above feet).
    getCenter: function() {
        var ret = this.position.copy();
        ret.z += this.eyeHeight*0.75;
        return ret;
    },

    //! Given an origin position (usually from an attachment tag), fix it so it
    //! corresponds to where the player actually targeted from. This should take into
    //! account the cameraheight, if such data is available. See the effectiveCameraHeight plugin.
    getTargetingOrigin: function(origin) {
        return origin;
    },

    isOnFloor: function() {
        if (floorDistance(this.position, 1024) < 1) return true;
        if (this.velocity.z < -1 || this.falling.z < -1) return false;
        return World.isColliding(this.position, this.radius+2, this);
    },
});


//! Player Character (PC). The default class used for PCs, if not otherwise set by the application.
Player = Character.extend({
    _class: "Player",
    _canEdit: new StateBoolean(),
    HUDModelName: new StateString(),

    init: function(uniqueId, kwargs) {
        log(DEBUG, "Player.init");
        this._super(uniqueId, kwargs);

        this._canEdit = false; // Set to true by the server if it should be true
        this.HUDModelName = '';
    },

    clientActivate: function(kwargs) {
        this._super(kwargs);
    },
});


//
// Register classes
//

registerEntityClass(Character, "dynent");
registerEntityClass(Player, "dynent");


// Some useful plugins

Character.plugins = {
    //! When a character shoots, the target position defined by the mouse is used. But when other
    //! clients calculate where they shoot at, they use the yaw/pitch values. If cameraheight > 0,
    //! then the two target positions may differ. This plugin calculates the effective camera height
    //! so other clients can use it
    effectiveCameraHeight: {
        effectiveCameraHeight: new StateFloat({ reliable: false }),

        clientActivate: function() {
            this.effectiveCameraHeightTimer = new RepeatingTimer(1.0);
            this.effectiveCameraHeightTimer.prime();
            this.effectiveCameraHeightSent = false;
        },

        clientAct: function(seconds) {
            if (this === getPlayerEntity() && this.effectiveCameraHeightTimer.tick(seconds) && !isPlayerEditing(this)) {
                if (Math.abs(Math.cos(this.pitch*Math.PI/180)) < 0.05) return;

                var target = CAPI.getTargetPosition();
                var xyDist = 0, tempDist;

                tempDist = this.position.x -target.x;
                xyDist += tempDist*tempDist;
                tempDist = this.position.y -target.y;
                xyDist += tempDist*tempDist;
                xyDist = Math.sqrt(xyDist);

                var upperHeight = Math.tan(-this.pitch*Math.PI/180) * xyDist;
                var effectiveCameraHeight = upperHeight + target.z - this.position.z;

                if (!this.effectiveCameraHeightSent || Math.abs(effectiveCameraHeight - this.effectiveCameraHeight) >= 1) {
                    this.effectiveCameraHeight = effectiveCameraHeight;
                    this.effectiveCameraHeightSent = true;
                }
            }
        },

        getTargetingOrigin: function(fallback) {
            if (!this.effectiveCameraHeight) {
                return fallback; // No idea about anything better
            } else {
                return this.position.addNew(new Vector3(0, 0, this.effectiveCameraHeight));
            }
        },
    },

    jumpWhilePressingSpace: {
        performJump: function(down) {
            var player = getPlayerEntity();
            var water = World.getMaterial(player.position) === MATERIAL.WATER;
            getPlayerEntity().isPressingJumpSeconds = (down && (player.isOnFloor() || water)) ? (!water ? 0.25 : 0.175) : -1;
        },

        plugin: {
            clientActivate: function() {
                this.isPressingJumpSeconds = -1;
            },

            clientAct: function(seconds) {
                if (this.isPressingJumpSeconds > 0 && Health.isActiveEntity(this)) {
                    this.velocity.z += (1500+World.gravity)*seconds*this.isPressingJumpSeconds/0.25;
                    this.isPressingJumpSeconds -= seconds;
                }
            },
        },
    },

    footsteps: {
        footsteps: {
            floorHeight: 0.5,
            readyHeight: 2.5,
            delay: 1/2,
            tags: ['tag_foot_l', 'tag_foot_r'],
            delays: [-1, -1],
            sounds: ['swarm/footstep1.ogg', 'swarm/footstep2.ogg'],
        },

        activate: function() {
            this.attachments.push('*tag_foot_l');
            this.attachments.push('*tag_foot_r');
        },

        clientAct: function(seconds) {
            if (!this.move && !this.strafe) return;
            if (isPlayerEditing(this)) return;

            var baseHeight = this.position.z;
            for (var i = 0; i < this.footsteps.tags.length; i++) {
                var footHeight = CAPI.getAttachmentPosition(this, this.footsteps.tags[i]).z - baseHeight;
                if (this.footsteps.delays[i] === 0) {
                    if (footHeight <= this.footsteps.floorHeight) {
                        Sound.play(this.footsteps.sounds[i], this.position.copy());
                        this.footsteps.delays[i] = -1;
                    }
                } else if (this.footsteps.delays[i] === -1) {
                    if (footHeight >= this.footsteps.readyHeight) {
                        this.footsteps.delays[i] = this.footsteps.delay;
                    }
                } else {
                    this.footsteps.delays[i] = Math.max(this.footsteps.delays[i] - seconds, 0);
                }
            }
        }
    },
};

