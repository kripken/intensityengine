
// Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
// This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

log(DEBUG, "Preliminary tests - __defineGetter__/__defineSetter");

var simple = { "0": "Hello" };
eval(assert(' simple["0"] === "Hello" '));
eval(assert(' simple[0] === "Hello" '));

var complicated = { value: 0 };

complicated.__defineGetter__("test", function() { return this.value; });
complicated.__defineSetter__("test", function(_value) { this.value = _value + 100; });

eval(assert(' complicated.test !== undefined '));
eval(assert(' complicated.test === 0 '));
complicated.test = 721;
eval(assert(' complicated.test === 821 '));
complicated.test = 0;
eval(assert(' complicated.test === 100 '));

eval(assert(' complicated["test"] !== undefined '));
eval(assert(' complicated["test"] === 100 '));
complicated["test"] = 721;
eval(assert(' complicated["test"] === 821 '));
complicated["test"] = 0;
eval(assert(' complicated["test"] === 100 '));

// The following fail on Chrome, issue 242
// http://code.google.com/p/v8/issues/detail?id=242&colspec=ID%20Type%20Status%20Priority%20Owner%20Summary%20Stars
//complicated.__defineGetter__("0", function() { return this.value; });
//complicated.__defineSetter__("0", function(_value) { this.value = _value + 100; });
//
//eval(assert(' complicated["0"] !== undefined '));
//eval(assert(' complicated["0"] === 100 '));
//complicated["0"] = 721;
//eval(assert(' complicated["0"] === 821 '));
//complicated["0"] = 0;
//eval(assert(' complicated["0"] === 100 '));


//

log(DEBUG, "generic array tests");

var dummy = new StateArray();

eval(assert(' arrayEqual(dummy.toWire([]), "[]") '));
eval(assert(' arrayEqual(dummy.toData([]), "[]") '));
eval(assert(' arrayEqual(dummy.fromWire("[]"), []) '));
eval(assert(' arrayEqual(dummy.fromData("[]"), []) '));

var x = ["a", "4"];
eval(assert(' typeof dummy.toWire(x) === "string" '));
eval(assert(' typeof dummy.toData(x) === "string" '));
eval(assert(' arrayEqual(dummy.fromWire(dummy.toWire(x)), x) '));
eval(assert(' arrayEqual(dummy.fromData(dummy.toData(x)), x) '));

log(DEBUG, "Abstract tests:");

var TestVariable = StateVariable.extend({
    toData: function(x) {
        return x;
    },
    fromData: function(x) {
        return x+1;
    }
});

var BaseClass = Class.extend({
    create: function() {
        this.stateVariableValues = {};
    },

    var1: new TestVariable(),
    var2: new TestVariable(),

    int1: new StateInteger(),
    float1: new StateFloat(),
    bool1: new StateBoolean(),
    string1: new StateString(),

    arr1: new StateArray(),
    arr2: new StateArray(),

    farr1: new StateArrayFloat(),

    _getStateDatum : function(key) {
        return this.stateVariableValues[key];
    },

    _setStateDatum : function(key, value, origin) {
        this.stateVariableValues[key] = value;
    },

    canCallCFuncs: function() { return true; }
});

var test = new BaseClass();

test.initialized = true;

eval(assert(" isVariable(test.var1) "));
eval(assert(" isVariable(test.int1) "));
eval(assert(" isVariable(test.float1) "));
eval(assert(" isVariable(test.arr1) "));

// abstract Variable

test.var1._register("var1", test);
test.var2._register("var2", test);

test.var1 = 175;

eval(assert(' test.stateVariableValues["var1"] === 175 '));
eval(assert(" test.var1 === 175 "));

test.var2 = 200;

eval(assert(' test.stateVariableValues["var2"] === 200 '));
eval(assert(" test.var2 === 200 "));

eval(assert(' test.stateVariableValues["var1"] === 175 '));
eval(assert(" test.var1 === 175 "));

// Integer

test.int1._register("int1", test);
test.int1 = 18;
eval(assert(' test.stateVariableValues["int1"] === 18 '));
eval(assert(" test.int1 === 18 "));

// Float

test.float1._register("float1", test);
test.float1 = 183.2;
eval(assert(' test.stateVariableValues["float1"] === 183.2 '));
eval(assert(" test.float1 === 183.2 "));

// Boolean

test.bool1._register("bool1", test);
test.bool1 = true;
eval(assert(' test.stateVariableValues["bool1"] === true '));
eval(assert(' test.bool1 === true '));
test.bool1 = false;
eval(assert(' test.stateVariableValues["bool1"] === false '));
eval(assert(" test.bool1 === false "));

// String

test.string1._register("string1", test);
test.string1 = "wakaw";
eval(assert(' test.stateVariableValues["string1"] === "wakaw" '));
eval(assert(' test.string1 === "wakaw" '));

// Array surrogate

log(DEBUG, "Array surrogate:");

var surrFeedback = {};

var entityWithSurr = {
    uniqueId: "harrowe",
    itemBase: 8
};

var variableWithSurr = {
    getLength: function(entity) {
        return entity.uniqueId;
    },
    getItem: function(entity, i) {
        return entity.itemBase + i;
    },
    setItem: function(entity, i, value) {
        entity.itemBase += i + value;
    }
};

var surr = new ArraySurrogate(entityWithSurr, variableWithSurr);

eval(assert(' surr.length === "harrowe" '));

eval(assert(' surr.get(0) === 8 '));
eval(assert(' surr.get(1) === 9 '));
eval(assert(' surr.get(2) === 10 '));
eval(assert(' surr.get(5) === 13 '));
eval(assert(' surr.get(11) === 19 '));

surr.set(13, 50);

eval(assert(' surr.get(2) === 73 '));
eval(assert(' surr.get(5) === 76 '));

// Array

log(DEBUG, "Array tests:");

test.arr1._register("arr1", test);
test.arr1 = ['a', 'bcd'];
eval(assert(' test[_SV_PREFIX + "arr1"].toData(test.stateVariableValues["arr1"]) === "[a|bcd]" '));
eval(assert(' arrayEqual(test.stateVariableValues["arr1"], ["a", "bcd"]) '));
eval(assert(' arrayEqual(test.arr1.asArray(), ["a", "bcd"]) '));

eval(assert(' test.arr1 instanceof ArraySurrogate '));

//log(DEBUG, "****************************************** We have: ");
//log(DEBUG, "1                                                 : " + test.arr1);
//log(DEBUG, "2                                                 : " + test.arr1[0] + "," + test.arr2[1]);
//log(DEBUG, "3                                                 : " + serializeJSON(test.arr1));

eval(assert(' arrayEqual(test.arr1.asArray(), ["a", "bcd"]) '));

test.arr2._register("arr2", test);
test.arr2 = ['a', 'b', 'cd', 'e'];
eval(assert(' test[_SV_PREFIX + "arr2"].toData(test.stateVariableValues["arr2"]) === "[a|b|cd|e]" '));
eval(assert(' arrayEqual(test.stateVariableValues["arr2"], ["a", "b", "cd", "e"]) '));
eval(assert(' arrayEqual(test.arr2.asArray(), ["a", "b", "cd", "e"]) '));
//test.arr2[2] = 'firgl'; // Chrome issue 242
test.arr2.set(2, 'firgl'); // Chrome issue 242
eval(assert(' test[_SV_PREFIX + "arr2"].toData(test.stateVariableValues["arr2"]) === "[a|b|firgl|e]" '));
eval(assert(' arrayEqual(test.arr2.asArray(), ["a", "b", "firgl", "e"]) '));
test.arr2.push('xor');
eval(assert(' test[_SV_PREFIX + "arr2"].toData(test.stateVariableValues["arr2"]) === "[a|b|firgl|e|xor]" '));
eval(assert(' arrayEqual(test.arr2.asArray(), ["a", "b", "firgl", "e", "xor"]) '));

eval(assert(' test[_SV_PREFIX + "arr1"].toData(test.stateVariableValues["arr1"]) === "[a|bcd]" ')); // Ensure first array untouched
eval(assert(' arrayEqual(test.stateVariableValues["arr1"], ["a", "bcd"]) '));
eval(assert(' arrayEqual(test.arr1.asArray(), ["a", "bcd"]) '));

log(DEBUG, "ArrayFloat tests:");

test.farr1._register("farr1", test);
test.farr1 = [1, 7.56];
// Chrome issue 242 eval(assert(' typeof test.farr1[0] === "number" '));
eval(assert(' typeof test.farr1.get(0) === "number" '));
eval(assert(' test[_SV_PREFIX + "farr1"].toData(test.stateVariableValues["farr1"]) === "[1|7.56]" '));
eval(assert(" arrayEqual(test.farr1.asArray(), [1, 7.56]) "));


// Subclasses, aliases, etc.

log(DEBUG, "Subclass tests:");

var SubClass = BaseClass.extend({
    subint: new StateInteger(),
    subintalias: new VariableAlias("subint"),
    int1alias: new VariableAlias("int1"),
});

var sub = new SubClass();

sub.initialized = true;

sub.int1._register("int1", sub);
sub.subint._register("subint", sub);
sub.subintalias._register("subintalias", sub);
sub.int1alias._register("int1alias", sub);

sub.int1 = 443;
eval(assert(' sub.stateVariableValues["int1"] === 443 '));
eval(assert(" sub.int1 === 443 "));

sub.subint = 1000;
eval(assert(' sub.stateVariableValues["subint"] === 1000 '));
eval(assert(" sub.subint === 1000 "));

eval(assert(" sub.subintalias === 1000 "));
sub.subintalias = 6006; // Change alias, but real one should alter
eval(assert(' sub.stateVariableValues["subint"] === 6006 '));
eval(assert(" sub.subint === 6006 "));

eval(assert(" sub.int1alias === 443 "));
sub.int1alias = 556;
eval(assert(' sub.stateVariableValues["int1"] === 556 '));
eval(assert(" sub.int1 === 556 "));

// Ensure no interference between test and sub

eval(assert(' test.stateVariableValues["int1"] === 18 '));
eval(assert(" test.int1 === 18 "));


log(DEBUG, "Wrapping tests:");

var testGetter = function(entity) {
    return entity.uniqueId + 1
};

var testGetter2 = function(entity) {
    return entity.uniqueId + 2
};

var VALS = {};

var testSetter = function(entity, value) {
    VALS[entity.uniqueId] = value + 1;
};

var testSetter2 = function(entity, value) {
    VALS[entity.uniqueId] = value + 2;
};

var testVecGetter = function(entity) {
    return [entity.uniqueId+1,entity.uniqueId+3,entity.uniqueId+2];
};

var testVecSetter = function(entity, value) {
    log(DEBUG, "testVecSetter:" + entity.uniqueId + "," + value);
    VALS[entity.uniqueId] = value;
};

var WrappedClass = BaseClass.extend({
    wrapint1: new WrappedCInteger({
        cGetter: testGetter, cSetter: testSetter
    }),
    wrapint2: new WrappedCInteger({
        cGetter: testGetter2, cSetter: testSetter2
    }),
    wrapfloat: new WrappedCInteger({
        cGetter: testGetter, cSetter: testSetter
    }),
    wrapvec3: new WrappedCVector3({
        cGetter: testVecGetter, cSetter: testVecSetter
    }),
    wrapvec3b: new WrappedCVector3({
        cGetter: testVecGetter, cSetter: testVecSetter
    }),
});

var wrap = new WrappedClass();

wrap.initialized = true;

wrap.uniqueId = 1330;

Object.addSignalMethods(wrap);

log(DEBUG, "wrapped ints");

wrap.int1._register("int1", wrap);
wrap.int1 = 98;
eval(assert(' wrap.stateVariableValues["int1"] === 98 '));
eval(assert(" wrap.int1 === 98 "));

wrap.wrapint1._register("wrapint1", wrap);
eval(assert(" wrap.wrapint1 === 1331 "));
wrap.wrapint1 = 2112;
eval(assert(' wrap.stateVariableValues["wrapint1"] === 2112 '));
eval(assert(' wrap.wrapint1 === 1331 '));

wrap.wrapint2._register("wrapint2", wrap);
eval(assert(" wrap.wrapint2 === 1332 "));
wrap.wrapint2 = 444;
eval(assert(' wrap.stateVariableValues["wrapint2"] === 444 '));
eval(assert(" wrap.wrapint2 === 1332 "));

log(DEBUG, "wrapped vecs");

wrap.wrapvec3._register("wrapvec3", wrap);
eval(assert(" arrayEqual(wrap.wrapvec3.asArray(), [1331,1333,1332]) "));
wrap.wrapvec3 = [4,1,6];

log(DEBUG, "final variables test: " + wrap[_SV_PREFIX + "wrapvec3"].toData(wrap.stateVariableValues["wrapvec3"]));
eval(assert(' wrap[_SV_PREFIX + "wrapvec3"].toData(wrap.stateVariableValues["wrapvec3"]) === "[4|1|6]" ')); // In this test, only the last is actually altered in the SD

eval(assert(" arrayEqual(wrap.wrapvec3.asArray(), [1331,1333,1332]) "));
eval(assert(" wrap.wrapvec3.x === 1331 "));
eval(assert(" wrap.wrapvec3.y === 1333 "));
eval(assert(" wrap.wrapvec3.z === 1332 "));

log(DEBUG, "Vector3 functions in WrappedCVector3/Vector3Surrogate:");

var w = wrap.wrapvec3;
eval(assert(" w.x === 1331 "));
eval(assert(" w.y === 1333 "));
eval(assert(" w.z === 1332 "));

w = wrap.wrapvec3.copy();
eval(assert(" w.x === 1331 "));
eval(assert(" w.y === 1333 "));
eval(assert(" w.z === 1332 "));
w.x = 17
eval(assert(" w.x === 17 "));
eval(assert(" w.y === 1333 "));
eval(assert(" w.z === 1332 "));

//log(DEBUG, "Assign wrapped vec to another");
//
//VALS[wrap.uniqueId] = [1,3,12];
//wrap.wrapvec3b._register("wrapvec3b", wrap);
//wrap.wrapvec3b = wrap.wrapvec3;
//
//log(DEBUG, "(2)final variables test: " + serializeJSON(VALS[wrap.uniqueId]));
//eval(assert('  arrayEqual(VALS[wrap.uniqueId], [1331,1333,1332]) ')); // Same as above for wrappedvec3

