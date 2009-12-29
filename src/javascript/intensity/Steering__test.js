
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

