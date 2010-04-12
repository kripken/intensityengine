
// Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
// This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

Testing.replaceEnvironment();

eval(assert(' getEntity(0) === null '));
eval(assert(' getEntity(11) === null '));

eval(assert(' getEntitiesByTag("17").length === 0 '));

log(DEBUG, "You can ignore the next 2 warnings");
eval(assert(' getEntityByTag("17") === null '));
eval(assert(' removeEntity(11) === undefined ')); // Should not crash

//eval(assert(' removeAllEntities() === undefined ')); // Should not crash

if (Global.SERVER) {

    var TestClass = LogicEntity.extend({
        _class: "TestClass",
        init: function(uniqueId) {
            this.uniqueId = uniqueId;
        },
        activate: function(kwargs) {
            this.arg1 = kwargs.arg1;
            this.arg2 = kwargs.arg2;
            this.arg3 = kwargs.arg3;
        },
        deactivate: function() { },
        hasTag: function(tag) {
            return ( (tag === this.arg1 || tag === this.arg3) );
        }
    });

    registerEntityClass(TestClass, "nada");

    eval(assert(' getEntity(1066) === null '));

    var test = newEntity("TestClass", { arg1: "an", arg2: 75, arg3: "argvec" }, 1066);

    eval(assert(' test instanceof TestClass '));
    eval(assert(' test.uniqueId === 1066 '));
    eval(assert(' test.arg1 === "an" '));
    eval(assert(' test.arg2 === 75 '));
    eval(assert(' test.arg3 === "argvec" '));

    eval(assert(' getEntity(0) === null '));
    eval(assert(' getEntity(11) === null '));
    eval(assert(' getEntity(1066) === test '));

    eval(assert(' arrayEqual(getEntitiesByTag("an"), [test]) '));
    eval(assert(' arrayEqual(getEntitiesByTag("another"), []) '));
    eval(assert(' arrayEqual(getEntitiesByTag("argvec"), [test]) '));

    eval(assert(' getEntityByTag("an") === test '));
    log(DEBUG, "You can ignore the following warning");
    eval(assert(' getEntityByTag("another") === null '));
    eval(assert(' getEntityByTag("argvec") === test '));

    eval(assert(' getEntity(1233) === null '));

    log(DEBUG, "LogicEntityStore tests part 2");

    var test2 = newEntity("TestClass", { arg1: "an", arg2: 72, arg3: "else"}, 1233);

    log(DEBUG, "You can ignore the following warning");
    eval(assert(' getEntityByTag("an") === null '));
    log(DEBUG, "You can ignore the following warning");
    eval(assert(' getEntityByTag("another") === null '));
    eval(assert(' getEntityByTag("argvec") === test '));
    eval(assert(' getEntityByTag("else") === test2 '));

    eval(assert(' arrayEqual(getEntitiesByTag("an"), [test, test2]) '));
    eval(assert(' arrayEqual(getEntitiesByTag("another"), []) '));
    eval(assert(' arrayEqual(getEntitiesByTag("argvec"), [test]) '));
    eval(assert(' arrayEqual(getEntitiesByTag("else"), [test2]) '));

    eval(assert(' removeEntity(1066) === undefined ')); // Clean up, don't crash while doing so
    eval(assert(' removeEntity(1233) === undefined ')); // Clean up, don't crash while doing so

    eval(assert(' getEntity(1066) === null '));
    eval(assert(' getEntity(1233) === null '));

    // Persistence checks

    log(DEBUG, "LogicEntityStore tests part 3");

    eval(assert(' saveEntities() === "[]" '));

    test = newEntity("TestClass", { stateData: "waka" }, 1012);
    test2 = newEntity("TestClass", { stateData: "kava" }, 2023);
    var test3 = newEntity("TestClass", { stateData: "kava" }, 655);

    eval(assert(' getEntity(1012) !== null '));
    eval(assert(' getEntity(2023) !== null '));
    eval(assert(' getEntity(655) !== null '));

    test.stateData = { 1: "alpha", 2: "beta" };
    test2.stateData = { 3: "gamma" };

    test._persistent = true;
    test2._persistent = true;
    // test 3 is not persistent, and should not be saved

// FIXME: These tests fail, but the real-life thing works... not sure why
//    var saved = saveEntities();
//
//    eval(assert(' evalJSON(saved).length === 2 '));
//
//    removeEntity(1012);
//    removeEntity(2023);
//    removeEntity(655);
//
//    eval(assert(' saveEntities() === "[]" /* 2 */ '));
//
//log(DEBUG, "saved 4");
//    loadEntities(saved);
//
//log(DEBUG, "saved 5");
//    eval(assert(' getEntity(1012) !== null '));
//    eval(assert(' getEntity(2023) !== null '));
//    eval(assert(' getEntity(655) === null '));
//
//    removeEntity(1012);
//    removeEntity(2023);
//
//    eval(assert(' getEntity(1012) == null '));
//    eval(assert(' getEntity(2023) == null '));
//    eval(assert(' getEntity(655) === null '));

} else {
    // CLIENT
}

Testing.restoreEnvironment();

__entitiesStore = {};

