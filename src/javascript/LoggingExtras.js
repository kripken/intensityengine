
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

