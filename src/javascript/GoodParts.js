
// Some useful code due to the excellent book "JavaScript: The Good Parts" by
// Douglas Crockford.
// http://oreilly.com/catalog/9780596517748/

// Lets us forget we are in a prototype-based language.
// You can now add methods without using the term 'prototype'.
Function.prototype.method = function (name, func) {
    this.prototype[name] = func;
    return this; // allow cascade ('builder pattern')
};

// Check for numberness, rejecting NaN, Infinite, and all other non-normal-numbers.
function isNumber(value) { return typeof value === 'number' &&
            isFinite(value);
}

// Check if something is an array
var is_array = function (value) {
    return value &&
        typeof value === 'object' &&
        typeof value.length === 'number' &&
        typeof value.splice === 'function' &&
        !(value.propertyIsEnumerable('length'));
};

Number.method('integer', function ( ) {
    return Math[this < 0 ? 'ceiling' : 'floor'](this);
});

// Allows currying - creating new functions with 'bound'
// parameters (at the start).
Function.method('curry', function ( ) {
    var slice = Array.prototype.slice,
        args = slice.apply(arguments),
        that = this;
    return function ( ) {
        return that.apply(null, args.concat(slice.apply(arguments)));
    };
});

