
// Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
// This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

Testing.replaceEnvironment();

eval(assert(' arrayEqual(keys(_logicEntityClasses), []) '));

var tc1 = LogicEntity.extend({
    _class: "tc1",
    var1a: new StateVariable(),
    var1b: new StateVariable()
});

var tc2 = LogicEntity.extend({
    _class: "tc2"
});

log(DEBUG, "tc1");
registerEntityClass(tc1, "nada");

eval(assert(' getEntityClass("tc1") === tc1 '));
eval(assert(' getEntitySauerType("tc1") === "nada" '));
assertException(' getEntityClass("tc2") ');

log(DEBUG, "MI:" + MessageSystem.feedback);
eval(assert(' serializeJSON(MessageSystem.feedback) === serializeJSON([["tc1", ["var1a", "var1b", "tags", "_persistent"]]]) '));

log(DEBUG, "tc2");
registerEntityClass(tc2, "wak");

eval(assert(' getEntityClass("tc2") === tc2 '));
eval(assert(' getEntitySauerType("tc2") === "wak" '));

// Child classes with sauerType from ancestor

var tc2_1 = tc2.extend({
    _class: "tc2_1"
});

var tc2_1_1 = tc2_1.extend({
    _class: "tc2_1_1"
});

registerEntityClass(tc2_1, undefined);
eval(assert(' getEntitySauerType("tc2_1") === "wak" '));

registerEntityClass(tc2_1_1, undefined);
eval(assert(' getEntitySauerType("tc2_1_1") === "wak" '));

// Child classes with sauerType from ancestor with skipping

var tc1_1 = tc1.extend({
    _class: "tc1_1",
    var1_1a: new StateVariable()
});

var tc1_1_1 = tc1_1.extend({
    _class: "tc1_1_1",
    var1_1_1a: new StateVariable()
});

MessageSystem.feedback = [];

registerEntityClass(tc1_1_1, undefined);

eval(assert(' serializeJSON(MessageSystem.feedback) === serializeJSON([["tc1_1_1", ["var1_1_1a", "var1_1a", "var1a", "var1b", "tags", "_persistent"]]]) ')); // All variables should be present, even from ancestors

eval(assert(' getEntitySauerType("tc1_1_1") === "nada" '));

// Restore all

_logicEntityClasses = {};
eval(assert(' arrayEqual(keys(_logicEntityClasses), []) '));

Testing.restoreEnvironment();

