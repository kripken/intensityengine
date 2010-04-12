
// Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
// This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

Tools = {
    //! Useful tool to replace globals. For example:
    //!     Tools.replaceFunction('Map.texture', function() { ... },);
    replaceFunction: function(originalName, replacement, callOriginal, _this) {
        callOriginal = defaultValue(callOriginal, true);
        replacement._super = eval(originalName);
        function wrapper() {
            _this = defaultValue(_this, this); // Use global if not otherwise specified
            if (callOriginal) {
                replacement._super.apply(_this, arguments);
            }
            replacement.apply(_this, arguments);
        }
        eval(originalName + ' = wrapper');
    },

    callbacks: {
        header: 'scriptcallback=',

        map: [],

        add: function(callback) {
            for (var i = 0; i < this.map.length+3; i++) {
                if (this.map[i] === undefined) {
                    this.map[i] = callback;
                    return this.header + i;
                }
            }
            return null;
        },

        get: function(text, remove) {
            if (text.substring(0, this.header.length) !== this.header) return null;
            var i = text.substring(this.header.length, text.length);

            remove = defaultValue(remove, true);

            var ret = this.map[i];
            if (remove) delete this.map[i];
            return ret;
        },

        tryCall: function(text, param, remove) {
            param = defaultValue(param, '');
            remove = defaultValue(remove, true);

            var callback = this.get(text);
            if (callback !== null) callback(param);
        },
    },
};

function cleanFunctionName(func) {
    var ret = func.im_func ? func.im_func : func;
    return ret.toString().replace(/\n/g, ' ').substring(0, 70);
}

function lerp(value, otherValue, alpha) {
    alpha = clamp(alpha, 0, 1);
    return alpha*value + (1-alpha)*otherValue;
}

function magnet(value, otherValue, radius) {
    return Math.abs(value - otherValue) <= radius ? otherValue : value;
}

function permanentBind(func, self) {
    if (!(typeof func === 'function')) return func;
    return function() {
        func.apply(self, arguments);
    };
}

function callAll() {
    var targets = arguments;
    return function() {
        for (var i = 0; i < targets.length; i++)
            targets[i].apply(this, arguments);
    };
}

function replaceNaN(value, fallback) {
    return !isNaN(value) ? value : fallback;
}

//! Returns the sum of absolute differences, elementwise
function arrayL1Diff(a, b) {
    var ret = 0;
    for (var i = 0; i < a.length; i++) {
        ret += Math.abs(a[i]-b[i]);
    }
    return ret;
}

