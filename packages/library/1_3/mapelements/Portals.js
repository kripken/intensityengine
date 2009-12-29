
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


Portal = registerEntityClass(bakePlugins(AreaTrigger, [{
    _class: 'Portal',
    shouldAct: true,

    targetIP: new StateString(),
    targetPort: new StateInteger(),

    visualModel: new StateString(),
    portalText: new StateString(),

    startPortalling: new StateInteger({ clientSet: true, hasHistory: false }),

    init: function() {
        this.visualModel = 'portal/map2';
        this.collisionRadiusWidth = 30;
        this.collisionRadiusHeight = 25;
        this.targetIP = 'www.syntensity.com';
        this.targetPort = 10000;
        this.portalText = '';
    },

    clientActivate: function() {
        this.connect('client_onModify_startPortalling', function(value) {
            var entity = getEntity(value);
            if (entity) { // check, as may have just vanished
                entity.queueAction(new PortallingAction(this));
            }
        });

        this.counter = 0;
    },

    clientAct: function(seconds) {
        this.counter += seconds;

        if (!this.portalling && getPlayerEntity().clientState !== CLIENTSTATE.EDITING) {
            var playerCenter = getPlayerEntity().getCenter();
            var upDownDist = Math.abs(playerCenter.z - this.position.z - this.collisionRadiusHeight);

            var myPlane = new Vector3().fromYawPitch(this.yaw + 90, 0);
            var frontBackDist = Math.abs(this.position.subNew(playerCenter).scalarProduct(myPlane));

            myPlane = new Vector3().fromYawPitch(this.yaw, 0);
            var leftRightDist = Math.abs(this.position.subNew(playerCenter).scalarProduct(myPlane));

            var closeEnough = (upDownDist < this.collisionRadiusHeight)  &&
                              (leftRightDist < this.collisionRadiusWidth) &&
                              (frontBackDist < getPlayerEntity().radius*2);

            if (closeEnough) {
                this.startPortalling = getPlayerEntity().uniqueId;
            }
        }

        if (this.portalText) {
            Effect.text(this.position.addNew(new Vector3(0, 0, this.collisionRadiusHeight*2.5)), this.portalText, 0.0, 0xFFDD99, 8.0, 0);
        }
    },

    renderDynamic: function() {
        var o = this.position;
        var flags = MODEL.LIGHT | MODEL.CULL_VFC | MODEL.FULLBRIGHT | MODEL.CULL_DIST;// optional: | MODEL.DYNSHADOW;
        var yaw = this.yaw + Math.sin(this.counter*1.5)*5;
        var pitch = Math.sin(this.counter*1.14159)*4;
        var args = [this, this.visualModel, ANIM_IDLE|ANIM_LOOP, o.x, o.y, o.z, yaw, pitch, flags, 0];
        CAPI.renderModel.apply(this, args);
    },

    clientClick: function() {
    },
}]));


PortallingAction = Action.extend({
    _name: 'PortallingAction',
    canMultiplyQueue: false,
    canBeCancelled: false,
    secondsLeft: 7,

    create: function(portal, respawn) {
        this._super();

        this.portal = portal;
        this.respawn = defaultValue(respawn, true);
    },

    doStart: function() {
        this.actor.portalling = true;

        this.oldMovementSpeed = this.actor.movementSpeed;
        this.actor.movementSpeed /= 10;

        Sound.play('olpc/Berklee44BoulangerFX/rubberband.wav', this.actor.getCenter());

        if (this.actor === getPlayerEntity()) {
            log(DEBUG, 'Going to:' + this.portal.targetIP + ':' + this.portal.targetPort);
            Network.connect(this.portal.targetIP, this.portal.targetPort);
        }
    },

    doExecute: function(seconds) {
        var where = this.actor.position.copy();
        where.z += this.actor.eyeHeight*1.5;
        Effect.splash(PARTICLE.SPARK, integer(500*seconds), 0.5, where, 0x779BB2, 1.0, 70, 1);

        return this._super.apply(this, arguments);
    },

    doFinish: function() {
        this.actor.portalling = false;
        this.actor.movementSpeed = this.oldMovementSpeed;

        if (this.secondsLeft < 0.1 && this.actor.respawn && this.respawn) {
            this.actor.respawn(); // Time ran out - we failed
            UserInterface.showMessage('Teleporting failed - wait a bit, then try again');
        }
    }
});



DynaPortals = {
    RADIUS: 10,

    managerPlugin: {
        newDynaPortal: new StateJSON({ hasHistory: false }),

        clientActivate: function() {
            this.dynaPortals = [];

            this.connect('client_onModify_newDynaPortal', function(portal) {
                portal.position = new Vector3(portal.x, portal.y, portal.z);
                this.dynaPortals.push(portal);
            });
        },

        clientAct: function(seconds) {
            this.dynaPortals = filter(function(portal) {
                // Draw
                portal.secondsLeft -= seconds;
                var timer = portal.secondsLeft*4;
                for (var i = 0; i < 10; i++) {
                    Effect.flame(PARTICLE.FLAME, portal.position.addNew(new Vector3(Math.sin(timer + 2*Math.PI*i/10)*DynaPortals.RADIUS*1.1, Math.cos(timer + 2*Math.PI*i/10)*DynaPortals.RADIUS*1.1, 0)), 0.25, 1.5, 0xBB8877, 2, 3.0, 200, 0.5, -6);
                }
                if (portal.text) {
                    Effect.text(portal.position.addNew(new Vector3(0, 0, 40)), portal.text, 0.0, 0xFFDD99, 8.0, 0);
                }

                // Portal away if we should
                var player = getPlayerEntity();
                if (player.position.isCloseTo(portal.position, DynaPortals.RADIUS) && player.portalled != portal) {
                    player.queueAction(new PortallingAction(portal, false));
                    player.portalled = portal;
                }

                return portal.secondsLeft >= 0;
            }, this.dynaPortals);
        },
    },

//transmit over network!
    clientClick: function(button, down, position) {
        if (button !== 2 || !down) return;

        var portal = { x: position.x, y: position.y, z: position.z };

        UserInterface.showInputDialog('Enter the portal target IP (blank for syntensity.com):', function(ip) {
            portal.targetIP = ip ? ip : 'www.syntensity.com';

            UserInterface.showInputDialog('Enter the portal target port:', function(port) {
                portal.targetPort = port;

                UserInterface.showInputDialog('Enter the portal duration in seconds (blank for 30, max 60):', function(secondsLeft) {
                    portal.secondsLeft = secondsLeft ? Math.min(secondsLeft, 60) : 30;

                    GameManager.getSingleton().newDynaPortal = portal;
                });
            });
        });
    },
};

