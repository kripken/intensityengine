
// Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
// This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

Library.include('library/' + Global.LIBRARY_VERSION + '/Health');


AutoTargeting = {
    managerPlugin: {
        disableAutoTargeting: new StateBoolean(), // Disable, so if doesn't exist (plugin not used), assume enable in scripts

        init: function() {
            this.disableAutoTargeting = false;
        },
    },
};

AutoTargetingPlugin = {
    canAutoTarget: new StateBoolean(), // Whether we are actively autotargeting
    autoTargetUniqueId: new StateInteger(), // The ID of the current target
    autoTargetParams: new StateJSON(), // Various parameters, like speed, distance, etc. - changes rarely
    autoTargetFullSync: new StateArrayFloat({ reliable: false }),
    autoTargetSearchRadius: new StateFloat(),

    // Client & Server

    setupAutoTargeting: function() {
        this.autoTargetYaw = 0;
        this.autoTargetPitch = 0;

        this.autoTargetEntity = null;
        this.autoTargetDirection = new Vector3(0,0,0);

        this.targetingSuspended = false;

        this.connect('suspendTargeting', function() {
            GameManager.getSingleton().eventManager.suspend(this.autoTargetInterpolateEvent);
            GameManager.getSingleton().eventManager.suspend(this.autoTargetSyncEvent);
        });

        this.connect('awakenTargeting', function() {
            GameManager.getSingleton().eventManager.awaken(this.autoTargetInterpolateEvent);
            GameManager.getSingleton().eventManager.awaken(this.autoTargetSyncEvent);
        });
    },

    suspendTargeting: function() {
        if (!this.targetingSuspended) {
            this.emit('suspendTargeting');
            this.targetingSuspended = true;
        }
    },

    awakenTargeting: function() {
        if (this.targetingSuspended) {
            this.emit('awakenTargeting');
            this.targetingSuspended = false;
        }
    },

    getTargetingOrigin: function() {
        var ret = this.position.copy();
        ret.z += this.autoTargetParams.eyeHeight;
        return ret;
    },

    autoTargetInterpolate: function(seconds) {
        seconds = defaultValue(seconds, this.autoTargetParams.interpolateRate);

        this.autoTargetEntity = getEntity(this.autoTargetUniqueId);

        if (this.autoTargetEntity === null || !this.isValidTarget(this.autoTargetEntity)) {
            this.autoTargetEntity = null;
            this.autoTargetingError = 180.0; // Not aiming at anything
            return -seconds*4; // No autoTarget, sleep
        }

        this.autoTargetPosition = this.autoTargetEntity.getCenter();
        var position = this.getTargetingOrigin();

        var autoTargetYaw = normalizeAngle( yawTo(position, this.autoTargetPosition), this.autoTargetYaw );
        var autoTargetPitch = this.fixPitch( normalizeAngle( pitchTo(position, this.autoTargetPosition), this.autoTargetPitch ) );

        this.autoTargetDirection = this.autoTargetPosition.subNew(position).normalize();

        this.doAutoTargetInterpolate(seconds, autoTargetYaw, autoTargetPitch);
    },

    doAutoTargetInterpolate: function(seconds, autoTargetYaw, autoTargetPitch) {
        this.autoTargetYaw = moveAngleTowards(this.autoTargetYaw, autoTargetYaw, seconds*this.autoTargetParams.rotateSpeed);
        this.autoTargetPitch = moveAngleTowards(this.autoTargetPitch, autoTargetPitch, seconds*this.autoTargetParams.rotateSpeed);
        this.autoTargetingError = Math.max( Math.abs(this.autoTargetYaw - autoTargetYaw), Math.abs(this.autoTargetPitch - autoTargetPitch) );
    },

    fixPitch: function(pitch) {
        return Math.min(this.autoTargetParams.maxPitch, Math.max(this.autoTargetParams.minPitch, pitch));
    },

    // Server

    init: function() {
        this.autoTargetParams = {
            rotateSpeed: 90,
            minPitch: -45,
            maxPitch: 45,
            eyeHeight: 10, // Where we shoot from
            effectiveSearchRadius: null, // If we define a function here, it can take into account where we look, not jus distance, etc.
            seekDelay: 0.5, // How often to search for an entity to target
            fullSyncRate: 1.0/15,
            interpolateRate: 1/60,
        };
        this.autoTargetSearchRadius = 100;
    },

    activate: function() {
        this.canAutoTarget = true;

        this.currAutoTarget = null;

        this.setupAutoTargeting();

        this.connect('onModify_autoTargetParams', function(params) {
            this.autoTargetSeekEvent = GameManager.getSingleton().eventManager.add({
                secondsBefore: 0,
                secondsBetween: params.seekDelay,
                func: bind(function() {
                    if (!this.canAutoTarget) {
                        if (this.autoTargetUniqueId !== -1) {
                            this.autoTargetUniqueId = -1;
                        }
                        this.suspendTargeting();
                        return;
                    }

                    var origin = this.position.copy();
                    var searchRadius = this.autoTargetSearchRadius;
                    var effectiveSearchRadius = params.effectiveSearchRadius;
                    var pairs = getCloseEntities(origin, searchRadius, Player, undefined, !!effectiveSearchRadius);
                    if (!!effectiveSearchRadius) {
                        forEach(pairs, function(pair) {
                            // Decide effective distance factor
                            pair[1] = effectiveSearchRadius(this, pair[0], pair[1]);
                        }, this);
                        pairs.sort(function(a, b) { return (b[1] - a[1]); });
                    }
                    pairs = filter(function(pair) { return pair[1] <= searchRadius; }, pairs);

                    this.currAutoTarget = null;
                    var newUniqueId = -1;
                    for (var i = 0; i < pairs.length; i++) {
                        var entity = pairs[i][0];
        //                var effectiveDistance = pairs[i][1];

                        if (this.isValidTarget(entity)) {
                            this.currAutoTarget = entity;
                            newUniqueId = entity.uniqueId;
                        }
                    }

                    if (newUniqueId >= 0) {
                        this.awakenTargeting();
                    } else {
                        this.suspendTargeting();
                    }

                    // Send only if different
                    if (newUniqueId !== this.autoTargetUniqueId) {
                        this.autoTargetUniqueId = newUniqueId;
                    }
                }, this),
                entity: this,
            }, this.autoTargetSeekEvent);

            this.autoTargetInterpolateEvent = GameManager.getSingleton().eventManager.add({
                secondsBefore: 0,
                secondsBetween: params.interpolateRate,
                func: bind(this.autoTargetInterpolate, this),
                entity: this,
            }, this.autoTargetInterpolateEvent);

            this.autoTargetSyncEvent = GameManager.getSingleton().eventManager.add({
                secondsBefore: 0,
                secondsBetween: params.fullSyncRate,
                func: bind(function() {
//log(ERROR, this.autoTargetSyncEvent.sleeping);
                    this.autoTargetYaw = normalizeAngle(this.autoTargetYaw, 0);
                    this.autoTargetPitch = normalizeAngle(this.autoTargetPitch, 0);

                    var newUpdate = [
                        this.autoTargetYaw,
                        this.autoTargetPitch,
                    ];

                    // Send if never sent, or about 1/second, and certainly if changed
                    if (!this.lastAutoTargetFullSync || Math.random() < Global.currFrameTime || !arrayEqual(this.lastAutoTargetFullSync, newUpdate)) {
                        this.autoTargetFullSync = newUpdate;
                        this.lastAutoTargetFullSync = newUpdate;
                    }
                }, this),
                entity: this,
            }, this.autoTargetSyncEvent);
        });

        this.autoTargetParams = this.autoTargetParams; // Force setting up of event

        // Backwards compat
        if (!this.autoTargetParams.interpolateRate) {
            var params = this.autoTargetParams;
            params.interpolateRate = 1/60;
            this.autoTargetParams = params;
        }
    },

    // Client

    clientActivate: function() {
        this.setupAutoTargeting();

        this.oldAutoTargetYaw = this.autoTargetYaw;
        this.oldAutoTargetPitch = this.autoTargetPitch;

        this.serverUpdateTime = -1;

        this.connect('client_onModify_autoTargetFullSync', function(fullSync) {
            if (fullSync.length > 0) { // Ignore empty initializations
                this.serverYaw = fullSync[0];
                this.serverPitch = fullSync[1];
                this.serverUpdateTime = Global.time;
            }
        });

        this.connect('client_onModify_autoTargetParams', function(params) {
            this.autoTargetParamsEvent = GameManager.getSingleton().eventManager.add({
                secondsBefore: 0,
                secondsBetween: params.interpolateRate,
                func: bind(function() {
                    var oldYaw = this.autoTargetYaw;
                    var oldPitch = this.autoTargetPitch;

                    var syncTime = Global.time - this.serverUpdateTime;
                    if (syncTime < this.autoTargetParams.fullSyncRate && this.serverUpdateTime >= 0) {
                        syncTime = Math.min(this.autoTargetParams.fullSyncRate - syncTime, this.autoTargetParams.interpolateRate);
                        this.doAutoTargetInterpolate(syncTime, this.serverYaw, this.serverPitch);
                        this.autoTargetInterpolate(this.autoTargetParams.interpolateRate - syncTime);
                    } else {
                        this.autoTargetInterpolate();
                    }
                    this.preventClientAutoTargetStutter(oldYaw, oldPitch);
                    if (!this.autoTargetEntity) return this.autoTargetParams.interpolateRate*4; // sleep
                }, this),
                entity: this,
            }, this.autoTargetParamsEvent);
        });
    },

    preventClientAutoTargetStutter: function(oldYaw, oldPitch) {
        var clientYawDir = angleDirectionChange(this.autoTargetYaw, oldYaw);
        var clientPitchDir = angleDirectionChange(this.autoTargetPitch, oldPitch);

        var serverYawDir = this.autoTargetYawDir;
        var serverPitchDir = this.autoTargetPitchDir;

        if (clientYawDir * serverYawDir < 0) {
            this.autoTargetYaw = oldYaw;
        }

        if (clientPitchDir * serverPitchDir < 0) {
            this.autoTargetPitch = oldPitch;
        }
    },

};

//! Plugin for multipart rendering that accesses the auto targeting
//! yaw and pitch values.
MultipartRenderingAutotargetingPlugin = {
    getMultipartYaw: function() {
        return this.autoTargetYaw-90;
    },

    getMultipartPitch: function() {
        return -this.autoTargetPitch;
    },
};

//! Fires shots for an NPC/bot using the auto-targeting system
BotFiringPlugin = {
    botFiringParams: new StateJSON(),
    botAccuracy: new StateFloat(),

    // Override with the gun to use
    gun: null,

    init: function() {
        this.botFiringParams = {
            firingDelay: this.gun.delay, // Delay between shots due to gun reload, etc.
            triggerFingerDelay: 0.33, // Delay between aiming at target to actually pulling trigger
        };
        this.botAccuracy = 0.5;
    },

    activate: function() {
        if (this.botAccuracy === undefined) {
            this.botAccuracy = 0.5; // Backwards compatibility
        }

        this.triggerFingerDelay = 0;

        this.connect('suspendTargeting',  function() {
            GameManager.getSingleton().eventManager.suspend(this.botFiringEvent);
        });

        this.connect('awakenTargeting', function() {
            GameManager.getSingleton().eventManager.awaken(this.botFiringEvent, this.botFiringParams.firingDelay);
        });

        this.botFiringEvent = GameManager.getSingleton().eventManager.add({
            secondsBefore: 0,
            secondsBetween: 0, // We add delay ourselves, flexibly
            func: bind(function() {
                if (!this.canAutoTarget) {
                    return -this.botFiringParams.firingDelay; // Sleep
                }
                if (this.autoTargetingError <= 5) {
                    this.triggerFingerDelay -= Global.currTimeDelta;
                    if (this.triggerFingerDelay <= 0) {
                        if (Math.random() < this.botAccuracy) {
                            this.gun.doShot(this, this.autoTargetPosition, this.currAutoTarget);
                        } else {
                            this.gun.doShot(this, this.autoTargetPosition.addNew(Random.normalizedVector3().mul(Math.random()*5)));
                        }
                        this.triggerFingerDelay = this.botFiringParams.triggerFingerDelay/2;
                        return -this.botFiringParams.firingDelay;
                    }
                } else {
                    this.triggerFingerDelay = this.botFiringParams.triggerFingerDelay/2;
                    if (this.autoTargetEntity) {
                        return this.botFiringParams.triggerFingerDelay/2;
                    } else {
                        return -this.botFiringParams.firingDelay; // Sleep
                    }
                }
            }, this),
            entity: this,
        }, this.botFiringEvent);
    },

    isValidTarget: function(target) {
        return Health.isValidTarget.call(this, target) && !GameManager.getSingleton().disableAutoTargeting && hasLineOfSight(this.getTargetingOrigin(), target.getCenter());
    },
};

AutoTargetingCharacterPlugin = {
    act: function() {
        this.yaw = this.autoTargetYaw;
        this.pitch = this.autoTargetPitch;
    },

    isValidTarget: BotFiringPlugin.isValidTarget,
};

