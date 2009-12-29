
//=============================================================================
// Copyright (C) 2008 Alon Zakai ('Kripken') kripkensteiner@gmail.com
//
// This file is part of the Intensity Engine project,
//    http://www.intensityengine.com
//
// The Intensity Engine is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published by
// the Free Software Foundation, version 3.
//
// The Intensity Engine is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with the Intensity Engine.  If not, see
//     http://www.gnu.org/licenses/
//     http://www.gnu.org/licenses/agpl-3.0.html
//=============================================================================

log(DEBUG, "Utilities tests 1");

// Vectors

var v = new Vector3(1,5,3);

log(DEBUG, "Utilities tests 2");

eval(assert(' v.x === 1.0 && v.y === 5.0 && v.z === 3.0 '));

eval(assert(' Math.abs( Math.sqrt(35) - v.magnitude()) < 0.001 ')); // Float math, so just approximating

v.normalize();
eval(assert(' v.magnitude() === 1.0 '));

v = new Vector3(1,5,3);
var w = new Vector3(10,20,30);
var t = w.subNew(v);
eval(assert(' t.x === 9.0 && t.y === 15.0 && t.z === 27.0 '));
eval(assert(' v.x === 1.0 && v.y === 5.0 && v.z === 3.0 '));
t = w.addNew(v);
eval(assert(' t.x === 11.0 && t.y === 25.0 && t.z === 33.0 '));
eval(assert(' v.x === 1.0 && v.y === 5.0 && v.z === 3.0 '));
var t = v.mulNew(2);
eval(assert(' t.x === 2.0 && t.y === 10.0 && t.z === 6.0 '));
eval(assert(' v.x === 1.0 && v.y === 5.0 && v.z === 3.0 '));

v = new Vector3(1,5,3);
w = new Vector3(10,20,30);
w.sub(v);
eval(assert(' w.x === 9.0 && w.y === 15.0 && w.z === 27.0 '));
eval(assert(' v.x === 1.0 && v.y === 5.0 && v.z === 3.0 '));
w = new Vector3(10,20,30);
w.add(v);
eval(assert(' w.x === 11.0 && w.y === 25.0 && w.z === 33.0 '));
eval(assert(' v.x === 1.0 && v.y === 5.0 && v.z === 3.0 '));
w = new Vector3(10,20,30);
w.mul(2);
eval(assert(' w.x === 20.0 && w.y === 40.0 && w.z === 60.0 '));
eval(assert(' v.x === 1.0 && v.y === 5.0 && v.z === 3.0 '));

log(DEBUG, "Utilities tests 25");

v = new Vector3(1,5,3);
w = v;
w.x = 2;
eval(assert(' v.x === 2.0 && v.y === 5.0 && v.z === 3.0 '));
w = v.copy();
w.x = 17
eval(assert(' v.x === 2.0 && v.y === 5.0 && v.z === 3.0 '));
eval(assert(' w.x === 17.0 && w.y === 5.0 && w.z === 3.0 '));

log(DEBUG, "Utilities tests 33");

v = new Vector3(1,5,3);
log(DEBUG, "Utilities tests 34");

v.x = 887; // was: tests with v[0], etc.
log(DEBUG, "Utilities tests 35");

v.y = 18;
log(DEBUG, "Utilities tests 36");

v.z = 1337;
log(DEBUG, "Utilities tests 37");

eval(assert(' v.x === 887.0 && v.y === 18.0 && v.z === 1337.0 ')); // If v[0], etc. in these last few lines, crashes(!) V8. Report. XXX

// Angles

log(DEBUG, "Utilities tests 50");

eval(assert(' normalizeAngle(200, 0) === -160 '));
eval(assert(' normalizeAngle(50, 0) === 50 '));
eval(assert(' normalizeAngle(0, 0) === 0 '));
eval(assert(' normalizeAngle(-50, 0) === -50 '));
eval(assert(' normalizeAngle(-200, 0) === 160 '));

eval(assert(' normalizeAngle(200, 180) === 200 '));
eval(assert(' normalizeAngle(50, 180) === 50 '));
eval(assert(' normalizeAngle(0, 180) === 0 '));
eval(assert(' normalizeAngle(-50, 180) === 310 '));
eval(assert(' normalizeAngle(-200, 180) === 160 '));

eval(assert(' Math.abs(yawTo(new Vector3(10, 100, 77), new Vector3(50, 80,59)) - 63.434) < 0.01 '));
eval(assert(' Math.abs(pitchTo(new Vector3(10, 100, 77), new Vector3(50, 80,59)) - -21.924) < 0.01 '));

eval(assert(' compareYaw(new Vector3(10, 100, 77), new Vector3(50, 80,59), 63.434, 0.01) === true '));
eval(assert(' compareYaw(new Vector3(10, 100, 77), new Vector3(50, 80,59), 63.434, 0.0001) === false '));

eval(assert(' sign(-2) === -1 '));
eval(assert(' sign(-1) === -1 '));
eval(assert(' sign(0) === 0 '));
eval(assert(' sign(1) === 1 '));
eval(assert(' sign(2) === 1 '));

eval(assert(' distance(new Vector3(10, 100, 77), new Vector3(10, 100, 77)) === 0 '));
eval(assert(' Math.abs(distance(new Vector3(1, 3, 7), new Vector3(2, 8, 15)) - 9.486) < 0.01 '));
eval(assert(' distance(new Vector3(1, 3, 7), new Vector3(2, 8, 15)) === distance(new Vector3(2, 8, 15), new Vector3(1, 3, 7)) '));

log(DEBUG, "Utilities tests 100");

