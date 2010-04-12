
// Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
// This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

Library.include('library/1_1/Health');


AutoTargeting = {
    managerPlugin: {
        disableAutoTargeting: new StateBoolean(), // Disable, so if doesn't exist (plugin not used), assume enable in scripts

        init: function() {
            this.disableAutoTargeting = false;
        },
    },
};

AutoTargetingPlugin = {
    shouldAct: true, // Necessary for entities that are static by default - make sure they will act

    canAutoTarget: new StateBoolean(), // Whether we are actively autotargeting
    autoTargetUniqueId: new StateInteger(), // The ID of the current target
    autoTargetParams: new StateJSON(), // Various parameters, like speed, distance, etc. - changes rarely
    autoTargetFullSync: new StateArrayFloat({ reliable: false }),

    // Client & Server

    setupAutoTargeting: function() {
        this.autoTargetYaw = 0;
        this.autoTargetPitch = 0;
        this.autoTargetEntity = null;
        this.autoTargetDirection = new Vector3(0,0,0);
    },

    getTargetingOrigin: function() {
        var ret = this.position.copy();
        ret.z += this.autoTargetParams.eyeHeight;
        return ret;
    },

    autoTargetInterpolate: function(seconds) {
        this.autoTargetEntity = getEntity(this.autoTargetUniqueId);

        if (this.autoTargetEntity === null || !this.isValidTarget(this.autoTargetEntity)) {
            this.autoTargetEntity = null;
            this.autoTargetingError = 180.0; // Not aiming at anything
            return; // No autoTarget
        }

        this.autoTargetPosition = this.autoTargetEntity.getCenter();
        var position = this.getTargetingOrigin();

        var autoTargetYaw = normalizeAngle( yawTo(position, this.autoTargetPosition), this.autoTargetYaw );
        var autoTargetPitch = this.fixPitch( normalizeAngle( pitchTo(position, this.autoTargetPosition), this.autoTargetPitch ) );

        this.autoTargetYaw = moveAngleTowards(this.autoTargetYaw, autoTargetYaw, seconds*this.autoTargetParams.rotateSpeed);
        this.autoTargetPitch = moveAngleTowards(this.autoTargetPitch, autoTargetPitch, seconds*this.autoTargetParams.rotateSpeed);
        this.autoTargetingError = Math.max( Math.abs(this.autoTargetYaw - autoTargetYaw), Math.abs(this.autoTargetPitch - autoTargetPitch) );
        this.autoTargetDirection = this.autoTargetPosition.subNew(position).normalize();
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
            searchRadius: 100,
            seekDelay: 0.5, // How often to search for an entity to target
            fullSyncRate: 1.0/15,
        };
    },

    activate: function() {
        this.canAutoTarget = true;

        this.currAutoTarget = null;
        this.currAutoTargetTimer = new RepeatingTimer(this.autoTargetParams.seekDelay);

        this.autoTargetFullSyncTimer = new RepeatingTimer(this.autoTargetParams.fullSyncRate);

        this.setupAutoTargeting();
    },

    act: function(seconds) {
        if (!this.canAutoTarget) {
            if (this.autoTargetUniqueId !== -1) {
                this.autoTargetUniqueId = -1;
            }
            return;
        }

        if (this.currAutoTargetTimer.tick(seconds)) {
            var pairs = getCloseEntities(this.position, this.autoTargetParams.searchRadius, Player);

            this.currAutoTarget = null;

            var newUniqueId = -1;

            for (var i = 0; i < pairs.length; i++) {
                var entity = pairs[i][0];

                if (this.isValidTarget(entity)) {
                    this.currAutoTarget = entity;
                    newUniqueId = entity.uniqueId;
                }
            }

            // Send only if different
            if (newUniqueId !== this.autoTargetUniqueId) {
                this.autoTargetUniqueId = newUniqueId;
            }
        }

        this.autoTargetInterpolate(seconds);

        if (this.autoTargetFullSyncTimer.tick(seconds)) {
            this.autoTargetYaw = normalizeAngle(this.autoTargetYaw, 0);
            this.autoTargetPitch = normalizeAngle(this.autoTargetPitch, 0);

            var newUpdate = [
                this.autoTargetYaw,
                this.autoTargetPitch,
            ];

            // Send if never sent, or about 1/second, and certainly if changed
            if (!this.lastAutoTargetFullSync || Math.random() < seconds || !arrayEqual(this.lastAutoTargetFullSync, newUpdate)) {
                this.autoTargetFullSync = newUpdate;
                this.lastAutoTargetFullSync = newUpdate;
            }
        }
    },

    // Client

    clientActivate: function() {
        this.setupAutoTargeting();

        this.oldAutoTargetYaw = this.autoTargetYaw;
        this.oldAutoTargetPitch = this.autoTargetPitch;

        this.connect('client_onModify_autoTargetFullSync', function(fullSync) {
            if (fullSync.length > 0) { // Ignore empty initializations
                var yaw = fullSync[0];
                var pitch = fullSync[1];

                this.autoTargetYaw = normalizeAngle(this.autoTargetYaw, yaw);
                this.autoTargetPitch = normalizeAngle(this.autoTargetPitch, pitch);

                this.autoTargetYaw = (this.autoTargetYaw + yaw)/2;
                this.autoTargetPitch = (this.autoTargetPitch + pitch)/2;

                this.autoTargetYawDir = angleDirectionChange(this.autoTargetYaw, this.oldAutoTargetYaw);
                this.autoTargetPitchDir = angleDirectionChange(this.autoTargetPitch, this.oldAutoTargetPitch);

                this.oldAutoTargetYaw = this.autoTargetYaw;
                this.oldAutoTargetPitch = this.autoTargetPitch;
            }
        });
    },

    clientAct: function(seconds) {
        if (!this.canAutoTarget) return;

        var oldYaw = this.autoTargetYaw;
        var oldPitch = this.autoTargetPitch;

        this.autoTargetInterpolate(seconds);

        this.preventClientAutoTargetStutter(oldYaw, oldPitch);
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

    // Override with the gun to use
    gun: null,

    init: function() {
        this.botFiringParams = {
            firingDelay: this.gun.delay, // Delay between shots due to gun reload, etc.
            triggerFingerDelay: 0.33, // Delay between aiming at target to actually pulling trigger
        };
    },

    activate: function() {
        this.firingTimer = new RepeatingTimer(this.botFiringParams.firingDelay);
    },

    act: function(seconds) {
        if (this.firingTimer.tick(seconds) && this.canAutoTarget) {
            if (this.currAutoTarget && this.autoTargetingError <= 5) {
                if (!this.triggerFingerTimer) {
                    this.triggerFingerTimer = new RepeatingTimer(this.botFiringParams.triggerFingerDelay);
                }
                if (this.triggerFingerTimer.tick(seconds)) {
                    this.gun.doShot(this, this.autoTargetPosition, this.currAutoTarget);
                    this.triggerFingerTimer = null;
                } else {
                    this.firingTimer.prime(); // Still ready to shoot
                }
            } else {
                this.firingTimer.prime(); // Shoot on sight next time
                this.triggerFingerTimer = null;
            }
        }
    },

    isValidTarget: function(target) {
        return Health.isValidTarget.call(this, target) && !GameManager.getSingleton().disableAutoTargeting && hasLineOfSight(this.getTargetingOrigin(), target.getCenter());
    },
};

