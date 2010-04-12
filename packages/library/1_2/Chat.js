
// Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
// This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

Chat = {
    managerPlugin: {
        clientActivate: function() {
        },

        addChat: function(senderId, targetId, text) {
        },
    },

    playerPlugin: {
//        chatTarget: new StateInteger(),
        chatMessage: new StateArray({ hasHistory: false }),

        activate: function() {
            this.chatTarget = -1;
        },

        clientActivate: function() {
            this.connect('client_onModify_chatMessage', function(msg) {
                if (this === getPlayerEntity()) return;
                if (msg.length !== 2) return;
                var target = msg[0];
                var text = msg[1];
                this.currChat = {
                    timer: new RepeatingTimer(5.0),
                    text: text,
                };
//TODO                if (target !== -1 && target !== getPlayerEntity().uniqueId) return;
//TODO                GameManager.getSingleton().addChat(this.uniqueId, target, text);
            });
        },

        clientAct: function(seconds) {
            if (this !== getPlayerEntity()) {
                var text = this._name;
                if (this.currChat && !this.currChat.timer.tick(seconds)) {
                    text += ' : ' + this.currChat.text;
                    if (text.length >= 25) {
                        text = text.substring(0, 21) + '[..]';
                    }
                } else {
                    this.currChat = null;
                }
                Effect.text(this.position.addNew(new Vector3(0, 0, this.eyeHeight * 1.5)), text, 0.0, 0xEEEEFF, 4.0, 0);
            } else {
/*
                // Who we are chatting with
                var text = 'chatting with: <Everybody>';
                var color = 0x7799DE;
                if (this.chatTarget !== -1) {
                    var entity = getEntity(this.chatTarget);
                    if (entity) {
                        text = 'chatting with: ' + entity._name;
                        color = 0x77DE99;
                    } else {
                        this.chatTarget = -1;
                    }
                }

                if (Global.fontHeight) {
                    var factorX = Global.fontHeight/Global.screenWidth;
                    var factorY = Global.fontHeight/Global.screenHeight;
                    CAPI.showHUDRect(0.5, 0.95, -0.666*factorX*text.length/2, -factorY/2, 0x202020)
                }

                CAPI.showHUDText(text, 0.5, 0.95, 0.4, color);
*/
            }
        },

        clientClick: function(button) {
            getPlayerEntity().chatTarget = this.uniqueId;
        },
    },

    //! Can call this e.g. when players click on empty areas, or press a particular action key
    cancelChatTarget: function() {
        getPlayerEntity().chatTarget = -1;
    },

    handleTextMessage: function(sender, text) {
        var senderEntity = getEntity(sender);
        if (!senderEntity) return false;
        var target = senderEntity.chatTarget;
        var targetEntity = null;
        if (target !== -1) {
            targetEntity = getEntity(target);
            if (!targetEntity) return false;
        }
        senderEntity.chatMessage = [target, text];
        return false;
    },
};

