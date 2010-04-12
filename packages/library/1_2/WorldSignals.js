
// Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
// This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

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

