
/*
 *=============================================================================
 * Copyright (C) 2008 Alon Zakai ('Kripken') kripkensteiner@gmail.com
 *
 * This file is part of the Intensity Engine project,
 *    http://www.intensityengine.com
 *
 * The Intensity Engine is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * The Intensity Engine is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with the Intensity Engine.  If not, see
 *     http://www.gnu.org/licenses/
 *     http://www.gnu.org/licenses/agpl-3.0.html
 *=============================================================================
 */

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

