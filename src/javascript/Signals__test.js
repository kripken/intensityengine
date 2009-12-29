
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

assert("typeof Object.addSignalMethods === 'function'");

var test = {};
Object.addSignalMethods(test);
assert( "typeof test.emit === 'function'" );

Logging.log(Logging.DEBUG, "You should now see a message about a callback being run:");
var bval = 10;
function callb(value) { Logging.log(Logging.WARNING, 'A callback warning which you can ignore.'); bval = 7119+value; };
var test2 = {};
Object.addSignalMethods(test2);
test2.connect('asig', callb);
test2.emit('asig', 15);
assert( "bval === 7119+15" );

