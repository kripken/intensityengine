
// Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
// This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

//! Static Entity - all entities based on Sauer's extent type, which are entities that are static in
//! the world. In general you should not create entities of this type, use the child types instead.
StaticEntity = AnimatableLogicEntity.extend({
    _class: "StaticEntity",

    //! By default static entities do not have actions. Change this if you need
    //! a particular subclass to have act/clientAct called each frame.
    shouldAct: false,

    //! StaticEntities correspond to the extent Sauer type
    _sauerType: "extent",
    //! The ent_type integer value in Sauer. Overridden in child classes
    _sauerTypeIndex: 0,

    //! The position of the entity in the world
    position: new WrappedCVector3({ cGetter: 'CAPI.getExtentO', cSetter: 'CAPI.setExtentO' }),

    radius: new StateFloat(), // TODO: Use sauer values for bounding box // XXX - needed?

    //! attr1 of Sauerbraten's extentity. Should only be used by the immediate subclasses of StaticEntity, who will then wrap
    //! higher-level concepts for end-user programmers to have. Normal developers should never see this.
    attr1: new WrappedCInteger({ cGetter: 'CAPI.getAttr1', cSetter: 'CAPI.setAttr1' }),

    //! attr2 of Sauerbraten's extentity. Should only be used by the immediate subclasses of StaticEntity, who will then wrap
    //! higher-level concepts for end-user programmers to have. Normal developers should never see this.
    attr2: new WrappedCInteger({ cGetter: 'CAPI.getAttr2', cSetter: 'CAPI.setAttr2' }),

    //! attr3 of Sauerbraten's extentity. Should only be used by the immediate subclasses of StaticEntity, who will then wrap
    //! higher-level concepts for end-user programmers to have. Normal developers should never see this.
    attr3: new WrappedCInteger({ cGetter: 'CAPI.getAttr3', cSetter: 'CAPI.setAttr3' }),

    //! attr4 of Sauerbraten's extentity. Should only be used by the immediate subclasses of StaticEntity, who will then wrap
    //! higher-level concepts for end-user programmers to have. Normal developers should never see this.
    attr4: new WrappedCInteger({ cGetter: 'CAPI.getAttr4', cSetter: 'CAPI.setAttr4' }),

    init: function(uniqueId, kwargs) {
        log(DEBUG, "StaticEntity.init");

        kwargs = defaultValue(kwargs, {});
        kwargs._persistent = true; // Static entities are by default persistent

        this._super(uniqueId, kwargs);

        if (kwargs === undefined || kwargs.position === undefined) {
            this.position = [511, 512, 513];
        } else {
            this.position = [ parseFloat(kwargs.position.x), parseFloat(kwargs.position.y), parseFloat(kwargs.position.z) ];
        }

        this.radius = 0;

        log(DEBUG, "StaticEntity.init complete");
    },

    activate: function(kwargs) {
//        print "***Static Entity activate", this.unique_id, params

        kwargs = defaultValue(kwargs, {});

        log(DEBUG, this.uniqueId + " SE: _super() " + serializeJSON(kwargs));

        this._super(kwargs);

        if (kwargs._type === undefined) {
            kwargs._type = this._sauerTypeIndex;
        } // TODO: Assert that these two are otherwise equal?

        log(DEBUG, "StaticEntity defaults:");

        //! If not given to us, get from the attributes, which will read from stateData, as
        //! cGetter won't be called yet
        var setDef = function(_name, _source, def) {
            eval(format(
                "try {{ "+
                "    kwargs.{0} = this.{1}; "+
                "}} catch (e) {{ /* No stateData */ "+
                "    kwargs.{0} = def; "+
                "}}",
                _name, _source)
            );
        }
        setDef("x", "position.x", 512);
        setDef("y", "position.y", 512);
        setDef("z", "position.z", 512);
        setDef("attr1", "attr1", 0);
        setDef("attr2", "attr2", 0);
        setDef("attr3", "attr3", 0);
        setDef("attr4", "attr4", 0);

        log(DEBUG, "SE: setupExtent:");

//        log(DEBUG, "Keys:" + keys(CAPI));
        // Done first so we set up the values ok
        CAPI.setupExtent(this, kwargs._type, kwargs.x, kwargs.y, kwargs.z, kwargs.attr1, kwargs.attr2, kwargs.attr3, kwargs.attr4);

        log(DEBUG, "SE: Flush:");

        this._flushQueuedStateVariableChanges();

        // Ensure the state data contains copies of C++ stuff (otherwise, might be empty, and we need it for initializing on the server)
        // XXX needed?
        log(DEBUG, "Ensuring SD values - deprecate");
        log(DEBUG, "position:" + this.position.x + "," + this.position.y + "," + this.position.z);
        log(DEBUG, "position class:" + this.position._class);
        this.position = this.position
        log(DEBUG, "position(2):" + this.position.x + "," + this.position.y + "," + this.position.z);
        log(DEBUG, "Ensuring SD values (2)");
        this.attr1 = this.attr1
        this.attr2 = this.attr2
        this.attr3 = this.attr3
        this.attr4 = this.attr4
        log(DEBUG, "Ensuring SD values - complete");
    },

    deactivate: function() {
//        print "*** Static Entity deactivating"

        CAPI.dismantleExtent(this);

        this._super();
    },

    clientActivate: function(kwargs) {
 //       print "Client Activate, params:", params

        if (kwargs._type === undefined) { // make up some stuff, just until we get the complete state data
            kwargs._type = this._sauerTypeIndex;
            kwargs.x = 512;
            kwargs.y = 512;
            kwargs.z = 512;
            kwargs.attr1 = 0;
            kwargs.attr2 = 0;
            kwargs.attr3 = 0;
            kwargs.attr4 = 0;
        }

        // Done first so we set up the values ok
        CAPI.setupExtent(this, kwargs._type, kwargs.x, kwargs.y, kwargs.z, kwargs.attr1, kwargs.attr2, kwargs.attr3, kwargs.attr4);

//        print "*** StaticEntity.client_activate"
        this._super(kwargs);
    },

    clientDeactivate: function() {
//        print "*** Static Entity client deactivating"

        CAPI.dismantleExtent(this);

        this._super();
    },

    sendCompleteNotification: function(clientNumber) {
        log(DEBUG, "StaticE.sendCompleteNotification:"); // + serializeJSON(this.stateData));

        MessageSystem.send( clientNumber,
                            CAPI.ExtentCompleteNotification,
                            this.uniqueId,
                            this._class,
                            serializeJSON(this.createStateDataDict()),
                            this.position.x, this.position.y, this.position.z,
                            this.attr1, this.attr2, this.attr3, this.attr4     );

        log(DEBUG, "StaticE.sendCompleteNotification complete");
    }
});

//! A fixed light source in the world, used for ray-traced (baked) shadows and lighting. Changes to this entity will only
//! become visible if we call calclight() for baking, as this is a static light, not a dynamic one.
Light = StaticEntity.extend({
    _class: "Light",

    _sauerTypeIndex: 1,

    //! attr1 of Sauerbraten's extentity. Should only be used by the immediate subclasses of StaticEntity, who will then wrap
    //! higher-level concepts for end-user programmers to have. Normal developers should never see this.
    attr1: new WrappedCInteger({ cGetter: 'CAPI.getAttr1', cSetter: 'CAPI.setAttr1', guiName: "radius", altName: "radius" }),

    //! attr2 of Sauerbraten's extentity. Should only be used by the immediate subclasses of StaticEntity, who will then wrap
    //! higher-level concepts for end-user programmers to have. Normal developers should never see this.
    attr2: new WrappedCInteger({ cGetter: 'CAPI.getAttr2', cSetter: 'CAPI.setAttr2', guiName: "red", altName: "red" }),

    //! attr3 of Sauerbraten's extentity. Should only be used by the immediate subclasses of StaticEntity, who will then wrap
    //! higher-level concepts for end-user programmers to have. Normal developers should never see this.
    attr3: new WrappedCInteger({ cGetter: 'CAPI.getAttr3', cSetter: 'CAPI.setAttr3', guiName: "green", altName: "green" }),

    //! attr4 of Sauerbraten's extentity. Should only be used by the immediate subclasses of StaticEntity, who will then wrap
    //! higher-level concepts for end-user programmers to have. Normal developers should never see this.
    attr4: new WrappedCInteger({ cGetter: 'CAPI.getAttr4', cSetter: 'CAPI.setAttr4', guiName: "blue", altName: "blue" }),

    //! The radius of the light, i.e., how far to send light
    radius: new VariableAlias("attr1"),

    //! The red component of this light, 0-255
    red: new VariableAlias("attr2"),

    //! The green component of this light, 0-255
    green: new VariableAlias("attr3"),

    //! The blue component of this light, 0-255
    blue: new VariableAlias("attr4"),

    init: function(uniqueId, kwargs) {
        this._super(uniqueId, kwargs);

        // Set up some nice defaults
        this.radius = 100;
        this.red = 128;
        this.green = 128;
        this.blue = 128;
    }
});


//! A particle effect. Until this is better documented, see the Sauer documentation for
//! the meanings of particle_type and value1-3 (editref.html in docs/).
ParticleEffect = StaticEntity.extend({
    _class: "ParticleEffect",

    _sauerTypeIndex: 5,

    //! Override the parent attr with nicer-named ones
    attr1: new WrappedCInteger({ cGetter: 'CAPI.getAttr1', cSetter: 'CAPI.setAttr1', guiName: "particle_type", alt_name: "particle_type" }),

    //! Override the parent attr with nicer-named ones
    attr2: new WrappedCInteger({ cGetter: 'CAPI.getAttr2', cSetter: 'CAPI.setAttr2', guiName: "value1", altName: "value1" }),

    //! Override the parent attr with nicer-named ones
    attr3: new WrappedCInteger({ cGetter: 'CAPI.getAttr3', cSetter: 'CAPI.setAttr3', guiName: "value2", altName: "value2" }),

    //! Override the parent attr with nicer-named ones
    attr4: new WrappedCInteger({ cGetter: 'CAPI.getAttr4', cSetter: 'CAPI.setAttr4', guiName: "value3", altName: "value3" }),

    //! The type of particle effect. See Sauer.
    particleType: new VariableAlias("attr1"),

    //! A parameter of this effect. See Sauer.
    value1: new VariableAlias("attr2"),

    //! A parameter of this effect. See Sauer.
    value2: new VariableAlias("attr3"),

    //! A parameter of this effect. See Sauer.
    value3: new VariableAlias("attr4"),

    init: function(uniqueId, kwargs) {
//        print "*** Init PE"
        this._super(uniqueId, kwargs);

        // Set up some nice defaults
        this.particleType = 0;
        this.value1 = 0;
        this.value2 = 0;
        this.value3 = 0;
    }
});


//! A mesh-based static entity in a map. Mapmodels have collision capabilities, i.e., you
//! can detect collisions between them and dynamic entities.
Mapmodel = StaticEntity.extend({
    _class: "Mapmodel",

    _sauerTypeIndex: 2,

    //! Yaw
    attr1: new WrappedCInteger({ cGetter: 'CAPI.getAttr1', cSetter: 'CAPI.setAttr1', guiName: "yaw", altName: "yaw" }),

    //! The yaw of the model. Sauer sets this to multiples of 15 degrees.
    yaw: new VariableAlias("attr1"),

    //! The radius used for collisions. This is only used for mapmodels that have 'perentitycollisionboxes' set to 1 in
    //! their .cfg file. Otherwise, the collision radius is taken from the model, i.e., it is the same for all instances
    //! of the model. The width is used for x and y radiuses of the collision box.
    collisionRadiusWidth: new WrappedCInteger({
        cGetter: 'CAPI.getCollisionRadiusWidth',
        cSetter: 'CAPI.setCollisionRadiusWidth'
    }),

    //! See Mapmodel.collisionRadiusWidth
    collisionRadiusHeight: new WrappedCInteger({
        cGetter: 'CAPI.getCollisionRadiusHeight',
        cSetter: 'CAPI.setCollisionRadiusHeight'
    }),

    init: function(uniqueId, kwargs) {
        log(DEBUG, "Mapmodel.init");

        this._super(uniqueId, kwargs);

        this.attr2 = -1; // This is the sauer mapmodel index - put it as '-1', to use our model names, as default
        this.yaw = 0; // TODO: Make dependent upon player yaw?

        // Need the following so that the C++ cached copies are in fact up to date.
        this.collisionRadiusWidth = 10;
        this.collisionRadiusHeight = 10;

        log(DEBUG, "Mapmodel.init complete");
    },

    clientActivate: function(kwargs) {
        this._super(kwargs);

        this.defaultAnimation = ANIM_TRIGGER | ANIM_START; // XXX why?
    },

    //! Called when a dynamic entity - like a player - collides with this mapmodel. This will be called
    //! multiple times - once per physics check, which is done many times per second. ResettableAreaTrigger is a class
    //! that gives an example of how to get a single event (no further calls occur until the trigger is reset).
    //! @param collider The colliding dynamic entity
    onCollision: function(collider) {
    },

    //! Called on the client when a dynamic entity - like a player - collides with this mapmodel.
    //! @param collider The colliding dynamic entity
    clientOnCollision: function(collider) {
    }
});


//
// Area triggers
//

//! A 'mapmodel' without a mesh, that is really just an area to detect collisions in, as an area trigger. For this to
//! work, the model it uses must have mdlcollisionsonlyfortriggering 1 and mdlperentitycollisionboxes 1 in the .cfg
//! file.
AreaTrigger = Mapmodel.extend({
    _class: "AreaTrigger",

    // The script that is run upon collision
    scriptToRun: new StateString(),

    init: function(uniqueId, kwargs) {
        this._super(uniqueId, kwargs);

        this.scriptToRun = "";

        this.modelName = "areatrigger"; // Hardcoded, an appropriate model with mdlcollisionsonlyfortriggering, mdlperentitycollisionboxes
    },

    onCollision: function(collider) {
        // XXX Should validate the scriptToRun, that it is the simple name of a function to be called. Passing
        // this to hasattr is a potential security risk.
        if (this.scriptToRun !== "") {
//            assert( hasattr(__main__, this.scriptToRun) )
            eval(this.scriptToRun + "(collider);"); // XXX Minor security risk
        }
    },

    clientOnCollision: function(collider) {
    },
});


//! An area trigger that, once triggers, will not trigger again until reset. Triggering state - can trigger or not
//! - is separate for client and server.
ResettableAreaTrigger = AreaTrigger.extend({
    _class: "ResettableAreaTrigger",

    activate: function(kwargs) {
        this._super(kwargs);
        this.reset()
    },

    clientActivate: function(kwargs) {
        this._super(kwargs);
        this.reset()
    },

    //! If a scriptToRun is defined, then it is run (by calling the parent's onCollision). Otherwise, we
    //! run a method called onTrigger, which can be overridden with specific behavior.
    //! Note that readyToTrigger is *NOT* a state variable. It doesn't persist, nor is it synced between
    //! client and server. The latter lets client and server triggering states be separate.
    onCollision: function(collider) {
        if (this.readyToTrigger) {
            if (this.scriptToRun !== "") {
                this._super(collider);
            } else {
                this.onTrigger(collider);
            }

            this.readyToTrigger = false;
        }
    },

    clientOnCollision: function(collider) {
        if (this.readyToTrigger) {
            if (this.scriptToRun !== "") {
                this._super(collider);
            } else {
                this.clientOnTrigger(collider);
            }

            this.readyToTrigger = false;
        }
    },

    //! Call this to reset the trigger, so that entering the area will once more cause a call to on_trigger.
    reset: function() {
        this.readyToTrigger = true;
        if (Global.SERVER) {
            this.onReset();
        } else {
            this.clientOnReset();
        }
    },

    //! Called when the trigger is reset. Override this to do something at that time. TODO: Make this an event?
    onReset: function() {
    },

    //! Called when the trigger is reset. Override this to do something at that time. TODO: Make this an event?
    clientOnReset: function() {
    },

    //! Called when the trigger is fired. Override this to do something at that time. TODO: Make this an event?
    onTrigger: function(collider) {
    },

    //! Called when the trigger is fired. Override this to do something at that time. TODO: Make this an event?
    clientOnTrigger: function(collider) {
    }
});


//
// Animated mapmodels
//

//! Shows the 'triggered' animation for a mapmodel, e.g., open/close for a door, chest, etc.
TriggeredAction = Action.extend({
    //! @param reverse Whether to run the animation in reverse or not.
    init: function(_reverse) {
        this._super();

        this.secondsLeft = 1.0; // 1 second hardcoded for now. Probably the length hardcoded in Sauer for running Trigger anims.

        if (_reverse) {
            this.animation = ANIM_TRIGGER | ANIM_REVERSE;
        } else {
            this.animation = ANIM_TRIGGER;
        }
    },

    getAnimation: function() {
        return this.animation;
    },

    getAnimationFrame: function() {
        return 0; // TODO: Should depend on the time, a different frame for each stage of the animation
    }
});


/*
// New classes


//! A simple mapmodel which has two states, open and closed. XXX Currently this is in mostly-usable state,
//! but not 100% working. It is a proof of concept, basically.
class Door(Mapmodel):
    status_open, status_closed = range(2)

    //! Whether the door is open or closed
    status = StateEnum( [status_open, status_closed] )

    def __init__(this, *args, **kwargs):
        Mapmodel.__init__(this, *args, **kwargs)

        this.status = Door.status_closed
        this.radius = 10. // TODO: Use sauer values of bounding box

    def __client_activate__(this, *args, **kwargs):
        Mapmodel.__client_activate__(this, *args, **kwargs)

        //! Only after shown once do we show animations upon being given a new status
        this._status_set_already = False

        ListenerSet.subscribe(this, "client_on_modify_status", this.animate_status_changes)

    //! Flip the status of the door from closed to open or vice versa.
    def toggle_status(this):
        if this.status == Door.status_open:
            this.status = Door.status_closed
        else:
            this.status = Door.status_open

    //! When clicked, the player moves towards the door and then tries to open it
    def client_click(this):
        get_player_logic_entity().queue_action( ArrivalAction(this) ) // character moves towards us, the door. If succeed, continue
        get_player_logic_entity().queue_action( OpenAction(this)  )

    // TODO: Check not locked, blocked, etc. etc. Perhaps do it by running a collision check on 
    // a few of the other frames?
    def can_modify_status(this, value, actor_unique_id):
        return True

    //! Set up animation for the door to open/close, and for the opening agent as well.
    def animate_status_changes(this, value):
        // Set new default animation TODO: Do only after animation finishes, meanwhile set some 'in-progress' state?
        if this.status == Door.status_open:
            this.default_animation = ANIM_TRIGGER | ANIM_END
        else:
            this.default_animation = ANIM_TRIGGER | ANIM_START

        // If this is a change to a value (not the first initialization), then show an animation
        if this._status_set_already:
            triggered = TriggeredAction(value == Door.status_closed)
            this.queue_action( triggered )

            // Make the fiddle action now parallel to the opening animation, so it finishes with it, if there is indeed
            // a fiddle action going on (there isn't, if another client is opening the door. TODO: Need animation for that too)
            if hasattr(this, "fiddle_action"):
                this.fiddle_action.parallel_to = triggered
                this.fiddle_action = None // Avoid memory leaks
                del this.fiddle_action

        this._status_set_already = True

    //! Show an appropriate animation frame for the door, based on its current position and stage in animation.
    def get_animation_frame(this):
        // If we have no data on our status yet, or we have actions, then let the action system decide
        if not this.initialized or not this.action_system.is_empty():
            return this.action_system.get_animation_frame()
        else:
            // Return 0 or 30, depending on position. 30 is hard-coded, same as in door.cpp - FIXME, of course
            if this.status == Door.status_closed:
                return 0
            else:
                return 29

        return 0


//! An actor attempts to open a target Door.
class OpenAction(TargetedAction):
    def do_execute(this, seconds):
        if distance_between(this.actor.position, this.target.position) > 40: // FIXME: Should take into account bounding boxes, not just
                                                                     // distance between centers. Does Sauer already have code for this?
            if Global.SERVER:
                MessageSystem.send(this.actor, CAPI.PersonalServerMessage, -1, "Door", "Can't reach the door to open it." )
            else:
                print "Can't reach the door to open it." // Local message if we are not on the server

            return True // We have failed

        this.target.fiddle_action = Action(parallel_to = NeverEndingAction(), animation = ANIM_PUNCH | ANIM_LOOP)
        this.actor.queue_action(this.target.fiddle_action)
        this.target.toggle_status()

        return True // We have succeeded

*/



//! Marks a position in the world. That position can then be found using the entity's unique id, or
//! by a tag. This then gives us the position (x,y,z) that it marks.
//! So, this is useful to mark positions in the in-world 3D editor, then access them in this way in code.
//! See the tutorials for examples.
WorldMarker = StaticEntity.extend({
    _class: "WorldMarker",

    attr1: new WrappedCInteger({ cGetter: 'CAPI.getAttr1', cSetter: 'CAPI.setAttr1', guiName: "yaw", altName: "yaw" }),

    //! The yaw of this marker, useful for positioning the player in a particular facing
    yaw: new VariableAlias("attr1"),

    _sauerTypeIndex: 3,

    //! Make an entity be at the position of this marker, and with its yaw
    placeEntity: function(entity) {
        entity.position = this.position;
        entity.yaw = this.yaw;
    }
});


//! PlayerStart - DEPRECATED.
PlayerStart = StaticEntity.extend({
    _class: "PlayerStart",

    _sauerTypeIndex: 3
});


//
// Register classes
//

registerEntityClass(StaticEntity, "mapmodel");
registerEntityClass(Light, "light");
registerEntityClass(ParticleEffect, "particles");
registerEntityClass(Mapmodel, "mapmodel");
registerEntityClass(AreaTrigger, "mapmodel");
registerEntityClass(ResettableAreaTrigger, "mapmodel");
//registerEntityClass(Door, "mapmodel");
registerEntityClass(PlayerStart, "playerstart"); // TODO: Remove
registerEntityClass(WorldMarker, "playerstart");

