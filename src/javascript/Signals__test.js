
// Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
// This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

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

