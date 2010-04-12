
// Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
// This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

Pickups = {
    managerPlugin: {
        newPickup: new StateJSON({ hasHistory: false }),
        removePickup: new StateInteger({ hasHistory: false }),

        pickupTypes: {}, // Fill this with data *not* sent over the wire. Like render(), doPickup(),

        activate: function() {
            this.pickups = [];
            this.pickupsCounter = 0;
        },

        act: function(seconds) {
            this.pickups = filter(function(pickup) {
//                if (pickup.secondsLeft < 0) return true; // Never expires
                pickup.secondsLeft -= seconds;
                if (pickup.secondsLeft > 0) {
                    return true;
                } else {
                    this.removePickup = pickup.id;
                    return false;
                }
            }, this.pickups, this);
        },

        clientActivate: function() {
            this.pickups = [];

            this.connect('client_onModify_newPickup', function(pickup) {
                this.pickups.push(merge(pickup, { position: new Vector3(pickup.position) }, this.pickupTypes[pickup.type]));
            });
            this.connect('client_onModify_removePickup', function(id) {
                this.pickups = filter(function(pickup) { return pickup.id !== id }, this.pickups, this);
            });

            this.pickupsTimer = new RepeatingTimer(1/10);
        },

        clientAct: function(seconds) {
            if (this.pickupsTimer.tick(seconds)) {
                var player = getPlayerEntity();
                if (!Health.isActiveEntity(player)) return;
                this.pickups = filter(function(pickup) {
                    if (pickup.render) {
                        pickup.render();
                    }
                    if (World.isPlayerCollidingEntity(player, pickup)) {
                        Sound.play('0ad/fs_sand7.ogg', player.position.copy());
                        GameManager.getSingleton().removePickup = pickup.id;
                        pickup.doPickup(player);
                        return false;
                    }
                    pickup.secondsLeft -= seconds;
                    return pickup.secondsLeft > 0;
                }, this.pickups, this);
            }
        },

        renderDynamic: function() {
            for (var i = 0; i < this.pickups.length; i++) {
                var pickup = this.pickups[i];

                if (!pickup.renderDynamic) continue;

                if (pickup.useRenderDynamicTest) {
                    if (!pickup.renderDynamicTest) {
                        Rendering.setupDynamicTest(pickup);
                    }

                    if (!pickup.renderDynamicTest()) continue;
                }

                pickup.renderDynamic();
            }
        },

        addPickup: function(pickup) {
            pickup.id = this.pickupsCounter;
            this.pickupsCounter = this.pickupsCounter === 120 ? 0 : this.pickupsCounter + 1;

            if (!pickup.floating) {
                pickup.position.z -= floorDistance(pickup.position, 256) - 1;
            }

            this.newPickup = merge(pickup, { position: pickup.position.asArray() });
            this.pickups.push(merge(pickup, this.pickupTypes[pickup.type]));
        },
    },

    //! Spawns pickups in a repeating way at a spot
    Spawner: registerEntityClass(bakePlugins(WorldMarker, [{
        _class: 'PickupSpawner',

        pickupType: new StateString(),

        init: function() {
            this.pickupType = 'hp';
        },

        activate: function() {
            this.fill();
            // Respawning events
            var that = this;
            GameManager.getSingleton().eventManager.add({
                secondsBefore: 0,
                secondsBetween: GameManager.getSingleton().pickupTypes[this.pickupType].secondsLeft,
                func: bind(that.fill, that),
                entity: this,
            });
        },

        fill: function() {
            GameManager.getSingleton().addPickup({
                position: this.position.copy(),
                type: this.pickupType,
            });
        },
    }])),
};

