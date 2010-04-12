
// Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
// This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

var __MODULAR_PREFIX = "__MODULAR_";

//! Given a normal class and a list of plugins, build a final class from
//! them.
//! This is done by letting plugins plug into the init,
//! activate, clientActivate, deactivate, clientDeactivate, act,
//! clientAct and renderDynamic functions.
//! TODO: Message passing system
bakePlugins = function(_parent, plugins) {
    var _class = _parent.extend({});

    var slots = ['init', 'activate', 'clientActivate', 'deactivate', 'clientDeactivate', 'act', 'clientAct', 'renderDynamic' ];

    forEach(slots, function(slot) {
        var old = _class.prototype[slot];

        _class.prototype[slot] = function() {
            var callees = this[__MODULAR_PREFIX + _parent.prototype._class + slot];
            for (var i = 0; i < callees.length; i++) {
                callees[i].apply(this, arguments);
            }
        };

        eval(assert(' _class.prototype[__MODULAR_PREFIX + _parent.prototype._class + slot] === undefined '));

        var callees = [];
        _class.prototype[__MODULAR_PREFIX + _parent.prototype._class + slot] = callees;
        if (old !== undefined && old !== null) {
            callees.push(old);
        }

        forEach(plugins, function(plugin) {
            if (plugin[slot] !== undefined) {
                callees.push( plugin[slot] );
            }
        });
    });

    // Add non-slot functions and items. No check is made for overridding and so forth.
    forEach(plugins, function(plugin) {
        for (item in plugin) {
            if (typeof plugin[item] !== 'function' || findValue(slots, item) === -1) {
                _class.prototype[item] = plugin[item];
            }
        }
    });

    return _class;
};


//
// Tests: TODO: Move to _tests file?
//

(function() {
    var output = [];

    var TestClass = Class.extend({
        init: function() { output.push( ['init', Array.prototype.slice.call(arguments)] ); },
        activate: function() { output.push( ['activate', Array.prototype.slice.call(arguments)] ); },
        clientActivate: function() { output.push( ['clientActivate', Array.prototype.slice.call(arguments)] ); },
        deactivate: function() { output.push( ['deactivate', Array.prototype.slice.call(arguments)] ); },
        clientDeactivate: function() { output.push( ['clientDeactivate', Array.prototype.slice.call(arguments)] ); },
        act: function() { output.push( ['act', Array.prototype.slice.call(arguments)] ); },
        clientAct: function() { output.push( ['clientAct', Array.prototype.slice.call(arguments)] ); },
    });

    var TestPlugin1 = {
        init: function() { output.push( ['Plugin1 init', Array.prototype.slice.call(arguments)] ); },
        activate: function() { output.push( ['Plugin1 activate', Array.prototype.slice.call(arguments)] ); },
        clientActivate: function() { output.push( ['Plugin1 clientActivate', Array.prototype.slice.call(arguments)] ); },
        deactivate: function() { output.push( ['Plugin1 deactivate', Array.prototype.slice.call(arguments)] ); },

        otherFunc: function() { return 172; }
    };

    var TestPlugin2 = {
        deactivate: function() { output.push( ['Plugin2 deactivate', Array.prototype.slice.call(arguments)] ); },
        clientDeactivate: function() { output.push( ['Plugin2 clientDeactivate', Array.prototype.slice.call(arguments)] ); },
        act: function() { output.push( ['Plugin2 act', Array.prototype.slice.call(arguments)] ); },
        clientAct: function() { output.push( ['Plugin2 clientAct', Array.prototype.slice.call(arguments)] ); },

        otherValue: 'cheezypoof'
    };

    var Baked = bakePlugins(TestClass, [TestPlugin1, TestPlugin2]);
    var baked = new Baked();

    eval(assert(' TestClass.prototype.otherFunc === undefined ')); // We should not affect the parent class
    eval(assert(' TestClass.prototype.otherValue === undefined '));

    eval(assert(' baked.otherFunc() === 172 '));
    eval(assert(' baked.otherValue === "cheezypoof" '));

    output = [];
    baked.init('alpha', 9);
    eval(assert(' arrayEqual(output, [["init", ["alpha", 9]], ["Plugin1 init", ["alpha", 9]]]) '));

    output = [];
    baked.activate('alpha', 9);
    eval(assert(' arrayEqual(output, [["activate", ["alpha", 9]], ["Plugin1 activate", ["alpha", 9]]]) '));

    output = [];
    baked.clientActivate('alpha', 9);
    eval(assert(' arrayEqual(output, [["clientActivate", ["alpha", 9]], ["Plugin1 clientActivate", ["alpha", 9]]]) '));

    output = [];
    baked.deactivate('alpha', 9);
    eval(assert(' arrayEqual(output, [["deactivate", ["alpha", 9]], ["Plugin1 deactivate", ["alpha", 9]], ["Plugin2 deactivate", ["alpha", 9]]]) '));

    output = [];
    baked.clientDeactivate('alpha', 9);
    eval(assert(' arrayEqual(output, [["clientDeactivate", ["alpha", 9]], ["Plugin2 clientDeactivate", ["alpha", 9]]]) '));

    output = [];
    baked.act('alpha', 9);
    eval(assert(' arrayEqual(output, [["act", ["alpha", 9]], ["Plugin2 act", ["alpha", 9]]]) '));

    output = [];
    baked.clientAct('alpha', 9);
    eval(assert(' arrayEqual(output, [["clientAct", ["alpha", 9]], ["Plugin2 clientAct", ["alpha", 9]]]) '));
})();

