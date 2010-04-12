
// Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
// This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

var myArray = [1, 2, "3", null];

assert( "typeof serializeJSON(myArray) === 'string'" );
assert( "typeof(evalJSON(serializeJSON(myArray))[1]) === 'number'" );
assert( "typeof(evalJSON(serializeJSON(myArray))[2]) === 'string'" );
assert( "objEqual(evalJSON(serializeJSON(myArray)), myArray)" );

// We have the following change to stock MochiKit:
//var me=arguments.callee;
//var _f5;
//if(o === undefined) return "[UNDEFINED]"; // Kripken: Added this!
//if(typeof (o.__json__)=="function"){
//_f5=o.__json__();

