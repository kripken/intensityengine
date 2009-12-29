Vec3 = function() {
    this.__defineGetter__("0", function() { return this.x; });
    this.__defineGetter__("1", function() { return this.y; });
    this.__defineGetter__("2", function() { return this.z; });

    this.__defineSetter__("0", function(value) { this.x = parseFloat(value); });
    this.__defineSetter__("1", function(value) { this.y = parseFloat(value); });
    this.__defineSetter__("2", function(value) { this.z = parseFloat(value); });

    this.__defineGetter__("length", function() { return 3; }); // Necessary
};

var v = new Vec3();

v[0] = 10;
v[1] = 20;
v[2] = 40;

