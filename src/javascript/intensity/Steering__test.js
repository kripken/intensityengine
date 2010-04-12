
// Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
// This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

// moveAngleTowards

eval(assert(' moveAngleTowards(10, 100, 50) === 60 '));
eval(assert(' moveAngleTowards(10, 100, 150) === 100 '));
eval(assert(' moveAngleTowards(310, 200, 50) === 260 '));
eval(assert(' moveAngleTowards(310, 200, 150) === 200 '));

// faceTowards

var actor = { position: new Vector3(100, 100, 100), yaw: 0, pitch: 0 };

var actorData = serializeJSON(actor);

var target = new Vector3(200, 300, 100);

eval(assert(' Math.abs(yawTo(actor.position, target) - 153.434) < 0.01 '));
eval(assert(' Math.abs(distance(actor.position, target) - 223.606) < 0.01 '));
eval(assert(' actor.yaw === 0 '));

faceTowards(actor, target, 10);

eval(assert(' Math.abs(distance(actor.position, target) - 223.606) < 0.01 '));
eval(assert(' actor.yaw === 10 '));

faceTowards(actor, target, 10);

eval(assert(' Math.abs(distance(actor.position, target) - 223.606) < 0.01 '));
eval(assert(' actor.yaw === 20 '));

faceTowards(actor, target, 150);

eval(assert(' Math.abs(distance(actor.position, target) - 223.606) < 0.01 '));
eval(assert(' Math.abs(actor.yaw - 153.434) < 0.01 '));

// moveTowards - but we don't do actual movement here

actor = { position: new Vector3(100, 100, 100), yaw: 0, pitch: 0, movementSpeed: 10, move: -10 };

eval(assert(' Math.abs(yawTo(actor.position, target) - 153.434) < 0.01 '));
eval(assert(' Math.abs(distance(actor.position, target) - 223.606) < 0.01 '));
eval(assert(' actor.yaw === 0 '));

moveTowards(actor, target, 10);

eval(assert(' actor.yaw === 10 '));

moveTowards(actor, target, 10);

eval(assert(' actor.yaw === 20 '));

moveTowards(actor, target, 150);

eval(assert(' Math.abs(actor.yaw - 153.434) < 0.01 '));
eval(assert(' actor.move = 1 '));

