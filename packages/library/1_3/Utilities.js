
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


// Button constants. Must be exactly the same as in SDL.
BUTTON_LEFT = 1;
BUTTON_MIDDLE = 2;
BUTTON_RIGHT = 3;


//! A simple timer
RepeatingTimer = Class.extend({
    create: function(interval, carryover) {
        this.interval = interval;
        this.carryover = defaultValue(carryover, false);

        this.sum = 0;
    },

    //! Ticks an amount of time. Returns true if the timer has reached
    //! the specified interval ('fire') of time since starting, and resets it.
    //! If 'carryover', then time over the interval is left for the next time.
    tick: function(seconds) {
        this.sum += seconds;

        if (this.sum >= this.interval) {
            if (!this.carryover) {
                this.sum = 0;
            } else {
                this.sum -= this.interval;
            }
            return true;
        } else {
            return false;
        }
    },

    //! Set the time to fire on the next call to 'tick', no matter how many
    //! seconds are given
    prime: function() {
        this.sum = this.interval;
    }
});


function clamp(value, low, high) {
    return Math.max(low, Math.min(value, high));
}


//! A simple 3-component vector of floats, that is, something like (x,y,z).
//! TODO: Consider using Sylvester,
//!     http://sylvester.jcoglan.com/
//! a JavaScript library for Vector and Matrix math.
Vector3 = Class.extend({
    _class: 'Vector3',

    create: function(x, y, z) {
        if (typeof x === 'object' && x.length === 3) {
            // We have been given an array
            this.x = x[0];
            this.y = x[1];
            this.z = x[2];
        } else {
            // We have been given three floats

            x = defaultValue(x, 0.0);
            y = defaultValue(y, 0.0);
            z = defaultValue(z, 0.0);

            //! The x component of this vector: Right along the screen
            this.x = parseFloat(x);

            //! The y component of this vector: Straight out of the screen
            this.y = parseFloat(y);

            //! The z component of this vector: Up along the screen
            this.z = parseFloat(z);
        }
/* Causes crash in V8
        this.__defineGetter__("0", function() { return this.x; });
        this.__defineGetter__("1", function() { return this.y; });
        this.__defineGetter__("2", function() { return this.z; });

        this.__defineSetter__("0", function(value) { this.x = parseFloat(value); });
        this.__defineSetter__("1", function(value) { this.y = parseFloat(value); });
        this.__defineSetter__("2", function(value) { this.z = parseFloat(value); });
*/

        this.__defineGetter__("length", function() { return 3; });
    },

    //! The magnitude of the vector: square root of x^2+y^2+z^2.
    magnitude: function() {
        return Math.sqrt( this.x*this.x + this.y*this.y + this.z*this.z );
    },

    //! Normalizes the vector so it has magnitude 1.
    normalize: function() {
        var mag = this.magnitude();
        if (mag !== 0.0) {
            this.x /= mag;
            this.y /= mag;
            this.z /= mag;
        } else {
            log(WARNING, "Trying to normalize a length 0 Vector3");
//            eval(assert(' false ')); // Uncomment this to help debug these messages
        }

        return this;
    },

    subNew: function(other) {
        return new Vector3(this.x - other.x, this.y - other.y, this.z - other.z);
    },

    addNew: function(other) {
        return new Vector3(this.x + other.x, this.y + other.y, this.z + other.z);
    },

    mulNew: function(other) {
        return new Vector3(this.x * other, this.y * other, this.z * other);
    },

    sub: function(other) {
        this.x -= other.x;
        this.y -= other.y;
        this.z -= other.z;
        return this;
    },

    add: function(other) {
        this.x += other.x;
        this.y += other.y;
        this.z += other.z;
        return this;
    },

    mul: function(other) {
        this.x *= other;
        this.y *= other;
        this.z *= other;
        return this;
    },

    copy: function() {
        return new Vector3(this.x, this.y, this.z);
    },

    asArray: function() {
        return [ this.x, this.y, this.z ];
    },

    fromYawPitch: function(yaw, pitch) {
        var TO_RAD = Math.PI / 180.0;

        this.x = Math.sin(TO_RAD*yaw);
        this.y = -Math.cos(TO_RAD*yaw);

        if(pitch !== 0) {
            this.x *= Math.cos(TO_RAD*pitch);
            this.y *= Math.cos(TO_RAD*pitch);
            this.z = Math.sin(TO_RAD*pitch);
        }
        else this.z = 0;

        return this;
    },

    toYawPitch: function() {
        var size = this.magnitude();
        if (size < 0.001) return {
            yaw: 0, pitch: 0,
        };

        var RAD = (Math.PI/180.0);
        return {
            yaw: -Math.atan2(this.x, this.y)/RAD + 180.0,
            pitch: Math.asin(this.z/size)/RAD,
        };
    },

    //! Optimized way to check if two positions are close. This is
    //! faster than this.sub(other).magnitude() <= dist (and
    //! certainly faster than subNew instead of sub).
    //! It avoid the square root, and may save some of the multiplications.
    isCloseTo: function(other, dist) {
        dist = dist*dist;

        var temp, sum;

        // Note order: We expect z to be less important, as most maps are 'flat'
        temp = (this.x-other.x);
        sum = temp*temp;
        if (sum > dist) return false;

        temp = (this.y-other.y);
        sum += temp*temp;
        if (sum > dist) return false;

        temp = (this.z-other.z);
        sum += temp*temp;
        return (sum <= dist);
    },

    toString: function() {
        return '<' + this.x.toString() + ',' + this.y.toString() + ',' + this.z.toString() + '>';
    },

    scalarProduct: function(other) {
        return this.x*other.x + this.y*other.y + this.z*other.z;
    },

    cosineAngleWith: function(other) {
        return this.scalarProduct(other) / (this.magnitude() * other.magnitude());
    },
});

Vector4 = Vector3.extend({
    create: function(x, y, z, w) {
        if (x.length === 4) {
            this.x = x[0]; this.y = x[1]; this.z = x[2]; this.w = x[3];
        } else {
            this.x = x; this.y = y; this.z = z; this.w = w;
        }
    },

    toYawPitchRoll: function() {
        var ret = this.toYawPitch();
        ret.roll = 0;
        return ret;

        if (Math.abs(this.z) < 0.99) {
            var ret = this.toYawPitch();
            ret.roll = this.w/(Math.PI/180.0);
            return ret;
        } else {
            return {
                yaw: this.w/(Math.PI/180.0) * (this.z < 0 ? 1 : -1),
                pitch: this.z > 0 ? -90 : 90,
                roll: 0,
            };
        }
    },
});

/*
//! Internal function to simplify the API. Many API functions expect a Vector3, but
//! we allow developers to pass lists or tuples as well. We convert them to a Vector3
//! using this function
function vector3ify(something) {
    if (something instanceof Vector3) {
        return something;
    }

    // Must be an array of sorts
    return new Vector3(something[0], something[1], something[2]);
}


//! Internal function to simplify the API. Many API functions expect a 'worldobject', typically a LogicEntity,
//! to be received, which then has a position, radius, etc., as attributes. For example, a target towards which
//! we face. But, we might want to pass just a Vector3, to face towards that, and it isn't an 'actor'. This
//! function solves that.
//!
//! Worldobjects also support activated versus deactivated states. If you try to worldobjectify a deactivated
//! entity, you will get None. This is a valid way to check for activation status.
//!
//! @param something Either a LogicEntity (which is *assumed* to have a position, radius, etc., as attributes), or
//! something (like a Vector3) that has x,y,z attributes. In the latter case, we wrap those x,y,z in a position
//! inside a class, and return that, so it appears like an 'actor'. Something can also be a list or tuple of
//! (position, radius). It can also be a simple list or tuple of three floats, which are assumed to be a position.
//! @return An 'actor', a class containing a position, radius, etc.
function worldobjectify(something) {
    if (something.position !== undefined) {
        if (something.deactivated !== undefined && something.deactivated) {
            return None;
        } else {
            return something;
        }
    }

    var radius = 0; // Default
    var position;

    if (something.length == 2) {
        position = something[0];

        eval(assert(' position.x !== undefined '));
        eval(assert(' position.y !== undefined '));
        eval(assert(' position.z !== undefined '));

        radius = something[1];
        eval(assert(' typeof radius === "number" '));
    } else if (something.length == 3) {
        eval(assert(' typeof something[0] === "number" '));
        eval(assert(' typeof something[1] === "number" '));
        eval(assert(' typeof something[2] === "number" '));

        position = new Vector3(something[0], something[1], something[2]);
    } else if (something instanceof Vector3 || (something.x !== undefined && something.y !== undefined && something.z !== undefined)) {
        position = something;
    } else {
        log(WARNING, format("Cannot worldobjectify {0}", something));
//
//        if hasattr(something, 'deactivated') and something.deactivated:
//            log(logging.WARNING, "Reason: Trying to access an entity that has been deactivated")
//            return None
//        else:
//            log(logging.ERROR, "Reason: Unknown")
//            raise IntensityError("Invalid WorldObjectification")
//
    }

    return new WorldObject(position, radius);
}


//! Related to the worldobjectify() function, this is a manual way to make a 'world object', i.e., something
//! with attributes of position, radius, deactivated or not, etc.
WorldObject = Class.extend({
    create: function(position, radius) {
        radius = defaultValue(radius, 0);
        this.position = position;
        this.radius = radius;
        this.deactivated = false;
    }
});
*/


//! Normalizes an angle to be within +-180 degrees of some value. Useful to know if we need to turn left or
//! right in order to be closer to something (we just need to check the sign, after normalizing relative
//! to that angle). See example uses in steering.py.
//! @param angle The original angle, e.g., 100
//! @param relativeTo The angle which we will normalize relative to, e.g., 300
//! @return The normalized angle, in the example of 100 relative to 300, we would return 460 (as 460 is 
//! within 180 degrees of 300, but 100 isn't).
function normalizeAngle(angle, relativeTo) {
    while (angle < relativeTo - 180.0) {
        angle += 360.0;
    }
    while (angle > relativeTo + 180.0) {
        angle -= 360.0;
    }
    return angle;
}


//! Returns the direction of change of an angle
function angleDirectionChange(angle, other) {
    angle = normalizeAngle(angle, other);
    return sign(angle - other);
}


//! Calculate the yaw from an origin to a target. Done on 2D data, (x,y) only.
//! @param origin The position from which we start.
//! @param target The target toward which we calculate the yaw.
//! @param reverse Whether to calculate the yaw *away* from the target (default: no).
//! @return The yaw.
function yawTo(origin, target, _reverse) {
    _reverse = defaultValue(_reverse, false);

    if (!_reverse) {
        return (-Math.atan2(target.x - origin.x, target.y - origin.y)/(Math.PI/180.0)) + 180.0;
    } else {
        return yawTo(target, origin); // Slow, TODO: write this explicitly
    }
}


//! Calculate the pitch from an origin to a target. Done on 2D data, (y,z) only.
//! @param origin The position from which we start.
//! @param target The target toward which we calculate the pitch.
//! @param reverse Whether to calculate the pitch *away* from the target (default: no).
//! @return The pitch.
function pitchTo(origin, target, _reverse) {
    _reverse = defaultValue(_reverse, false);

    if (!_reverse) {
        radians = Math.asin( (target.z - origin.z) / distance(origin, target) );
        return 360.0*radians/(2.0*Math.PI);
    } else {
        return pitchTo(target, origin); // Slow, TODO: write this explicitly
    }
}


//! Check if the yaw between two points is within a certain error range. Useful to see if
//! a character is facing closely enough to a target, for example
//! @param origin The origin, from where we calculate the yaw
//! @param target Where we calculate the yaw to
//! @param currentYaw The current yaw of the origin, which we ask is close to the actual yaw
//! @param acceptableError How close the yaws must be for us to return True
//! @return Whether the yaw difference is within the range of acceptable error
function compareYaw(origin, target, currentYaw, acceptableError) {
    var targetYaw = yawTo(origin, target);
    targetYaw = normalizeAngle(targetYaw, currentYaw);
    return ( Math.abs(targetYaw - currentYaw) <= acceptableError );
}

function comparePitch(origin, target, currentPitch, acceptableError) {
    var targetPitch = pitchTo(origin, target);
    targetPitch = normalizeAngle(targetPitch, currentPitch);
    return ( Math.abs(targetPitch - currentPitch) <= acceptableError );
}


//! Sign (-1, 0, or +1) of a value.
function sign(x) {
    if (x < 0) {
        return -1;
    } else if (x > 0) {
        return +1;
    } else {
        return 0;
    }
}


//! The distance between two positions.
//! @param a One position.
//! @param b Another position.
//! @return The distance between the two positions.
function distance(a, b) {
    return Math.sqrt( Math.pow(a.x-b.x, 2) + Math.pow(a.y-b.y, 2) + Math.pow(a.z-b.z, 2) );
}


//! Checks for line of sight between two positions, i.e., if there is a clear line between them
//! with no obstructions. This is a symmetrical operation. (Ignores entities?)
//!
//! @param a One position.
//! @param b Another position.
//! @return True if there is a clear line of sight between the positions.
function hasLineOfSight(a, b) {
    return CAPI.rayLos( a.x, a.y, a.z, b.x, b.y, b.z );
}


//! Check for collisions of a ray against world geometry - ignores entities.
//! @param origin Where the ray starts from.
//! @param ray We look for collisions along this ray. The length of the ray implies how far ahead to look. XXX - seems we look farther
//! @return The distance along the ray to the first collision.
function rayCollisionDistance(origin, ray) {
    var rayMag = ray.magnitude();
    return CAPI.rayPos( origin.x, origin.y, origin.z, ray.x/rayMag, ray.y/rayMag, ray.z/rayMag, rayMag)
}


//! Finds the floor below some position
//! @param origin The position to start looking from.
//! @param distance How far to look for the floor before giving up.
//! @return The distance to the floor.
function floorDistance(origin, distance) {
    return CAPI.rayFloor( origin.x, origin.y, origin.z, distance);
}


//! Finds the distance to the *highest* floor, not just of a point but below an area of a certain radius. This is suitable
//! for finding the floor beneath a character, for example, as a single point calculation does not suffice. By 'highest'
//! floor we mean the highest floor, i.e., the smallest distance from the origin to that floor.
//! @param origin The position to start lookin from.
//! @param distance How far to look for the floor before giving up.
//! @param radius The radius around the origin around which to look. Rays are cast downwards up to this radius
//! @return The distance to the floor.
function highestFloorDistance(origin, distance, radius) {
    var ret = floorDistance(origin, distance);

    forEach([ -radius/2, 0, +radius/2 ], function(x) {
        forEach([ -radius/2, 0, +radius/2 ], function(y) {
            ret = Math.min(ret, floorDistance(origin + new Vector3(x, y, 0), distance));
        });
    });

    return ret;
}


//! Finds the distance to the *lowest* floor, not just of a point but below an area of a certain radius. This is suitable
//! for finding if there are *any* 'deep spots' around an area, even if it is still solid enough to stand on. By 'lowest'
//! floor we mean the lowest floor, i.e., biggest distance from the origin to that floor.
//! @param origin The position to start lookin from.
//! @param distance How far to look for the floor before giving up.
//! @param radius The radius around the origin around which to look. Rays are cast downwards up to this radius
//! @return The distance to the floor.
function lowestFloorDistance(origin, distance, radius) {
    var ret = floorDistance(origin, distance);

    forEach([ -radius/2, 0, +radius/2 ], function(x) {
        forEach([ -radius/2, 0, +radius/2 ], function(y) {
            ret = Math.max(ret, floorDistance(origin + new Vector3(x, y, 0), distance));
        });
    });

    return ret;
}

//! Parse a number representing three packed 0-255 values. E.g. parseABC(0xFF0022) returns [255, 0, 34]. Useful for RGB values.
function parseABC(ABC) {
    var ret = [0, 0, ABC & 255];
    ABC = ABC >> 8;
    ret[1] = ABC & 255;
    ret[0] = ABC >> 8;
    return ret;
}

