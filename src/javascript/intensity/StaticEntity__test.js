
// Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
// This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

Testing.replaceEnvironment();

eval(assert(' getEntityClass("StaticEntity") === StaticEntity '));
eval(assert(' getEntitySauerType("StaticEntity") === "mapmodel" '));

eval(assert(' getEntityClass("Light") === Light '));
eval(assert(' getEntitySauerType("Light") === "light" '));

eval(assert(' getEntityClass("ParticleEffect") === ParticleEffect '));
eval(assert(' getEntitySauerType("ParticleEffect") === "particles" '));

eval(assert(' getEntityClass("Mapmodel") === Mapmodel '));
eval(assert(' getEntitySauerType("Mapmodel") === "mapmodel" '));

eval(assert(' getEntityClass("AreaTrigger") === AreaTrigger '));
eval(assert(' getEntitySauerType("AreaTrigger") === "mapmodel" '));

eval(assert(' getEntityClass("ResettableAreaTrigger") === ResettableAreaTrigger '));
eval(assert(' getEntitySauerType("ResettableAreaTrigger") === "mapmodel" '));

eval(assert(' getEntityClass("PlayerStart") === PlayerStart '));
eval(assert(' getEntitySauerType("PlayerStart") === "playerstart" '));

eval(assert(' getEntityClass("WorldMarker") === WorldMarker '));
eval(assert(' getEntitySauerType("WorldMarker") === "playerstart" '));

if (Global.SERVER) {
    log(DEBUG, "Light:");

    var test = new Light();

    eval(assert(' test._sauerType === "extent" '));

    log(DEBUG, "Light: init");

    test.init(878);

    eval(assert(' test.uniqueId === 878 '));

    eval(assert(' !test.canCallCFuncs() '));
    var count = 0; forEach(keys(test._queuedStateVariableChanges), function() { count += 1; });
    eval(assert(' count > 0 '));

    eval(assert(' test.radius === 100 '));
    eval(assert(' test.red === 128 '));
    eval(assert(' test.green === 128 '));
    eval(assert(' test.blue === 128 '));

    eval(assert(' test.position instanceof Vector3Surrogate '));
    eval(assert(' test.position.x === 511 '));
    eval(assert(' test.position.y === 512 '));
    eval(assert(' test.position.z === 513 '));

    log(DEBUG, "Light: activate");

    test.activate(undefined, 200, 400, 800);

    eval(assert(' CAPI.feedback["setupExtent"] === 878 '));

    eval(assert(' test.canCallCFuncs() ')); // Flushing occurred

    eval(assert(' arrayEqual(test.position.asArray(), [700, 11, 5]) '));

    test.position = [111, 222, 444];
    eval(assert(' arrayEqual(test.position.asArray(),  [700, 11, 5]) ')); // Out CAPI shell doesn't actually set
    eval(assert(' CAPI.feedback["setExtentO"] === 878 '));

    log(DEBUG, "Alias tests");

    test.radius = 4;
    eval(assert(' test.radius === 878+11 '));
    eval(assert(' test.attr1 === 878+11 '));

    test.red = 84;
    eval(assert(' test.red === 878+22 '));
    eval(assert(' test.attr2 === 878+22 '));

    test.green = 184;
    eval(assert(' test.green === 878+33 '));
    eval(assert(' test.attr3 === 878+33 '));

    test.blue = 844;
    eval(assert(' test.blue === 878+44 '));
    eval(assert(' test.attr4 === 878+44 '));

    // State data application - should occur at activation

    log(DEBUG, "Light (w/stateData):");

    test = new Light();

    var setSDfeedback = [];

    test._setStateDatum = function(key, value) {
        setSDfeedback.push([key, value]);
        this.stateVariableValues[key] = value;
    };

    var dummyKwargs = { stateData: "{position: '[1,20,300]', red: 11, green: 271}" };

    test.init(878, dummyKwargs);

    log(DEBUG, "SSDF 1:" + serializeJSON(setSDfeedback));

    forEach(values(setSDfeedback), function(pair) {
        eval(assert(' pair[0] !== "red" '));
        eval(assert(' pair[0] !== "green" '));
    });

    test.activate(dummyKwargs);

    log(DEBUG, "SSDF 2:" + serializeJSON(setSDfeedback));

    var okRed = false;
    var okGreen = false;
    forEach(values(setSDfeedback), function(pair) {
        okRed = okRed | (pair[0] === "red" && pair[1] === 11);
        okGreen = okGreen | (pair[0] === "green" && pair[1] === 271);
    });

    eval(assert(' okRed '));
    eval(assert(' okGreen '));

    //

    log(DEBUG, "ParticleEffect:");

    var test = new ParticleEffect();

    eval(assert(' test._sauerType === "extent" '));

    test.init(878);

    eval(assert(' test.uniqueId === 878 '));

    eval(assert(' !test.canCallCFuncs() '));
    count = 0; forEach(keys(test._queuedStateVariableChanges), function() { count += 1; });
    eval(assert(' count > 0 '));

    eval(assert(' test.particleType === 0 '));
    eval(assert(' test.value1 === 0 '));
    eval(assert(' test.value2 === 0 '));
    eval(assert(' test.value3 === 0 '));

    test.activate(undefined, 200, 400, 800);

    eval(assert(' CAPI.feedback["setupExtent"] === 878 '));

    eval(assert(' test.canCallCFuncs() ')); // Flushing occurred

    eval(assert(' arrayEqual(test.position.asArray(), [700, 11, 5]) '));

    test.position = [111, 222, 444];
    eval(assert(' arrayEqual(test.position.asArray(),  [700, 11, 5]) ')); // Out CAPI shell doesn't actually set
    eval(assert(' CAPI.feedback["setExtentO"] === 878 '));

    log(DEBUG, "Alias tests");

    test.particleType = 4;
    eval(assert(' test.particleType === 878+11 '));
    eval(assert(' test.attr1 === 878+11 '));

    test.value1 = 84;
    eval(assert(' test.value1 === 878+22 '));
    eval(assert(' test.attr2 === 878+22 '));

    test.value2 = 184;
    eval(assert(' test.value2 === 878+33 '));
    eval(assert(' test.attr3 === 878+33 '));

    test.value3 = 844;
    eval(assert(' test.value3 === 878+44 '));
    eval(assert(' test.attr4 === 878+44 '));

    //

    log(DEBUG, "Mapmodel:");

    var test = new Mapmodel();

    eval(assert(' test._sauerType === "extent" '));

    test.init(878);

    eval(assert(' test.uniqueId === 878 '));

    eval(assert(' !test.canCallCFuncs() '));
    count = 0; forEach(keys(test._queuedStateVariableChanges), function() { count += 1; });
    eval(assert(' count > 0 '));

    eval(assert(' test.yaw === 0 '));

    test.activate(undefined, 200, 400, 800);

    eval(assert(' CAPI.feedback["setupExtent"] === 878 '));

    eval(assert(' test.canCallCFuncs() ')); // Flushing occurred

    eval(assert(' arrayEqual(test.position.asArray(), [700, 11, 5]) '));

    test.position = [111, 222, 444];
    eval(assert(' arrayEqual(test.position.asArray(),  [700, 11, 5]) ')); // Out CAPI shell doesn't actually set
    eval(assert(' CAPI.feedback["setExtentO"] === 878 '));

    test.yaw = 4;
    eval(assert(' test.yaw === 878+11 '));
    eval(assert(' test.attr1 === 878+11 '));

    //

    log(DEBUG, "AreaTrigger:");

    var test = new AreaTrigger();

    eval(assert(' test._sauerType === "extent" '));

    test.init(878);

    eval(assert(' test.uniqueId === 878 '));

    eval(assert(' !test.canCallCFuncs() '));
    count = 0; forEach(keys(test._queuedStateVariableChanges), function() { count += 1; });
    eval(assert(' count > 0 '));

    eval(assert(' test.yaw === 0 '));

    test.activate(undefined, 200, 400, 800);

    eval(assert(' CAPI.feedback["setupExtent"] === 878 '));

    eval(assert(' test.canCallCFuncs() ')); // Flushing occurred

    eval(assert(' arrayEqual(test.position.asArray(), [700, 11, 5]) '));

    test.position = [111, 222, 444];
    eval(assert(' arrayEqual(test.position.asArray(),  [700, 11, 5]) ')); // Out CAPI shell doesn't actually set
    eval(assert(' CAPI.feedback["setExtentO"] === 878 '));

    eval(assert(' test.scriptToRun === "" '));
    test.scriptToRun = 'runMe';
    eval(assert(' test.scriptToRun === "runMe" '));

    var collider = {};
    var runMe = function(entity) {
        entity.feedbackOC = 1976;
    };
    eval(assert(' collider.feedbackOC === undefined '));
    test.onCollision(collider);
    eval(assert(' collider.feedbackOC === 1976 '));

    //

    log(DEBUG, "ResettableAreaTrigger:");

    var test = new ResettableAreaTrigger();

    eval(assert(' test._sauerType === "extent" '));

    test.init(878);

    eval(assert(' test.uniqueId === 878 '));

    eval(assert(' !test.canCallCFuncs() '));
    count = 0; forEach(keys(test._queuedStateVariableChanges), function() { count += 1; });
    eval(assert(' count > 0 '));

    eval(assert(' test.yaw === 0 '));

    test.activate(undefined, 200, 400, 800);

    eval(assert(' CAPI.feedback["setupExtent"] === 878 '));

    eval(assert(' test.canCallCFuncs() ')); // Flushing occurred

    eval(assert(' arrayEqual(test.position.asArray(), [700, 11, 5]) '));

    test.position = [111, 222, 444];
    eval(assert(' arrayEqual(test.position.asArray(),  [700, 11, 5]) ')); // Out CAPI shell doesn't actually set
    eval(assert(' CAPI.feedback["setExtentO"] === 878 '));

    var collider = {};

    test.feedbackOnReset = 0;

    test.onReset = function() {
        test.feedbackOnReset += 1;
    };

    test.onTrigger = function(entity) {
        entity.feedbackOC = 1776;
    };

    eval(assert(' test.feedbackOnReset === 0 '));
    eval(assert(' collider.feedbackOC === undefined '));
    test.onCollision(collider);
    eval(assert(' test.feedbackOnReset === 0 '));
    eval(assert(' collider.feedbackOC === 1776 '));

    test.reset();
    eval(assert(' test.feedbackOnReset === 1 '));

    eval(assert(' test.scriptToRun === "" '));
    test.scriptToRun = 'runMe'; // Now we use a script, instead of onTrigger
    eval(assert(' test.scriptToRun === "runMe" '));

    test.onCollision(collider);
    eval(assert(' test.feedbackOnReset === 1 '));
    eval(assert(' collider.feedbackOC === 1976 '));

} else {
    // Client
}

Testing.restoreEnvironment();

