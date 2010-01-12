
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

    extraPlugins: {
        skypeManager: {
            skypeHandle: new StateString(),

            init: function() {
                this.skypeHandle = '';
            },

            clientActivate: function() {
                log(ERROR, "clientactivate");
                this.skypeAttempts = 0;
                // Keep asking for handle until we get it
                GameManager.getSingleton().eventManager.add({
                    secondsBefore: 0,
                    secondsBetween: 2,
                    func: bind(function() {
log(ERROR, "need handle?");
                        if (this.skypeHandle) return false; // stop now
                        this.skypeAttempts += 1;
                        if (this.skypeAttempts === 10) return false; // give up

log(ERROR, "need handle!!!1");
                        CAPI.signalComponent('Skype', 'whoami|' + Tools.callbacks.add(bind(function(handle) {
                            this.skypeHandle = handle;
                        }, this)));
                    }, this),
                    entity: this,
                });
            },
        },
    },
};

