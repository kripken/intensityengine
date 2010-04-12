
// Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
// This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

Testing.replaceEnvironment();

eval(assert(' getEntityClass("Character") === Character '));
eval(assert(' getEntitySauerType("Character") === "dynent" '));

eval(assert(' getEntityClass("Player") === Player '));
eval(assert(' getEntitySauerType("Player") === "dynent" '));

if (Global.SERVER) {
    var test = new Character();

    eval(assert(' test._sauerType === "fpsent" '));

    test.init(1112);

    eval(assert(' test.uniqueId === 1112 '));

    eval(assert(' !test.canCallCFuncs() '));
    var count = 0; forEach(keys(test._queuedStateVariableChanges), function() { count += 1; });
    eval(assert(' count > 0 '));

    test.activate({ clientNumber: 67 });

    eval(assert(' test.canCallCFuncs() ')); // Flushing occurred

    eval(assert(' test.clientNumber === 67 '));

    test.facingSpeed = 80;
    eval(assert(' test.facingSpeed === 80 '));

    test.movementSpeed = 81;
    eval(assert(' test.movementSpeed === 81 '));

    test.yaw = 82;
    eval(assert(' test.yaw === 82 '));

    test.pitch = 83;
    eval(assert(' test.pitch === 83 '));

    eval(assert(' CAPI.feedback["setMove"] === undefined ')); // TODO: Add these for other assignments
    test.move = 84;
    eval(assert(' test.move === 84 '));
    eval(assert(' CAPI.feedback["setMove"] === 1112 '));

    test.strafe = 85;
    eval(assert(' test.strafe === 85 '));

    test.position = [8,9,11];
    eval(assert(' arrayEqual(test.position.asArray(), [8,9,11]) '));

    test.velocity = [1,7,55];
    eval(assert(' arrayEqual(test.velocity.asArray(), [1,7,55]) '));

    test.radius = 87;
    eval(assert(' test.radius === 87 '));

    test.aboveEye = 88;
    eval(assert(' test.aboveEye === 88 '));

    test.eyeHeight = 89;
    eval(assert(' test.eyeHeight === 89 '));

    test.blocked = true;
    eval(assert(' test.blocked === true '));

    eval(assert(' CAPI.feedback["setJumping"] === undefined '));
    test.jump();
    eval(assert(' CAPI.feedback["setJumping"] === 1112 '));

    removeEntity(1112);

} else {
    // Client
}

Testing.restoreEnvironment();

