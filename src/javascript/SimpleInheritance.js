
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

// Based on simple inheritance system, due to John Resig
// http://ejohn.org/blog/simple-javascript-inheritance/
// Thanks!
//
// * HISTORY:
// *     Jan 2009 Alon Zakai ('kripken')
// *         Renamed 'init' to 'create', as we use
// *             'init' for other things.
// *         Added _name and stack tracing to use _name
// *         Added TRACING as an option
// *         Added __new__
//

var TRACING = true;

(function(){
  var initializing = false, fnTest = /xyz/.test(function(){xyz;}) ? /\b_super\b/ : /.*/;

  // The base Class implementation (does nothing)
  this.Class = function(){};
 
  // Create a new Class that inherits from this class
  Class.extend = function(prop) {
    var _super = this.prototype;
   
    // Instantiate a base class (but only create the instance,
    // don't run the create constructor)
    initializing = true;
    var prototype = new this();
    initializing = false;
   
    // Copy the properties over onto the new prototype
    for (var name in prop) {
      // Check if we're overwriting an existing function
      prototype[name] = typeof prop[name] == "function" &&
        typeof _super[name] == "function" && fnTest.test(prop[name]) ?
        (function(name, fn){
          return function() {
            var tmp = this._super;
           
            // Add a new ._super() method that is the same method
            // but on the super-class
            this._super = _super[name];
           
            // The method only need to be bound temporarily, so we
            // remove it when we're done executing
            var ret = fn.apply(this, arguments);       
            this._super = tmp;
           
            return ret;
          };
        })(name, prop[name]) :
        prop[name];

        if (TRACING) {
            // Kripken: Naming for stack dumps
            if (typeof prop[name] === 'function') {
                prototype[name]._name = name;
                try {
                    prototype[name].__sourcefile = __MODULE_SOURCE;
                } catch (e) {
                    prototype[name].__sourcefile = "--source unknown--";
                }
            }
        }
    }
   
    // The dummy class constructor
    function Class() {
      // All construction is actually done in the create method
      if ( !initializing && this.create )
        this.create.apply(this, arguments);
    }
   
    // Populate our constructed prototype object
    Class.prototype = prototype;
   
    // Enforce the constructor to be what we expect
    Class.constructor = Class;

    // And make this class extendable
    Class.extend = arguments.callee;
   
    return Class;
  };
})();


//! Do not use stackTrace, it may cause infinite loops!
function stackTrace(expandAnonymous) {
    log(ERROR, "Do not run stackTrace, it may cause errors!");
    var curr = stackTrace.caller; // Mozilla-specific?
    var ret = "\n\nSTACK TRACE: \n";
    var i = 0;

    while (curr != null) {
        var _name = "";
        if (curr.name !== undefined && typeof curr.name === 'string' && curr.name.length > 0) {
            _name = curr.name
        } else if (curr._name !== undefined) {
            _name = curr._name + " (" + curr.__sourcefile + ")";
        } else {
            if (expandAnonymous) {
                _name = curr.toString();
            } else {
                _name = "[anonymous]";
            }
        }

        ret += "(" + i + ") " + _name + "\n";
        i += 1;
        curr = curr.caller;
    }

    return ret;
}

//! @param _class A class (i.e., constructor function)
function traverseAncestors(_class, operation, haltCondition) {
    var currClass = _class.prototype.__proto__; // Immediate parent

    while (!haltCondition(currClass)) {
        operation(currClass);
        currClass = currClass.__proto__; // XXX Mozilla-specific, but necessary
    }
}

//! __new__( Class, arg1, arg2, arg3 ) creates a new object of that class, with
//! those parameters. It is a simple replacement for the "new Class(...)" notation
//! which is cumbersome to call from C++.
function __new__() {
    var _args = Array.prototype.slice.call(arguments);

    var _class = _args[0];
    var __args = _args.slice(1, arguments.length);

    var DummyClass = _class.extend({
        create: function() {
            this._super(__args);
        }
    });

    return new DummyClass();
}

