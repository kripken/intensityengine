
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


Map.fogColorOld = Map.fogColor;
Map.fogColor = function(r, g, b) {
    Map.fogColorOld((r<<16) + (g<<8) + b);
}

rayCollisionDistance = function(origin, ray) {
    var rayMag = ray.magnitude();
    if (rayMag === 0) return -1;
    return CAPI.rayPos( origin.x, origin.y, origin.z, ray.x/rayMag, ray.y/rayMag, ray.z/rayMag, rayMag)
}


Global.currTimestamp = currTimestamp;

startFrame = function() {
    currTimestamp += 1; // backwards compatibility
    Global.currTimestamp = currTimestamp;
}


Vector3.prototype.scalarProduct = function(other) {
    return this.x*other.x + this.y*other.y + this.z*other.z;
};

Vector3.prototype.cosineAngleWith = function(other) {
    return this.scalarProduct(other) / (this.magnitude() * other.magnitude());
};

Vector3.prototype.crossProduct = function(other) {
    return new Vector3(
        this.y*other.z - this.z*other.y, this.z*other.x - this.x*other.z, this.x*other.y - this.y*other.x
    );
};

Vector3.prototype.mulPointwise = function(other) {
        this.x *= other.x;
        this.y *= other.y;
        this.z *= other.z;
        return this;
};

Vector3.prototype.mulPointwiseNew = function(other) {
        return new Vector3(this.x * other.x, this.y * other.y, this.z * other.z);
};

//! Projects the vector along a surface (defined by a normal). Returns this, the modified vector.
Vector3.prototype.projectAlongSurface = function(surface) {
    var normalProjection = this.scalarProduct(surface);
    return this.sub(surface.mulNew(normalProjection));
};

//! Given an up vector, uses this as a forward vector to find the yaw, pitch and roll.
//! @param yawHint If the yaw isn't clear enough, we use a yawHint vector
Vector3.prototype.toYawPitchRoll = function(up, yawHint) {
    var left = this.crossProduct(up);
    //log(ERROR, this + '      ,      ' + up + '    :    ' + left);

    var yaw, pitch, roll; // http://planning.cs.uiuc.edu/node103.html

    if (Math.abs(this.z) < 0.975 || !yawHint) {// && Math.abs(this.x) > 0.025) {
//        if (Math.abs(this.x) < 0.01) log(ERROR, "small");
        yaw = Math.atan2(this.y, this.x)*180/Math.PI + 90;
    } else {
        //log(ERROR, "yawhint! " + this + ',' + yawHint);
//        if (Math.abs(yawHint.x) < 0.01) log(ERROR, "yawhint small");
        yaw = Math.atan2(yawHint.y, yawHint.x)*180/Math.PI + 90;
    }

//    if (Math.abs(left.z) < 0.001) log(ERROR, "tiny");

    var pitch = Math.atan2(-this.z, Math.sqrt(up.z*up.z + left.z*left.z))*180/Math.PI;
    var roll = Math.atan2(up.z, left.z)*180/Math.PI - 90;

    //log(ERROR, yaw + ',' + pitch + ',' + roll);
    return { yaw: yaw, pitch: pitch, roll: roll};
};

Vector3.prototype.lerp = function(other, alpha) {
    alpha = clamp(alpha, 0, 1);
    return this.mulNew(alpha).add(other.mulNew(1-alpha));
};

Vector3.prototype.isZero = function() {
    return this.x === 0 && this.y === 0 && this.z === 0;
};

// New variables

//! An array of float values
StateArrayInteger = StateArray.extend({
    _class: "StateArrayInteger",

    toWireItem: string,
    fromWireItem: integer,

    toDataItem: string,
    fromDataItem: integer,
});


// New CAPI stuff

if (!CAPI.renderModelFixed) {
    log(WARNING, "Fixing CAPI.renderModel for version 1 and 2");

    var renderModelOld = CAPI.renderModel;
    var renderModel2Old = CAPI.renderModel2;

    CAPI.renderModel = function() {
        if (arguments[1] === 'areatrigger') return; // Just used for physics, not rendering

        if (!renderModel2Old) {
            // No need to change anything
            renderModelOld.apply(this, arguments);
        } else {
            // CAPI expects new version
            var args = Array.prototype.slice.call(arguments);
            args.splice(8, 0, 0); // Add roll = 0
            renderModel2Old.apply(this, args);
        }
    };

    CAPI.renderModel2 = function() {
        if (arguments[1] === 'areatrigger') return; // Just used for physics, not rendering

        if (!renderModel2Old) {
            // No renderModel2, so downconvert into renderModel
            var args = Array.prototype.slice.call(arguments);
            args.splice(8, 1); // Remove roll
            renderModelOld.apply(this, args);
        } else {
            renderModel2Old.apply(this, arguments);
        }
    };

    CAPI.renderModelFixed = true;
}


Random = {
    randint: function(_min, _max) {
        return Math.floor(Math.random()*(_max-_min+1))+_min;
    },

    uniform: function(_min, _max) {
        return (Math.random()*(_max-_min) + _min);
    },

    normalizedVector3: function() {
        var ret = null;
        while (ret === null || ret.magnitude() === 0) {
            ret = new Vector3(Random.uniform(-1, 1), Random.uniform(-1, 1), Random.uniform(-1, 1));
        }
        return ret.normalize();
    },

    choice: function(stuff) {
        if (!stuff || !(stuff.length > 0)) return undefined;

        return stuff[Random.randint(0, stuff.length-1)];
    },
};


Tools.replaceFunction('CAPI.forceCamera', function(x, y, z, yaw, pitch, roll, fov) {
    arguments.callee._super(x, y, z, yaw, pitch, roll, defaultValue(fov, -1));
}, false);

UserInterface.forceCamera = function(position, yaw, pitch, roll, fov) {
    CAPI.forceCamera(position.x, position.y, position.z, yaw, pitch, roll, defaultValue(fov, -1));
};

