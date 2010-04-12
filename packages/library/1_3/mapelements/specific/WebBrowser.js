
// Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
// This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.


// TODO: Separate windows, etc.

WebBrowser = registerEntityClass(bakePlugins(Mapmodel, [{
    _class: 'WebBrowser',
    shouldAct: true,

    webBrowserURL: new StateString(),

    init: function() {
        this.modelName = ''; // Each client sets their own values
        this.webBrowserURL = 'http://www.google.com/ncr';
    },

    clientActivate: function() {
        this.slot = -1;
        this.visualModel = 'videoscreen/1024';
        this.connect('client_onModify_webBrowserURL', function(value) {
            this.refreshComponent(value);
        });
        this.connect('client_onModify_modelName', function(value) {
            this.eraseModelName = true;
        });
    },

    clientDeactivate: function() {
        this.refreshComponent();
    },

    getWebBrowserTexture: function() {
        if (!this.visualModel || !this.slot) return undefined;
        return 'packages/models/' + this.visualModel + '/' + this.slot + '/blank.png';
    },

    refreshComponent: function(url) {
        if (!this.webBrowserURL) return;

        if (this.slot >= 0) {
            WebBrowsers.freeSlot(this.slot);
            CAPI.signalComponent('WebBrowser', 'delete|' + this.getWebBrowserTexture());
        }
        if (url) {
            this.slot = WebBrowsers.getSlot(this);
            if (this.slot >= 0) {
                CAPI.signalComponent('WebBrowser', 'new|' + this.getWebBrowserTexture() + '|' + url);
                this.setLocalModelName(this.visualModel + '/' + this.slot);
            } else {
                log(ERROR, "cannot get free web browser slot");
            }
        }
    },
/*
    renderDynamic: function() {
        if (!this.getWebBrowserTexture()) return;

        var o = this.position;
        var flags = MODEL.LIGHT | MODEL.CULL_VFC | MODEL.FULLBRIGHT | MODEL.CULL_DIST | MODEL.DYNSHADOW;
        var yaw = this.yaw;
        var pitch = 0;
        var model = this.visualModel + '/' + this.slot;
        var args = [this, model, ANIM_IDLE|ANIM_LOOP, o.x, o.y, o.z, yaw, pitch, flags, 0];
        CAPI.renderModel.apply(this, args);
    },
*/

    clientAct: function() {
        if (this.eraseModelName) {
            // Undo a server update - we do this ourselves locally
            this.setLocalModelName('');
            this.eraseModelName = false;
        }

        if (this.webBrowserURL && (!this.modelName || this.modelName === 'blank')) {
            this.refreshComponent(this.webBrowserURL);
        }
    },

    clientClick: function(button, down, position) {
        if (down) Sound.play('olpc/AdamKeshen/BeatBoxCHIK.wav', position);

        var normal = new Vector3().fromYawPitch(this.yaw+90, 0);

        for (var i = -4; i <= 4; i++) {
            Effect.splash(PARTICLE.SMOKE, 2, 0.3, position.addNew(normal.mulNew(i)), 0x101010, 2, 1, -20);
        }
        Effect.flare(PARTICLE.STREAK, getPlayerEntity().center, position, 0.2, 0xFFAA00, 1.0);
//        Effect.splash(PARTICLE.SPARK, 55, 0.5, position, 0xEECC33, 1.0, 70, 1);
//        Effect.fireball(PARTICLE.EXPLOSION, position, 10, 0.4, 0xDD8844);

        var w = 66.666, h = 50;
        var along = position.subNew(this.position).projectAlongSurface(normal);
        var y = along.z;
        var direction = new Vector3().fromYawPitch(this.yaw, 0);
        var x = Math.sqrt(along.magnitude()*along.magnitude()-y*y) * sign(normal.scalarProduct(getPlayerEntity().center.subNew(position))) * sign(along.scalarProduct(direction));
        x = clamp((x + w/2)/w, 0, 1);
        y = clamp(1 - y/h, 0, 1);
        if (button === 1) {
            CAPI.signalComponent('WebBrowser', 'click|' + this.getWebBrowserTexture() + '|' + x + '|' + y + '|' + button + '|' + down);
        } else {
            // Get keyboard input
            var that = this;
            UserInterface.showInputDialog('Enter your text:', function(text) {
//                // Click there, so can enter text
//                CAPI.signalComponent('WebBrowser', 'click|' + that.getWebBrowserTexture() + '|' + x + '|' + y + '|1|1');
//                CAPI.signalComponent('WebBrowser', 'click|' + that.getWebBrowserTexture() + '|' + x + '|' + y + '|1|0');

                // Inject keypress and release events
                for (var j = 0; j < text.length; j++) {
                    var key = text[j];
                    CAPI.signalComponent('WebBrowser', 'keypress|' + that.getWebBrowserTexture() + '|' + key + '|1');
                    CAPI.signalComponent('WebBrowser', 'keypress|' + that.getWebBrowserTexture() + '|' + key + '|0');
                }
            });
        }

        return true;
    },
}]));

WebBrowsers = {
    slots: {}, // slot->entity.

    getSlot: function(entity) {
        var possibles = filter(function(slot) { return !WebBrowsers.slots[slot]; }, keys(WebBrowsers.slots));
        if (possibles.length === 0) return -1;
        var slot = possibles[0];
        WebBrowsers.slots[slot] = entity;
        return slot;
    },

    freeSlot: function(slot) {
        WebBrowsers.slots[slot] = null;
    }
};

forEach([0, 1, 2, 3, 4, 5, 6, 7, 8, 9], function(slot) {
    WebBrowsers.freeSlot(slot);
});

// Setup

if (!CAPI.signalComponent) {
    CAPI.signalComponent = function() {
        log(ERROR, "CAPI.signalComponent is not present in this build");
    }
}

