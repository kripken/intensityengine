
//=============================================================================
// Copyright (C) 2008 Alon Zakai ('Kripken') kripkensteiner@gmail.com
//
// This file is part of the Intensity Engine project,
//    http://www.intensityengine.com
//
// The Intensity Engine is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published by
// the Free Software Foundation, version 3.
//
// The Intensity Engine is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with the Intensity Engine.  If not, see
//     http://www.gnu.org/licenses/
//     http://www.gnu.org/licenses/agpl-3.0.html
//=============================================================================


//! Lets you define signals and handlers for areas in the world (as opposed to
//! on a specific entity). Each signal connection specifies the signal type
//! (a string), and the spatial area which is relevant - origin and a radius.
//! (if the origin is null, the entire space is relevant).
//!
//! Emitting is done by specifying a type and a origin. Optionally, you can
//! also specify a radius, which makes handlers notice it even if outside their
//! normal radius (as if the signal were emitted in the entire sphere defined
//! by the origin/radius given for the emitting).
//!
WorldSignals = {
    _handlers: {},

    //! origin of null means that this handler will be sensitive to the entire world
    connect: function(_type, origin, radius, func) {
        var id = 0;
        while (this._handlers[id]) id++;
        this._handlers[id] = {
            _type: _type, origin: origin, radius: radius, func: func,
        };
        return id;
    },

    disconnect: function(id) {
        delete this._handlers[id];
    },

    //! TODO: Use efficient octree etc. data models, for cases with lots of handlers
    emit: function(_type, origin, radius, kwargs) {
        radius = defaultValue(radius, 0); // Can set this higher, to make handlers notice it even if outside their
                                          // normal radius

        forEach(values(this._handlers), function(handler) {
            if (handler._type === _type && (
                    !handler.origin || handler.origin.isCloseTo(origin, handler.radius - radius)
            )) {
                handler.func(origin, kwargs);
            }
        });
    },
};

