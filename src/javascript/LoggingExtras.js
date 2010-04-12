
// Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
// This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

function AssertionError(message) {
    this.message = message;
    this.name = "AssertionError";
}


// Export as global
log = Logging.log

INFO = Logging.INFO;
DEBUG = Logging.DEBUG;
WARNING = Logging.WARNING;
ERROR = Logging.ERROR;
OFF = Logging.OFF;

function debugDump(object, level) {
    if (level === undefined) {
        level = DEBUG;
    };
    log(level, "DUMP of " + object);
    for (var key in object) {
        log(level, "   " + key + ": " + object[key]);
    }
};

//! Assert is used as follows:
//!     eval(assert("expression that should be true"));
function assert(expression) {
    return "if (!(" + expression + ")) { log(ERROR, '<<SCRIPT>> Assertion error on: " + expression + "'); log(ERROR, 'stack:'); log(ERROR, (new Error).stack); throw new AssertionError('" + expression + "'); };"; // stackTrace can cause infinite looping sometimes // /*log(ERROR, stackTrace());*/
};

function assertException(expression) {
    try {
        eval(expression);
        Logging.log(Logging.ERROR, "<< JAVASCRIPT >> Assertion error on '" + expression + "' not leading to an error, but it should");
    } catch (e) {
        return; // All ok
    }
    throw new AssertionError(expression + " did not lead to an error");
};

