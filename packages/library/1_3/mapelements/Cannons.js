
// Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
// This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

Library.include('library/' + Global.LIBRARY_VERSION + '/AutoTargeting');
Library.include('library/' + Global.LIBRARY_VERSION + '/MultipartRendering');
Library.include('library/' + Global.LIBRARY_VERSION + '/CustomEffect');

//! Example use: registerEntityClass( bakePlugins(Mapmodel, [AutoTargetingPlugin, MultipartRenderingPlugin, MultipartRenderingAutotargetingPlugin, CannonPlugin]) ); // FiringPlugin
CannonPlugin = {
    _class: "Cannon",

    upperHeight: 5,
    firingRadius: 15,

    activate: function(kwargs) {
        this.modelName = "invisiblegeneric/ellipse";
        this.collisionRadiusHeight = 4.5;
        this.collisionRadiusWidth = 6.66;
    },

    createRenderingArgs: function(yaw, pitch, anim, o, flags, basetime) {
        return [
            [this, "cannon/base", anim, o.x, o.y, o.z, yaw, 0, flags, basetime],
            [this, "cannon/barrel", anim, o.x, o.y, o.z+this.upperHeight, yaw, pitch, flags, basetime]
        ];
    },

    getCenter: function() {
        var ret = this.position.copy();
        ret.z += this.upperHeight;
        return ret;
    },
};

UpsidedownCannonPlugin = {
    upperHeight: 4,

    createRenderingArgs: function(yaw, pitch, anim, o, flags, basetime) {
        return [
            [this, "cannon/base", anim, o.x, o.y, o.z+9, yaw+180, 180, flags, basetime],
            [this, "cannon/barrel", anim, o.x, o.y, o.z+9-this.upperHeight, yaw+180, 180-pitch, flags, basetime]
        ];
    },

    getCenter: function() {
        var ret = this.position.copy();
        ret.z += this.upperHeight;
        return ret;
    },
};

//! Plugin for the cannon's guns, so they shoot from the right place
CannonGunPlugin = {
    getOrigin: function(shooter) {
        var ret = shooter.autoTargetDirection.copy().mul(shooter.firingRadius);
        ret.add(shooter.position);
        ret.z += shooter.upperHeight;
        return ret;
    },
};

//! Plugin to make the cannon shootable
CannonHealthPlugin = {
    maxHealth: 30,

    healthSystem: 'regen',

    activate: function() {
        this.health = this.maxHealth;

        if (this.healthSystem === 'regen') {
            this.connect('onModify_canAutoTarget', function(value) {
                if (value === false) {
                    this.disabledEvent = GameManager.getSingleton().eventManager.add({
                        secondsBefore: 6,
                        func: bind(function() {
                            this.health = this.maxHealth;
                            this.canAutoTarget = true;
                        }, this),
                        entity: this,
                    }, this.disabledEvent);
                }
            });
        }
    },

    clientActivate: function() {
        this.health = this.maxHealth;

        this.connect('client_onModify_canAutoTarget', function(value) {
            if (value === false) {
                if (this.healthSystem === 'regen') {
                    this.disabledVisualEvent = GameManager.getSingleton().eventManager.add({
                        secondsBefore: 0,
                        secondsBetween: 0,
                        func: bind(function() {
                            if (this.canAutoTarget) {
                                return false; // We are done
                            }
                            Effect.flame(PARTICLE.SMOKE, this.getCenter(), 3.5, 1.5, 0x000000, 1, 7.0, 100, Math.max(Global.currTimeDelta*50, 0.5), -15);
                        }, this),
                        entity: this,
                    }, this.disabledVisualEvent);
                } else {
                    this.renderDynamic = function() { };
                    this.collisionRadiusWidth = 0.1;
                    this.collisionRadiusHeight = 0.1;

                    CustomEffect.explosion(this.center, this.position, 30, 0xFF775F, 10, this);
                }
            } else {
                this.health = this.maxHealth;
            }
        });
    },

    sufferDamage: function(kwargs) {
        this.health -= kwargs.damage;

        if (this.health <= 0) {
            if (this.canAutoTarget) {
                this.canAutoTarget = false;
            }
        }
    },
};


//! Creates cannon entity classes
function makeCannon(_name, gunClass, additionalPlugins) {
    additionalPlugins = defaultValue(additionalPlugins, []);

    var CannonGun = bakePlugins(gunClass, [CannonGunPlugin]);
    var cannonGun = new CannonGun();
    Firing.registerGun(cannonGun);
    var plugins = [AutoTargetingPlugin, MultipartRenderingPlugin, MultipartRenderingAutotargetingPlugin, CannonPlugin, Firing.plugins.protocol, BotFiringPlugin, CannonHealthPlugin, { _class: _name, gun: cannonGun }];
    return registerEntityClass( bakePlugins(Mapmodel, plugins.concat(additionalPlugins)) );
}

