
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
    return function() {
        func.apply(self, arguments);
    };
}

