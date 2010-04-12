
// Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
// This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

var save = ApplicationManager.instance;

var ApplicationTest = Application.extend({
});

ApplicationManager.setApplicationClass(ApplicationTest);

eval(assert(' ApplicationManager.instance instanceof ApplicationTest '));

ApplicationManager.instance = save;

eval(assert(' typeof ApplicationManager.instance.performClick === "function" '));
eval(assert(' ApplicationManager.instance.getPcClass() === "Player" '));

// TODO: more tests

