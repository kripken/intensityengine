
// Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
// This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

eval(assert(' typeof Global === "object" '));
eval(assert(' typeof Global.initAsClient === "function" '));
eval(assert(' typeof Global.initAsServer === "function" '));

eval(assert(' !Global.CLIENT '));
eval(assert(' !Global.SERVER '));

Global.initAsClient();
eval(assert(' Global.CLIENT '));
eval(assert(' !Global.SERVER '));

Global.initAsServer();
eval(assert(' !Global.CLIENT '));
eval(assert(' Global.SERVER '));

// Restore initial state
Global.CLIENT = false;
Global.SERVER = false;

