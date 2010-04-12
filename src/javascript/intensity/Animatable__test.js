
// Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
// This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

Testing.replaceEnvironment();

if (Global.SERVER) {
    // SERVER
    var test = new AnimatableLogicEntity();

    eval(assert(' typeof test.canCallCFuncs === "function" '));

    test.init(77);

    eval(assert(' test.uniqueId === 77 '));

    eval(assert(' test.modelName === "" '));
    eval(assert(' arrayEqual(test.attachments, []) '));
    eval(assert(' test.animation === ANIM_IDLE | ANIM_LOOP '));

    eval(assert(' !test.canCallCFuncs() '));

    var count = 0; forEach(keys(test._queuedStateVariableChanges), function() { count += 1; });
    eval(assert(' count === 3 ')); // The three mentioned above
                                   // Animatable does not flush SV changes

    test.animation = 220022;
    eval(assert(' test.animation === 220022 '));
    count = 0; forEach(keys(test._queuedStateVariableChanges), function() { count += 1; });
    eval(assert(' count === 3 ')); // No more queued - we repeated an old one

    test.setAttachment('akey', 'awest');
    eval(assert(' arrayEqual(test.attachments.asArray(), ["akey,awest"]) '));
    test.setAttachment('bkey', 'bnorth');
    eval(assert(' arrayEqual(test.attachments.asArray(), ["akey,awest", "bkey,bnorth"]) '));
    test.setAttachment('akey', null);
    eval(assert(' arrayEqual(test.attachments.asArray(), ["bkey,bnorth"]) '));

    test.activate();

    eval(assert(' test.modelName === "" '));

} else {
    // CLIENT
}

Testing.restoreEnvironment();

