
// Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
// This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

Library.include('library/1_2/Plugins');
Library.include('library/1_2/mapelements/WorldAreas');

// An area in the world where you can tinker with your models.
//  * Shows a model
//  * Lets you reload it, so you can change the config and see the results

ModelForge = registerEntityClass(bakePlugins(AreaTrigger, [WorldAreas.plugins.Core, {
    _class: 'ModelForge',

    forgedModelName: new StateString({ clientSet: true }), // Need clientSet, to recover from reloading errors - or else the loadmodel
                                                           // after it would kill us
    forgedModelAnimation: new StateInteger(),

    init: function() {
        this.forgedModelName = '';
        this.forgedModelAnimation = ANIM_IDLE|ANIM_LOOP;
    },

    resetForgedModel: function() {
        this.forgedModelBasetime = 0;
        this.forgedModelChanged = true;
    },

    clientActivate: function() {
        this.resetForgedModel();

        var that = this;

        this.worldArea.actionKey = function(index, down) {
            if (!down) return;
            if (index === 1) {
                UserInterface.showInputDialog('Enter the model name:', function(modelName) {
                    that.forgedModelName = modelName;
                });
            } else if (index === 2) {
                UserInterface.showInputDialog('Enter the model animation (e.g. ANIM_IDLE|ANIM_LOOP):', function(animation) {
                    try {
                        that.forgedModelAnimation = eval(animation);
                        that.animation = that.forgedModelAnimation;
                        that.forgedModelBasetime = that.startTime;
                    } catch (e) {
                        UserInterface.showMessage('The animation "' + animation + '" is not valid');
                    }
                });
            } else if (index === 3) {
                that.safelyLoadModel(that.forgedModelName);
            }
        };

        this.worldArea.clientClick = function() {
        };

        this.connect('worldAreaActive', function() {
            CAPI.showHUDText('1 - change model, 2 - change animation, 3 - reload model,', 0.5, 0.9, 0.5, 0xFFFFFF);
        });

        this.connect('client_onModify_forgedModelName', function(modelName) {
            this.resetForgedModel();
        });
    },

    clientAct: function(seconds) {
        if (this.forgedModelName === '') {
            Effect.splash(PARTICLE.SPARK, 15, seconds*10, this.getCenter(), 0x55FF90, 1.0, 70, 1);
        }
    },

    renderDynamic: function() {
        if (this.forgedModelChanged) {
            this.safelyLoadModel(this.forgedModelName);
            this.forgedModelChanged = false;
        }
        var o = this.getCenter();
        var flags = MODEL.LIGHT | MODEL.CULL_VFC | MODEL.FULLBRIGHT | MODEL.CULL_DIST | MODEL.DYNSHADOW;
        var basetime = this.forgedModelBasetime;
        var args = [this, this.forgedModelName, this.forgedModelAnimation, o.x, o.y, o.z, 0, 0, flags, basetime];
        CAPI.renderModel.apply(this, args);
    },

    getCenter: function() {
        return this.position.addNew(new Vector3(0, 0, this.collisionRadiusHeight/2));
    },

    safelyLoadModel: function(modelName) {
        var ret = CAPI.reloadModel(this.forgedModelName);
        if (ret && ret.error) {
            UserInterface.showMessage('The model failed to load. See console output.');
            this.forgedModelName = ''; // Prevent fatal error on next loadmodel() during rendering
        }
    },
}]));

