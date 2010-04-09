/***

MochiKit.Base 1.5 (Revision 1561, Feb 11 2010)

See <http://mochikit.com/> for documentation, downloads, license, etc.

(c) 2005 Bob Ippolito.  All rights Reserved.

***/

if (typeof(MochiKit) == 'undefined') {
    MochiKit = {};
}
if (typeof(MochiKit.__export__) == "undefined") {
    MochiKit.__export__ = true;
}
if (typeof(MochiKit.Base) == 'undefined') {
    MochiKit.Base = {};
}

/**
 * Registers a new MochiKit module. This function will insert a new
 * property into the "MochiKit" object, making sure that all
 * dependency modules have already been inserted. It will also make
 * sure that the appropriate properties and default module functions
 * are defined.
 *
 * @param {String} name the module name, e.g. "Base"
 * @param {String} version the module version, e.g. "1.5"
 * @param {Array} deps the array of module dependencies (as strings)
 */
MochiKit.Base._module = function (name, version, deps) {
    if (!(name in MochiKit)) {
        MochiKit[name] = {};
    }
    var module = MochiKit[name];
    module.NAME = "MochiKit." + name;
    module.VERSION = version;
    module.__repr__ = function () {
        return "[" + this.NAME + " " + this.VERSION + "]";
    };
    module.toString = function () {
        return this.__repr__();
    };
    for (var i = 0; i < deps.length; i++) {
        if (!(deps[i] in MochiKit)) {
            throw 'MochiKit.' + name + ' depends on MochiKit.' + deps[i] + '!';
        }
    }
}

MochiKit.Base._module("Base", "1.5", []);

/** @id MochiKit.Base.update */
MochiKit.Base.update = function (self, obj/*, ... */) {
    if (self === null || self === undefined) {
        self = {};
    }
    for (var i = 1; i < arguments.length; i++) {
        var o = arguments[i];
        if (typeof(o) != 'undefined' && o !== null) {
            for (var k in o) {
                self[k] = o[k];
            }
        }
    }
    return self;
};

MochiKit.Base.update(MochiKit.Base, {
    /** @id MochiKit.Base.camelize */
    camelize: function (selector) {
        /* from dojo.style.toCamelCase */
        var arr = selector.split('-');
        var cc = arr[0];
        for (var i = 1; i < arr.length; i++) {
            cc += arr[i].charAt(0).toUpperCase() + arr[i].substring(1);
        }
        return cc;
    },

    /** @id MochiKit.Base.counter */
    counter: function (n/* = 1 */) {
        if (arguments.length === 0) {
            n = 1;
        }
        return function () {
            return n++;
        };
    },

    /** @id MochiKit.Base.clone */
    clone: function (obj) {
        var me = arguments.callee;
        if (arguments.length == 1) {
            me.prototype = obj;
            return new me();
        }
    },
    
    _flattenArray: function (res, lst) {
        for (var i = 0; i < lst.length; i++) {
            var o = lst[i];
            if (o instanceof Array) {
                arguments.callee(res, o);
            } else {
                res.push(o);
            }
        }
        return res;
    },

    /** @id MochiKit.Base.flattenArray */
    flattenArray: function (lst) {
        return MochiKit.Base._flattenArray([], lst);
    },

    /** @id MochiKit.Base.flattenArguments */
    flattenArguments: function (lst/* ...*/) {
        var res = [];
        var m = MochiKit.Base;
        var args = m.extend(null, arguments);
        while (args.length) {
            var o = args.shift();
            if (o && typeof(o) == "object" && typeof(o.length) == "number") {
                for (var i = o.length - 1; i >= 0; i--) {
                    args.unshift(o[i]);
                }
            } else {
                res.push(o);
            }
        }
        return res;
    },

    /** @id MochiKit.Base.extend */
    extend: function (self, obj, /* optional */skip) {
        // Extend an array with an array-like object starting
        // from the skip index
        if (!skip) {
            skip = 0;
        }
        if (obj) {
            // allow iterable fall-through, but skip the full isArrayLike
            // check for speed, this is called often.
            var l = obj.length;
            if (typeof(l) != 'number' /* !isArrayLike(obj) */) {
                if (typeof(MochiKit.Iter) != "undefined") {
                    obj = MochiKit.Iter.list(obj);
                    l = obj.length;
                } else {
                    throw new TypeError("Argument not an array-like and MochiKit.Iter not present");
                }
            }
            if (!self) {
                self = [];
            }
            for (var i = skip; i < l; i++) {
                self.push(obj[i]);
            }
        }
        // This mutates, but it's convenient to return because
        // it's often used like a constructor when turning some
        // ghetto array-like to a real array
        return self;
    },


    /** @id MochiKit.Base.updatetree */
    updatetree: function (self, obj/*, ...*/) {
        if (self === null || self === undefined) {
            self = {};
        }
        for (var i = 1; i < arguments.length; i++) {
            var o = arguments[i];
            if (typeof(o) != 'undefined' && o !== null) {
                for (var k in o) {
                    var v = o[k];
                    if (typeof(self[k]) == 'object' && typeof(v) == 'object') {
                        arguments.callee(self[k], v);
                    } else {
                        self[k] = v;
                    }
                }
            }
        }
        return self;
    },

    /** @id MochiKit.Base.setdefault */
    setdefault: function (self, obj/*, ...*/) {
        if (self === null || self === undefined) {
            self = {};
        }
        for (var i = 1; i < arguments.length; i++) {
            var o = arguments[i];
            for (var k in o) {
                if (!(k in self)) {
                    self[k] = o[k];
                }
            }
        }
        return self;
    },

    /** @id MochiKit.Base.keys */
    keys: function (obj) {
        var rval = [];
        for (var prop in obj) {
            rval.push(prop);
        }
        return rval;
    },

    /** @id MochiKit.Base.values */
    values: function (obj) {
        var rval = [];
        for (var prop in obj) {
            rval.push(obj[prop]);
        }
        return rval;
    },

     /** @id MochiKit.Base.items */
    items: function (obj) {
        var rval = [];
        var e;
        for (var prop in obj) {
            var v;
            try {
                v = obj[prop];
            } catch (e) {
                continue;
            }
            rval.push([prop, v]);
        }
        return rval;
    },


    _newNamedError: function (module, name, func) {
        func.prototype = new MochiKit.Base.NamedError(module.NAME + "." + name);
        module[name] = func;
    },


    /** @id MochiKit.Base.operator */
    operator: {
        // unary logic operators
        /** @id MochiKit.Base.truth */
        truth: function (a) { return !!a; },
        /** @id MochiKit.Base.lognot */
        lognot: function (a) { return !a; },
        /** @id MochiKit.Base.identity */
        identity: function (a) { return a; },

        // bitwise unary operators
        /** @id MochiKit.Base.not */
        not: function (a) { return ~a; },
        /** @id MochiKit.Base.neg */
        neg: function (a) { return -a; },

        // binary operators
        /** @id MochiKit.Base.add */
        add: function (a, b) { return a + b; },
        /** @id MochiKit.Base.sub */
        sub: function (a, b) { return a - b; },
        /** @id MochiKit.Base.div */
        div: function (a, b) { return a / b; },
        /** @id MochiKit.Base.mod */
        mod: function (a, b) { return a % b; },
        /** @id MochiKit.Base.mul */
        mul: function (a, b) { return a * b; },

        // bitwise binary operators
        /** @id MochiKit.Base.and */
        and: function (a, b) { return a & b; },
        /** @id MochiKit.Base.or */
        or: function (a, b) { return a | b; },
        /** @id MochiKit.Base.xor */
        xor: function (a, b) { return a ^ b; },
        /** @id MochiKit.Base.lshift */
        lshift: function (a, b) { return a << b; },
        /** @id MochiKit.Base.rshift */
        rshift: function (a, b) { return a >> b; },
        /** @id MochiKit.Base.zrshift */
        zrshift: function (a, b) { return a >>> b; },

        // near-worthless built-in comparators
        /** @id MochiKit.Base.eq */
        eq: function (a, b) { return a == b; },
        /** @id MochiKit.Base.ne */
        ne: function (a, b) { return a != b; },
        /** @id MochiKit.Base.gt */
        gt: function (a, b) { return a > b; },
        /** @id MochiKit.Base.ge */
        ge: function (a, b) { return a >= b; },
        /** @id MochiKit.Base.lt */
        lt: function (a, b) { return a < b; },
        /** @id MochiKit.Base.le */
        le: function (a, b) { return a <= b; },

        // strict built-in comparators
        seq: function (a, b) { return a === b; },
        sne: function (a, b) { return a !== b; },

        // compare comparators
        /** @id MochiKit.Base.ceq */
        ceq: function (a, b) { return MochiKit.Base.compare(a, b) === 0; },
        /** @id MochiKit.Base.cne */
        cne: function (a, b) { return MochiKit.Base.compare(a, b) !== 0; },
        /** @id MochiKit.Base.cgt */
        cgt: function (a, b) { return MochiKit.Base.compare(a, b) == 1; },
        /** @id MochiKit.Base.cge */
        cge: function (a, b) { return MochiKit.Base.compare(a, b) != -1; },
        /** @id MochiKit.Base.clt */
        clt: function (a, b) { return MochiKit.Base.compare(a, b) == -1; },
        /** @id MochiKit.Base.cle */
        cle: function (a, b) { return MochiKit.Base.compare(a, b) != 1; },

        // binary logical operators
        /** @id MochiKit.Base.logand */
        logand: function (a, b) { return a && b; },
        /** @id MochiKit.Base.logor */
        logor: function (a, b) { return a || b; },
        /** @id MochiKit.Base.contains */
        contains: function (a, b) { return b in a; }
    },

    /** @id MochiKit.Base.forwardCall */
    forwardCall: function (func) {
        return function () {
            return this[func].apply(this, arguments);
        };
    },

    /** @id MochiKit.Base.itemgetter */
    itemgetter: function (func) {
        return function (arg) {
            return arg[func];
        };
    },

    /** @id MochiKit.Base.bool */
    bool: function (value) {
        if (typeof(value) === "boolean" || value instanceof Boolean) {
            return value.valueOf();
        } else if (typeof(value) === "string" || value instanceof String) {
            return value.length > 0 && value != "false" && value != "null" &&
                   value != "undefined" && value != "0";
        } else if (typeof(value) === "number" || value instanceof Number) {
            return !isNaN(value) && value != 0;
        } else if (value != null && typeof(value.length) === "number") {
            return value.length !== 0
        } else {
            return value != null;
        }
    },

    /** @id MochiKit.Base.typeMatcher */
    typeMatcher: function (/* typ */) {
        var types = {};
        for (var i = 0; i < arguments.length; i++) {
            var typ = arguments[i];
            types[typ] = typ;
        }
        return function () {
            for (var i = 0; i < arguments.length; i++) {
                if (!(typeof(arguments[i]) in types)) {
                    return false;
                }
            }
            return true;
        };
    },

    /** @id MochiKit.Base.isNull */
    isNull: function (/* ... */) {
        for (var i = 0; i < arguments.length; i++) {
            if (arguments[i] !== null) {
                return false;
            }
        }
        return true;
    },

    /** @id MochiKit.Base.isUndefinedOrNull */
    isUndefinedOrNull: function (/* ... */) {
        for (var i = 0; i < arguments.length; i++) {
            var o = arguments[i];
            if (!(typeof(o) == 'undefined' || o === null)) {
                return false;
            }
        }
        return true;
    },

    /** @id MochiKit.Base.isEmpty */
    isEmpty: function (obj) {
        return !MochiKit.Base.isNotEmpty.apply(this, arguments);
    },

    /** @id MochiKit.Base.isNotEmpty */
    isNotEmpty: function (obj) {
        for (var i = 0; i < arguments.length; i++) {
            var o = arguments[i];
            if (!(o && o.length)) {
                return false;
            }
        }
        return true;
    },

    /** @id MochiKit.Base.isArrayLike */
    isArrayLike: function () {
        for (var i = 0; i < arguments.length; i++) {
            var o = arguments[i];
            var typ = typeof(o);
            if (
                (typ != 'object' && !(typ == 'function' && typeof(o.item) == 'function')) ||
                o === null ||
                typeof(o.length) != 'number' ||
                o.nodeType === 3 ||
                o.nodeType === 4
            ) {
                return false;
            }
        }
        return true;
    },

    /** @id MochiKit.Base.isDateLike */
    isDateLike: function () {
        for (var i = 0; i < arguments.length; i++) {
            var o = arguments[i];
            if (typeof(o) != "object" || o === null
                    || typeof(o.getTime) != 'function') {
                return false;
            }
        }
        return true;
    },


    /** @id MochiKit.Base.xmap */
    xmap: function (fn/*, obj... */) {
        if (fn === null) {
            return MochiKit.Base.extend(null, arguments, 1);
        }
        var rval = [];
        for (var i = 1; i < arguments.length; i++) {
            rval.push(fn(arguments[i]));
        }
        return rval;
    },

    /** @id MochiKit.Base.map */
    map: function (fn, lst/*, lst... */) {
        var m = MochiKit.Base;
        var itr = MochiKit.Iter;
        var isArrayLike = m.isArrayLike;
        if (arguments.length <= 2) {
            // allow an iterable to be passed
            if (!isArrayLike(lst)) {
                if (itr) {
                    // fast path for map(null, iterable)
                    lst = itr.list(lst);
                    if (fn === null) {
                        return lst;
                    }
                } else {
                    throw new TypeError("Argument not an array-like and MochiKit.Iter not present");
                }
            }
            // fast path for map(null, lst)
            if (fn === null) {
                return m.extend(null, lst);
            }
            // disabled fast path for map(fn, lst)
            /*
            if (false && typeof(Array.prototype.map) == 'function') {
                // Mozilla fast-path
                return Array.prototype.map.call(lst, fn);
            }
            */
            var rval = [];
            for (var i = 0; i < lst.length; i++) {
                rval.push(fn(lst[i]));
            }
            return rval;
        } else {
            // default for map(null, ...) is zip(...)
            if (fn === null) {
                fn = Array;
            }
            var length = null;
            for (var i = 1; i < arguments.length; i++) {
                // allow iterables to be passed
                if (!isArrayLike(arguments[i])) {
                    if (itr) {
                        return itr.list(itr.imap.apply(null, arguments));
                    } else {
                        throw new TypeError("Argument not an array-like and MochiKit.Iter not present");
                    }
                }
                // find the minimum length
                var l = arguments[i].length;
                if (length === null || length > l) {
                    length = l;
                }
            }
            rval = [];
            for (var i = 0; i < length; i++) {
                var args = [];
                for (var j = 1; j < arguments.length; j++) {
                    args.push(arguments[j][i]);
                }
                rval.push(fn.apply(this, args));
            }
            return rval;
        }
    },

    /** @id MochiKit.Base.xfilter */
    xfilter: function (fn/*, obj... */) {
        var rval = [];
        if (fn === null) {
            fn = MochiKit.Base.operator.truth;
        }
        for (var i = 1; i < arguments.length; i++) {
            var o = arguments[i];
            if (fn(o)) {
                rval.push(o);
            }
        }
        return rval;
    },

    /** @id MochiKit.Base.filter */
    filter: function (fn, lst, self) {
        var rval = [];
        // allow an iterable to be passed
        var m = MochiKit.Base;
        if (!m.isArrayLike(lst)) {
            if (MochiKit.Iter) {
                lst = MochiKit.Iter.list(lst);
            } else {
                throw new TypeError("Argument not an array-like and MochiKit.Iter not present");
            }
        }
        if (fn === null) {
            fn = m.operator.truth;
        }
        if (typeof(Array.prototype.filter) == 'function') {
            // Mozilla fast-path
            return Array.prototype.filter.call(lst, fn, self);
        } else if (typeof(self) == 'undefined' || self === null) {
            for (var i = 0; i < lst.length; i++) {
                var o = lst[i];
                if (fn(o)) {
                    rval.push(o);
                }
            }
        } else {
            for (var i = 0; i < lst.length; i++) {
                o = lst[i];
                if (fn.call(self, o)) {
                    rval.push(o);
                }
            }
        }
        return rval;
    },


    _wrapDumbFunction: function (func) {
        return function () {
            // fast path!
            switch (arguments.length) {
                case 0: return func();
                case 1: return func(arguments[0]);
                case 2: return func(arguments[0], arguments[1]);
                case 3: return func(arguments[0], arguments[1], arguments[2]);
            }
            var args = [];
            for (var i = 0; i < arguments.length; i++) {
                args.push("arguments[" + i + "]");
            }
            return eval("(func(" + args.join(",") + "))");
        };
    },

    /** @id MochiKit.Base.methodcaller */
    methodcaller: function (func/*, args... */) {
        var args = MochiKit.Base.extend(null, arguments, 1);
        if (typeof(func) == "function") {
            return function (obj) {
                return func.apply(obj, args);
            };
        } else {
            return function (obj) {
                return obj[func].apply(obj, args);
            };
        }
    },

    /** @id MochiKit.Base.method */
    method: function (self, func) {
        var m = MochiKit.Base;
        return m.bind.apply(this, m.extend([func, self], arguments, 2));
    },

    /** @id MochiKit.Base.compose */
    compose: function (f1, f2/*, f3, ... fN */) {
        var fnlist = [];
        var m = MochiKit.Base;
        if (arguments.length === 0) {
            throw new TypeError("compose() requires at least one argument");
        }
        for (var i = 0; i < arguments.length; i++) {
            var fn = arguments[i];
            if (typeof(fn) != "function") {
                throw new TypeError(m.repr(fn) + " is not a function");
            }
            fnlist.push(fn);
        }
        return function () {
            var args = arguments;
            for (var i = fnlist.length - 1; i >= 0; i--) {
                args = [fnlist[i].apply(this, args)];
            }
            return args[0];
        };
    },

    /** @id MochiKit.Base.bind */
    bind: function (func, self/* args... */) {
        if (typeof(func) == "string") {
            func = self[func];
        }
        var im_func = func.im_func;
        var im_preargs = func.im_preargs;
        var im_self = func.im_self;
        var m = MochiKit.Base;
        if (typeof(func) == "function" && typeof(func.apply) == "undefined") {
            // this is for cases where JavaScript sucks ass and gives you a
            // really dumb built-in function like alert() that doesn't have
            // an apply
            func = m._wrapDumbFunction(func);
        }
        if (typeof(im_func) != 'function') {
            im_func = func;
        }
        if (typeof(self) != 'undefined') {
            im_self = self;
        }
        if (typeof(im_preargs) == 'undefined') {
            im_preargs = [];
        } else  {
            im_preargs = im_preargs.slice();
        }
        m.extend(im_preargs, arguments, 2);
        var newfunc = function () {
            var args = arguments;
            var me = arguments.callee;
            if (me.im_preargs.length > 0) {
                args = m.concat(me.im_preargs, args);
            }
            var self = me.im_self;
            if (!self) {
                self = this;
            }
            return me.im_func.apply(self, args);
        };
        newfunc.im_self = im_self;
        newfunc.im_func = im_func;
        newfunc.im_preargs = im_preargs;
        return newfunc;
    },

    /** @id MochiKit.Base.bindLate */
    bindLate: function (func, self/* args... */) {
        var m = MochiKit.Base;
        var args = arguments;
        if (typeof(func) === "string") {
            args = m.extend([m.forwardCall(func)], arguments, 1);
            return m.bind.apply(this, args);
        }
        return m.bind.apply(this, args);
    },

    /** @id MochiKit.Base.bindMethods */
    bindMethods: function (self) {
        var bind = MochiKit.Base.bind;
        for (var k in self) {
            var func = self[k];
            if (typeof(func) == 'function') {
                self[k] = bind(func, self);
            }
        }
    },

    /** @id MochiKit.Base.registerComparator */
    registerComparator: function (name, check, comparator, /* optional */ override) {
        MochiKit.Base.comparatorRegistry.register(name, check, comparator, override);
    },

    _primitives: {'boolean': true, 'string': true, 'number': true},

    /** @id MochiKit.Base.compare */
    compare: function (a, b) {
        if (a == b) {
            return 0;
        }
        var aIsNull = (typeof(a) == 'undefined' || a === null);
        var bIsNull = (typeof(b) == 'undefined' || b === null);
        if (aIsNull && bIsNull) {
            return 0;
        } else if (aIsNull) {
            return -1;
        } else if (bIsNull) {
            return 1;
        }
        var m = MochiKit.Base;
        // bool, number, string have meaningful comparisons
        var prim = m._primitives;
        if (!(typeof(a) in prim && typeof(b) in prim)) {
            try {
                return m.comparatorRegistry.match(a, b);
            } catch (e) {
                if (e != m.NotFound) {
                    throw e;
                }
            }
        }
        if (a < b) {
            return -1;
        } else if (a > b) {
            return 1;
        }
        // These types can't be compared
        var repr = m.repr;
        throw new TypeError(repr(a) + " and " + repr(b) + " can not be compared");
    },

    /** @id MochiKit.Base.compareDateLike */
    compareDateLike: function (a, b) {
        return MochiKit.Base.compare(a.getTime(), b.getTime());
    },

    /** @id MochiKit.Base.compareArrayLike */
    compareArrayLike: function (a, b) {
        var compare = MochiKit.Base.compare;
        var count = a.length;
        var rval = 0;
        if (count > b.length) {
            rval = 1;
            count = b.length;
        } else if (count < b.length) {
            rval = -1;
        }
        for (var i = 0; i < count; i++) {
            var cmp = compare(a[i], b[i]);
            if (cmp) {
                return cmp;
            }
        }
        return rval;
    },

    /** @id MochiKit.Base.registerRepr */
    registerRepr: function (name, check, wrap, /* optional */override) {
        MochiKit.Base.reprRegistry.register(name, check, wrap, override);
    },

    /** @id MochiKit.Base.repr */
    repr: function (o) {
        if (typeof(o) == "undefined") {
            return "undefined";
        } else if (o === null) {
            return "null";
        }
        try {
            if (typeof(o.__repr__) == 'function') {
                return o.__repr__();
            } else if (typeof(o.repr) == 'function' && o.repr != arguments.callee) {
                return o.repr();
            }
            return MochiKit.Base.reprRegistry.match(o);
        } catch (e) {
            try {
                if (typeof(o.NAME) == 'string' && (
                        o.toString == Function.prototype.toString ||
                        o.toString == Object.prototype.toString
                    )) {
                    return o.NAME;
                }
            } catch (ignore) {
            }
        }
        try {
            var ostring = (o + "");
        } catch (e) {
            return "[" + typeof(o) + "]";
        }
        if (typeof(o) == "function") {
            ostring = ostring.replace(/^\s+/, "").replace(/\s+/g, " ");
            ostring = ostring.replace(/,(\S)/, ", $1");
            var idx = ostring.indexOf("{");
            if (idx != -1) {
                ostring = ostring.substr(0, idx) + "{...}";
            }
        }
        return ostring;
    },

    /** @id MochiKit.Base.reprArrayLike */
    reprArrayLike: function (o) {
        var m = MochiKit.Base;
        return "[" + m.map(m.repr, o).join(", ") + "]";
    },

    /** @id MochiKit.Base.reprString */
    reprString: function (o) {
        return ('"' + o.replace(/(["\\])/g, '\\$1') + '"'
            ).replace(/[\f]/g, "\\f"
            ).replace(/[\b]/g, "\\b"
            ).replace(/[\n]/g, "\\n"
            ).replace(/[\t]/g, "\\t"
            ).replace(/[\v]/g, "\\v"
            ).replace(/[\r]/g, "\\r");
    },

    /** @id MochiKit.Base.reprNumber */
    reprNumber: function (o) {
        return o + "";
    },

    /** @id MochiKit.Base.registerJSON */
    registerJSON: function (name, check, wrap, /* optional */override) {
        MochiKit.Base.jsonRegistry.register(name, check, wrap, override);
    },


    /** @id MochiKit.Base.evalJSON */
    evalJSON: function (jsonText) {
log(ERROR, "going to parse:" + jsonText);
var ret = JSON.parse('(' + jsonText + ')');
log(ERROR, "parsed ok");
return ret;
//        return JSON.parse("(" + jsonText + ")"); // INTENSITY: Had MochiKit.Base._filterJSON(jsonText)... XXX
//        return eval("(" + MochiKit.Base._filterJSON(jsonText) + ")");
    },

    _filterJSON: function (s) {
        var m = s.match(/^\s*\/\*(.*)\*\/\s*$/);
        return (m) ? m[1] : s;
    },

    /** @id MochiKit.Base.serializeJSON */
    serializeJSON: function (o) {
        var objtype = typeof(o);
        if (objtype == "number" || objtype == "boolean") {
            return o + "";
        } else if (o === null) {
            return "null";
        } else if (objtype == "string") {
            var res = "";
            for (var i = 0; i < o.length; i++) {
                var c = o.charAt(i);
                if (c == '\"') {
                    res += '\\"';
                } else if (c == '\\') {
                    res += '\\\\';
                } else if (c == '\b') {
                    res += '\\b';
                } else if (c == '\f') {
                    res += '\\f';
                } else if (c == '\n') {
                    res += '\\n';
                } else if (c == '\r') {
                    res += '\\r';
                } else if (c == '\t') {
                    res += '\\t';
                } else if (o.charCodeAt(i) <= 0x1F) {
                    var hex = o.charCodeAt(i).toString(16);
                    if (hex.length < 2) {
                        hex = '0' + hex;
                    }
                    res += '\\u00' + hex.toUpperCase();
                } else {
                    res += c;
                }
            }
            return '"' + res + '"';
        }
        // recurse
        var me = arguments.callee;
        // short-circuit for objects that support "json" serialization
        // if they return "self" then just pass-through...
        var newObj;
        if(o === undefined) return "[UNDEFINED]"; // INTENSITY: Kripken: Added this!
        if (typeof(o.__json__) == "function") {
            newObj = o.__json__();
            if (o !== newObj) {
                return me(newObj);
            }
        }
        if (typeof(o.json) == "function") {
            newObj = o.json();
            if (o !== newObj) {
                return me(newObj);
            }
        }
        // array
        if (objtype != "function" && typeof(o.length) == "number") {
            var res = [];
            for (var i = 0; i < o.length; i++) {
                var val = me(o[i]);
                if (typeof(val) != "string") {
                    // skip non-serializable values
                    continue;
                }
                res.push(val);
            }
            return "[" + res.join(", ") + "]";
        }
        // look in the registry
        var m = MochiKit.Base;
        try {
            newObj = m.jsonRegistry.match(o);
            if (o !== newObj) {
                return me(newObj);
            }
        } catch (e) {
            if (e != m.NotFound) {
                // something really bad happened
                throw e;
            }
        }
        // undefined is outside of the spec
        if (objtype == "undefined") {
            throw new TypeError("undefined can not be serialized as JSON");
        }
        // it's a function with no adapter, bad
        if (objtype == "function") {
            return null;
        }
        // generic object code path
        res = [];
        for (var k in o) {
            var useKey;
            if (typeof(k) == "number") {
                useKey = '"' + k + '"';
            } else if (typeof(k) == "string") {
                useKey = me(k);
            } else {
                // skip non-string or number keys
                continue;
            }
            val = me(o[k]);
            if (typeof(val) != "string") {
                // skip non-serializable values
                continue;
            }
            res.push(useKey + ":" + val);
        }
        return "{" + res.join(", ") + "}";
    },


    /** @id MochiKit.Base.objEqual */
    objEqual: function (a, b) {
        return (MochiKit.Base.compare(a, b) === 0);
    },

    /** @id MochiKit.Base.arrayEqual */
    arrayEqual: function (self, arr) {
        if (self.length != arr.length) {
            return false;
        }
        return (MochiKit.Base.compare(self, arr) === 0);
    },

    /** @id MochiKit.Base.concat */
    concat: function (/* lst... */) {
        var rval = [];
        var extend = MochiKit.Base.extend;
        for (var i = 0; i < arguments.length; i++) {
            extend(rval, arguments[i]);
        }
        return rval;
    },

    /** @id MochiKit.Base.keyComparator */
    keyComparator: function (key/* ... */) {
        // fast-path for single key comparisons
        var m = MochiKit.Base;
        var compare = m.compare;
        if (arguments.length == 1) {
            return function (a, b) {
                return compare(a[key], b[key]);
            };
        }
        var compareKeys = m.extend(null, arguments);
        return function (a, b) {
            var rval = 0;
            // keep comparing until something is inequal or we run out of
            // keys to compare
            for (var i = 0; (rval === 0) && (i < compareKeys.length); i++) {
                var key = compareKeys[i];
                rval = compare(a[key], b[key]);
            }
            return rval;
        };
    },

    /** @id MochiKit.Base.reverseKeyComparator */
    reverseKeyComparator: function (key) {
        var comparator = MochiKit.Base.keyComparator.apply(this, arguments);
        return function (a, b) {
            return comparator(b, a);
        };
    },

    /** @id MochiKit.Base.partial */
    partial: function (func) {
        var m = MochiKit.Base;
        return m.bind.apply(this, m.extend([func, undefined], arguments, 1));
    },

    /** @id MochiKit.Base.listMinMax */
    listMinMax: function (which, lst) {
        if (lst.length === 0) {
            return null;
        }
        var cur = lst[0];
        var compare = MochiKit.Base.compare;
        for (var i = 1; i < lst.length; i++) {
            var o = lst[i];
            if (compare(o, cur) == which) {
                cur = o;
            }
        }
        return cur;
    },

    /** @id MochiKit.Base.objMax */
    objMax: function (/* obj... */) {
        return MochiKit.Base.listMinMax(1, arguments);
    },

    /** @id MochiKit.Base.objMin */
    objMin: function (/* obj... */) {
        return MochiKit.Base.listMinMax(-1, arguments);
    },

    /** @id MochiKit.Base.findIdentical */
    findIdentical: function (lst, value, start/* = 0 */, /* optional */end) {
        if (typeof(end) == "undefined" || end === null) {
            end = lst.length;
        }
        if (typeof(start) == "undefined" || start === null) {
            start = 0;
        }
        for (var i = start; i < end; i++) {
            if (lst[i] === value) {
                return i;
            }
        }
        return -1;
    },

    /** @id MochiKit.Base.mean */
    mean: function(/* lst... */) {
        /* http://www.nist.gov/dads/HTML/mean.html */
        var sum = 0;

        var m = MochiKit.Base;
        var args = m.extend(null, arguments);
        var count = args.length;

        while (args.length) {
            var o = args.shift();
            if (o && typeof(o) == "object" && typeof(o.length) == "number") {
                count += o.length - 1;
                for (var i = o.length - 1; i >= 0; i--) {
                    sum += o[i];
                }
            } else {
                sum += o;
            }
        }

        if (count <= 0) {
            throw new TypeError('mean() requires at least one argument');
        }

        return sum/count;
    },

    /** @id MochiKit.Base.median */
    median: function(/* lst... */) {
        /* http://www.nist.gov/dads/HTML/median.html */
        var data = MochiKit.Base.flattenArguments(arguments);
        if (data.length === 0) {
            throw new TypeError('median() requires at least one argument');
        }
        data.sort(compare);
        if (data.length % 2 == 0) {
            var upper = data.length / 2;
            return (data[upper] + data[upper - 1]) / 2;
        } else {
            return data[(data.length - 1) / 2];
        }
    },

    /** @id MochiKit.Base.findValue */
    findValue: function (lst, value, start/* = 0 */, /* optional */end) {
        if (typeof(end) == "undefined" || end === null) {
            end = lst.length;
        }
        if (typeof(start) == "undefined" || start === null) {
            start = 0;
        }
        var cmp = MochiKit.Base.compare;
        for (var i = start; i < end; i++) {
            if (cmp(lst[i], value) === 0) {
                return i;
            }
        }
        return -1;
    },

    /** @id MochiKit.Base.nodeWalk */
    nodeWalk: function (node, visitor) {
        var nodes = [node];
        var extend = MochiKit.Base.extend;
        while (nodes.length) {
            var res = visitor(nodes.shift());
            if (res) {
                extend(nodes, res);
            }
        }
    },


    /** @id MochiKit.Base.nameFunctions */
    nameFunctions: function (namespace) {
        var base = namespace.NAME;
        if (typeof(base) == 'undefined') {
            base = '';
        } else {
            base = base + '.';
        }
        for (var name in namespace) {
            var o = namespace[name];
            if (typeof(o) == 'function' && typeof(o.NAME) == 'undefined') {
                try {
                    o.NAME = base + name;
                } catch (e) {
                    // pass
                }
            }
        }
    },


    /** @id MochiKit.Base.queryString */
    queryString: function (names, values) {
        // check to see if names is a string or a DOM element, and if
        // MochiKit.DOM is available.  If so, drop it like it's a form
        // Ugliest conditional in MochiKit?  Probably!
        if (typeof(MochiKit.DOM) != "undefined" && arguments.length == 1
            && (typeof(names) == "string" || (
                typeof(names.nodeType) != "undefined" && names.nodeType > 0
            ))
        ) {
            var kv = MochiKit.DOM.formContents(names);
            names = kv[0];
            values = kv[1];
        } else if (arguments.length == 1) {
            // Allow the return value of formContents to be passed directly
            if (typeof(names.length) == "number" && names.length == 2) {
                return arguments.callee(names[0], names[1]);
            }
            var o = names;
            names = [];
            values = [];
            for (var k in o) {
                var v = o[k];
                if (typeof(v) == "function") {
                    continue;
                } else if (MochiKit.Base.isArrayLike(v)){
                    for (var i = 0; i < v.length; i++) {
                        names.push(k);
                        values.push(v[i]);
                    }
                } else {
                    names.push(k);
                    values.push(v);
                }
            }
        }
        var rval = [];
        var len = Math.min(names.length, values.length);
        var urlEncode = MochiKit.Base.urlEncode;
        for (var i = 0; i < len; i++) {
            v = values[i];
            if (typeof(v) != 'undefined' && v !== null) {
                rval.push(urlEncode(names[i]) + "=" + urlEncode(v));
            }
        }
        return rval.join("&");
    },


    /** @id MochiKit.Base.parseQueryString */
    parseQueryString: function (encodedString, useArrays) {
        // strip a leading '?' from the encoded string
        var qstr = (encodedString.charAt(0) == "?")
            ? encodedString.substring(1)
            : encodedString;
        var pairs = qstr.replace(/\+/g, "%20").split(/\&amp\;|\&\#38\;|\&#x26;|\&/);
        var o = {};
        var decode;
        if (typeof(decodeURIComponent) != "undefined") {
            decode = decodeURIComponent;
        } else {
            decode = unescape;
        }
        if (useArrays) {
            for (var i = 0; i < pairs.length; i++) {
                var pair = pairs[i].split("=");
                var name = decode(pair.shift());
                if (!name) {
                    continue;
                }
                var arr = o[name];
                if (!(arr instanceof Array)) {
                    arr = [];
                    o[name] = arr;
                }
                arr.push(decode(pair.join("=")));
            }
        } else {
            for (var i = 0; i < pairs.length; i++) {
                pair = pairs[i].split("=");
                var name = pair.shift();
                if (!name) {
                    continue;
                }
                o[decode(name)] = decode(pair.join("="));
            }
        }
        return o;
    }
});

/** @id MochiKit.Base.AdapterRegistry */
MochiKit.Base.AdapterRegistry = function () {
    this.pairs = [];
};

MochiKit.Base.AdapterRegistry.prototype = {
    /** @id MochiKit.Base.AdapterRegistry.prototype.register */
    register: function (name, check, wrap, /* optional */ override) {
        if (override) {
            this.pairs.unshift([name, check, wrap]);
        } else {
            this.pairs.push([name, check, wrap]);
        }
    },

    /** @id MochiKit.Base.AdapterRegistry.prototype.match */
    match: function (/* ... */) {
        for (var i = 0; i < this.pairs.length; i++) {
            var pair = this.pairs[i];
            if (pair[1].apply(this, arguments)) {
                return pair[2].apply(this, arguments);
            }
        }
        throw MochiKit.Base.NotFound;
    },

    /** @id MochiKit.Base.AdapterRegistry.prototype.unregister */
    unregister: function (name) {
        for (var i = 0; i < this.pairs.length; i++) {
            var pair = this.pairs[i];
            if (pair[0] == name) {
                this.pairs.splice(i, 1);
                return true;
            }
        }
        return false;
    }
};

MochiKit.Base._exportSymbols = function (globals, module) {
    if (MochiKit.__export__ === false || module.__export__ === false) {
        return;
    }
    for (var k in module) {
        var v = module[k];
        if (v != null) {
            var okName = (k[0] !== "_" && k !== "toString");
            if (v.__export__ === true || (v.__export__ !== false && okName)) {
                globals[k] = module[k];
            }
        }
    }
};

/**
 * Creates a deprecated function alias in the specified module. The
 * deprecated function will forward all calls and arguments to a
 * target function, while also logging a debug message on the first
 * call (if MochiKit.Logging is loaded). The destination function may
 * be located in another module, which must be loaded, or an
 * exception will be thrown.
 *
 * @param {Object/String} module the source module or module name
 *            (e.g. 'DOM' or 'MochiKit.DOM')
 * @param {String} name the deprecated function name (e.g. 'getStyle')
 * @param {String} target the fully qualified name of the target
 *            function (e.g. 'MochiKit.Style.getStyle')
 * @param {String} version the first version when the source function
 *            was deprecated (e.g. '1.4')
 * @param {Boolean} [exportable] the exportable function flag,
 *            defaults to false
 */
MochiKit.Base._deprecated = function (module, name, target, version, exportable) {
    if (typeof(module) === 'string') {
        if (module.indexOf('MochiKit.') === 0) {
            module = module.substring(9);
        }
        module = MochiKit[module];
    }
    var targetModule = target.split('.')[1];
    var targetName = target.split('.')[2];
    var func = function () {
        var self = arguments.callee;
        var msg = module.NAME + '.' + name + ' is deprecated since version ' +
                  version + '. Use ' + target + ' instead.';
        if (self.logged !== true) {
            self.logged = true;
            if (MochiKit.Logging) {
                MochiKit.Logging.logDebug(msg);
            } else if (console && console.log) {
                console.log(msg);
            }
        }
        if (!MochiKit[targetModule]) {
            throw new Error(msg);
        }
        return MochiKit[targetModule][targetName].apply(this, arguments);
    };
    func.__export__ = (exportable === true);
    module[name] = func;
}

MochiKit.Base.__new__ = function () {
    var m = this;

    /** @id MochiKit.Base.noop */
    m.noop = m.operator.identity;

    // Backwards compat
    m._deprecated(m, 'forward', 'MochiKit.Base.forwardCall', '1.3');
    m._deprecated(m, 'find', 'MochiKit.Base.findValue', '1.3');

    if (typeof(encodeURIComponent) != "undefined") {
        /** @id MochiKit.Base.urlEncode */
        m.urlEncode = function (unencoded) {
            return encodeURIComponent(unencoded).replace(/\'/g, '%27');
        };
    } else {
        m.urlEncode = function (unencoded) {
            return escape(unencoded
                ).replace(/\+/g, '%2B'
                ).replace(/\"/g,'%22'
                ).rval.replace(/\'/g, '%27');
        };
    }

    /** @id MochiKit.Base.NamedError */
    m.NamedError = function (name) {
        this.message = name;
        this.name = name;
    };
    m.NamedError.prototype = new Error();
    m.update(m.NamedError.prototype, {
        repr: function () {
            if (this.message && this.message != this.name) {
                return this.name + "(" + m.repr(this.message) + ")";
            } else {
                return this.name + "()";
            }
        },
        toString: m.forwardCall("repr")
    });

    /** @id MochiKit.Base.NotFound */
    m.NotFound = new m.NamedError("MochiKit.Base.NotFound");


    /** @id MochiKit.Base.listMax */
    m.listMax = m.partial(m.listMinMax, 1);
    /** @id MochiKit.Base.listMin */
    m.listMin = m.partial(m.listMinMax, -1);

    /** @id MochiKit.Base.isCallable */
    m.isCallable = m.typeMatcher('function');
    /** @id MochiKit.Base.isUndefined */
    m.isUndefined = m.typeMatcher('undefined');

    /** @id MochiKit.Base.merge */
    m.merge = m.partial(m.update, null);
    /** @id MochiKit.Base.zip */
    m.zip = m.partial(m.map, null);

    /** @id MochiKit.Base.average */
    m.average = m.mean;

    /** @id MochiKit.Base.comparatorRegistry */
    m.comparatorRegistry = new m.AdapterRegistry();
    m.registerComparator("dateLike", m.isDateLike, m.compareDateLike);
    m.registerComparator("arrayLike", m.isArrayLike, m.compareArrayLike);

    /** @id MochiKit.Base.reprRegistry */
    m.reprRegistry = new m.AdapterRegistry();
    m.registerRepr("arrayLike", m.isArrayLike, m.reprArrayLike);
    m.registerRepr("string", m.typeMatcher("string"), m.reprString);
    m.registerRepr("numbers", m.typeMatcher("number", "boolean"), m.reprNumber);

    /** @id MochiKit.Base.jsonRegistry */
    m.jsonRegistry = new m.AdapterRegistry();

    m.nameFunctions(this);

};

MochiKit.Base.__new__();

//
// XXX: Internet Explorer blows
//
if (MochiKit.__export__) {
    compare = MochiKit.Base.compare;
    compose = MochiKit.Base.compose;
    serializeJSON = MochiKit.Base.serializeJSON;
    mean = MochiKit.Base.mean;
    median = MochiKit.Base.median;
}

MochiKit.Base._exportSymbols(this, MochiKit.Base);




///////////////// DateTime




/***

MochiKit.DateTime 1.5

See <http://mochikit.com/> for documentation, downloads, license, etc.

(c) 2005 Bob Ippolito.  All rights Reserved.

***/

MochiKit.Base._module('DateTime', '1.5', ['Base']);

/** @id MochiKit.DateTime.isoDate */
MochiKit.DateTime.isoDate = function (str) {
    str = str + "";
    if (typeof(str) != "string" || str.length === 0) {
        return null;
    }
    var iso = str.split('-');
    if (iso.length === 0) {
        return null;
    }
	var date = new Date(iso[0], iso[1] - 1, iso[2]);
	date.setFullYear(iso[0]);
	date.setMonth(iso[1] - 1);
	date.setDate(iso[2]);
    return date;
};

MochiKit.DateTime._isoRegexp = /(\d{4,})(?:-(\d{1,2})(?:-(\d{1,2})(?:[T ](\d{1,2}):(\d{1,2})(?::(\d{1,2})(?:\.(\d+))?)?(?:(Z)|([+-])(\d{1,2})(?::(\d{1,2}))?)?)?)?)?/;

/** @id MochiKit.DateTime.isoTimestamp */
MochiKit.DateTime.isoTimestamp = function (str) {
    str = str + "";
    if (typeof(str) != "string" || str.length === 0) {
        return null;
    }
    var res = str.match(MochiKit.DateTime._isoRegexp);
    if (typeof(res) == "undefined" || res === null) {
        return null;
    }
    var year, month, day, hour, min, sec, msec;
    year = parseInt(res[1], 10);
    if (typeof(res[2]) == "undefined" || res[2] === '') {
        return new Date(year);
    }
    month = parseInt(res[2], 10) - 1;
    day = parseInt(res[3], 10);
    if (typeof(res[4]) == "undefined" || res[4] === '') {
        return new Date(year, month, day);
    }
    hour = parseInt(res[4], 10);
    min = parseInt(res[5], 10);
    sec = (typeof(res[6]) != "undefined" && res[6] !== '') ? parseInt(res[6], 10) : 0;
    if (typeof(res[7]) != "undefined" && res[7] !== '') {
        msec = Math.round(1000.0 * parseFloat("0." + res[7]));
    } else {
        msec = 0;
    }
    if ((typeof(res[8]) == "undefined" || res[8] === '') && (typeof(res[9]) == "undefined" || res[9] === '')) {
        return new Date(year, month, day, hour, min, sec, msec);
    }
    var ofs;
    if (typeof(res[9]) != "undefined" && res[9] !== '') {
        ofs = parseInt(res[10], 10) * 3600000;
        if (typeof(res[11]) != "undefined" && res[11] !== '') {
            ofs += parseInt(res[11], 10) * 60000;
        }
        if (res[9] == "-") {
            ofs = -ofs;
        }
    } else {
        ofs = 0;
    }
    return new Date(Date.UTC(year, month, day, hour, min, sec, msec) - ofs);
};

/** @id MochiKit.DateTime.toISOTime */
MochiKit.DateTime.toISOTime = function (date, realISO/* = false */) {
    if (typeof(date) == "undefined" || date === null) {
        return null;
    }
    var hh = date.getHours();
    var mm = date.getMinutes();
    var ss = date.getSeconds();
    var lst = [
        ((realISO && (hh < 10)) ? "0" + hh : hh),
        ((mm < 10) ? "0" + mm : mm),
        ((ss < 10) ? "0" + ss : ss)
    ];
    return lst.join(":");
};

/** @id MochiKit.DateTime.toISOTimeStamp */
MochiKit.DateTime.toISOTimestamp = function (date, realISO/* = false*/) {
    if (typeof(date) == "undefined" || date === null) {
        return null;
    }
    var sep = realISO ? "T" : " ";
    var foot = realISO ? "Z" : "";
    if (realISO) {
        date = new Date(date.getTime() + (date.getTimezoneOffset() * 60000));
    }
    return MochiKit.DateTime.toISODate(date) + sep + MochiKit.DateTime.toISOTime(date, realISO) + foot;
};

/** @id MochiKit.DateTime.toISODate */
MochiKit.DateTime.toISODate = function (date) {
    if (typeof(date) == "undefined" || date === null) {
        return null;
    }
    var _padTwo = MochiKit.DateTime._padTwo;
	var _padFour = MochiKit.DateTime._padFour;
    return [
        _padFour(date.getFullYear()),
        _padTwo(date.getMonth() + 1),
        _padTwo(date.getDate())
    ].join("-");
};

/** @id MochiKit.DateTime.americanDate */
MochiKit.DateTime.americanDate = function (d) {
    d = d + "";
    if (typeof(d) != "string" || d.length === 0) {
        return null;
    }
    var a = d.split('/');
    return new Date(a[2], a[0] - 1, a[1]);
};

MochiKit.DateTime._padTwo = function (n) {
    return (n > 9) ? n : "0" + n;
};

MochiKit.DateTime._padFour = function(n) {
	switch(n.toString().length) {
		case 1: return "000" + n; break;
		case 2: return "00" + n; break;
		case 3: return "0" + n; break;
		case 4:
		default:
			return n;
	}
};

/** @id MochiKit.DateTime.toPaddedAmericanDate */
MochiKit.DateTime.toPaddedAmericanDate = function (d) {
    if (typeof(d) == "undefined" || d === null) {
        return null;
    }
    var _padTwo = MochiKit.DateTime._padTwo;
    return [
        _padTwo(d.getMonth() + 1),
        _padTwo(d.getDate()),
        d.getFullYear()
    ].join('/');
};

/** @id MochiKit.DateTime.toAmericanDate */
MochiKit.DateTime.toAmericanDate = function (d) {
    if (typeof(d) == "undefined" || d === null) {
        return null;
    }
    return [d.getMonth() + 1, d.getDate(), d.getFullYear()].join('/');
};

MochiKit.DateTime.__new__ = function () {
    MochiKit.Base.nameFunctions(this);
};

MochiKit.DateTime.__new__();

MochiKit.Base._exportSymbols(this, MochiKit.DateTime);



/////////////////////// Format


/***

MochiKit.Format 1.5

See <http://mochikit.com/> for documentation, downloads, license, etc.

(c) 2005 Bob Ippolito.  All rights Reserved.

***/

MochiKit.Base._module('Format', '1.5', ['Base']);

MochiKit.Format._numberFormatter = function (placeholder, header, footer, locale, isPercent, precision, leadingZeros, separatorAt, trailingZeros) {
    return function (num) {
        num = parseFloat(num);
        if (typeof(num) == "undefined" || num === null || isNaN(num)) {
            return placeholder;
        }
        var curheader = header;
        var curfooter = footer;
        if (num < 0) {
            num = -num;
        } else {
            curheader = curheader.replace(/-/, "");
        }
        var me = arguments.callee;
        var fmt = MochiKit.Format.formatLocale(locale);
        if (isPercent) {
            num = num * 100.0;
            curfooter = fmt.percent + curfooter;
        }
        num = MochiKit.Format.roundToFixed(num, precision);
        var parts = num.split(/\./);
        var whole = parts[0];
        var frac = (parts.length == 1) ? "" : parts[1];
        var res = "";
        while (whole.length < leadingZeros) {
            whole = "0" + whole;
        }
        if (separatorAt) {
            while (whole.length > separatorAt) {
                var i = whole.length - separatorAt;
                //res = res + fmt.separator + whole.substring(i, whole.length);
                res = fmt.separator + whole.substring(i, whole.length) + res;
                whole = whole.substring(0, i);
            }
        }
        res = whole + res;
        if (precision > 0) {
            while (frac.length < trailingZeros) {
                frac = frac + "0";
            }
            res = res + fmt.decimal + frac;
        }
        return curheader + res + curfooter;
    };
};

/** @id MochiKit.Format.numberFormatter */
MochiKit.Format.numberFormatter = function (pattern, placeholder/* = "" */, locale/* = "default" */) {
    // http://java.sun.com/docs/books/tutorial/i18n/format/numberpattern.html
    // | 0 | leading or trailing zeros
    // | # | just the number
    // | , | separator
    // | . | decimal separator
    // | % | Multiply by 100 and format as percent
    if (typeof(placeholder) == "undefined") {
        placeholder = "";
    }
    var match = pattern.match(/((?:[0#]+,)?[0#]+)(?:\.([0#]+))?(%)?/);
    if (!match) {
        throw TypeError("Invalid pattern");
    }
    var header = pattern.substr(0, match.index);
    var footer = pattern.substr(match.index + match[0].length);
    if (header.search(/-/) == -1) {
        header = header + "-";
    }
    var whole = match[1];
    var frac = (typeof(match[2]) == "string" && match[2] != "") ? match[2] : "";
    var isPercent = (typeof(match[3]) == "string" && match[3] != "");
    var tmp = whole.split(/,/);
    var separatorAt;
    if (typeof(locale) == "undefined") {
        locale = "default";
    }
    if (tmp.length == 1) {
        separatorAt = null;
    } else {
        separatorAt = tmp[1].length;
    }
    var leadingZeros = whole.length - whole.replace(/0/g, "").length;
    var trailingZeros = frac.length - frac.replace(/0/g, "").length;
    var precision = frac.length;
    var rval = MochiKit.Format._numberFormatter(
        placeholder, header, footer, locale, isPercent, precision,
        leadingZeros, separatorAt, trailingZeros
    );
    var m = MochiKit.Base;
    if (m) {
        var fn = arguments.callee;
        var args = m.concat(arguments);
        rval.repr = function () {
            return [
                self.NAME,
                "(",
                map(m.repr, args).join(", "),
                ")"
            ].join("");
        };
    }
    return rval;
};

/** @id MochiKit.Format.formatLocale */
MochiKit.Format.formatLocale = function (locale) {
    if (typeof(locale) == "undefined" || locale === null) {
        locale = "default";
    }
    if (typeof(locale) == "string") {
        var rval = MochiKit.Format.LOCALE[locale];
        if (typeof(rval) == "string") {
            rval = arguments.callee(rval);
            MochiKit.Format.LOCALE[locale] = rval;
        }
        return rval;
    } else {
        return locale;
    }
};

/** @id MochiKit.Format.twoDigitAverage */
MochiKit.Format.twoDigitAverage = function (numerator, denominator) {
    if (denominator) {
        var res = numerator / denominator;
        if (!isNaN(res)) {
            return MochiKit.Format.twoDigitFloat(res);
        }
    }
    return "0";
};

/** @id MochiKit.Format.twoDigitFloat */
MochiKit.Format.twoDigitFloat = function (aNumber) {
    var res = roundToFixed(aNumber, 2);
    if (res.indexOf(".00") > 0) {
        return res.substring(0, res.length - 3);
    } else if (res.charAt(res.length - 1) == "0") {
        return res.substring(0, res.length - 1);
    } else {
        return res;
    }
};

/** @id MochiKit.Format.lstrip */
MochiKit.Format.lstrip = function (str, /* optional */chars) {
    str = str + "";
    if (typeof(str) != "string") {
        return null;
    }
    if (!chars) {
        return str.replace(/^\s+/, "");
    } else {
        return str.replace(new RegExp("^[" + chars + "]+"), "");
    }
};

/** @id MochiKit.Format.rstrip */
MochiKit.Format.rstrip = function (str, /* optional */chars) {
    str = str + "";
    if (typeof(str) != "string") {
        return null;
    }
    if (!chars) {
        return str.replace(/\s+$/, "");
    } else {
        return str.replace(new RegExp("[" + chars + "]+$"), "");
    }
};

/** @id MochiKit.Format.strip */
MochiKit.Format.strip = function (str, /* optional */chars) {
    var self = MochiKit.Format;
    return self.rstrip(self.lstrip(str, chars), chars);
};

/** @id MochiKit.Format.truncToFixed */
MochiKit.Format.truncToFixed = function (aNumber, precision) {
    var fixed = MochiKit.Format._numberToFixed(aNumber, precision);
    var fracPos = fixed.indexOf(".");
    if (fracPos > 0 && fracPos + precision + 1 < fixed.length) {
        fixed = fixed.substring(0, fracPos + precision + 1);
        fixed = MochiKit.Format._shiftNumber(fixed, 0);
    }
    return fixed;
}

/** @id MochiKit.Format.roundToFixed */
MochiKit.Format.roundToFixed = function (aNumber, precision) {
    var fixed = MochiKit.Format._numberToFixed(aNumber, precision);
    var fracPos = fixed.indexOf(".");
    if (fracPos > 0 && fracPos + precision + 1 < fixed.length) {
        var str = MochiKit.Format._shiftNumber(fixed, precision);
        str = MochiKit.Format._numberToFixed(Math.round(parseFloat(str)), 0);
        fixed = MochiKit.Format._shiftNumber(str, -precision);
    }
    return fixed;
}

/**
 * Converts a number to a fixed format string. This function handles
 * conversion of exponents by shifting the decimal point to the left
 * or the right. It also guarantees a specified minimum number of
 * fractional digits (but no maximum).
 *
 * @param {Number} aNumber the number to convert
 * @param {Number} precision the minimum number of decimal digits
 *
 * @return {String} the fixed format number string
 */
MochiKit.Format._numberToFixed = function (aNumber, precision) {
    var str = aNumber.toString();
    var parts = str.split(/[eE]/);
    var exp = (parts.length === 1) ? 0 : parseInt(parts[1]) || 0;
    var fixed = MochiKit.Format._shiftNumber(parts[0], exp);
    parts = fixed.split(/\./);
    var whole = parts[0];
    var frac = (parts.length === 1) ? "" : parts[1];
    while (frac.length < precision) {
        frac += "0";
    }
    if (frac.length > 0) {
        return whole + "." + frac;
    } else {
        return whole;
    }
}

/**
 * Shifts the decimal dot location in a fixed format number string.
 * This function handles negative values and will add and remove
 * leading and trailing zeros as needed.
 *
 * @param {String} num the fixed format number string
 * @param {Number} exp the base-10 exponent to apply
 *
 * @return {String} the new fixed format number string
 */
MochiKit.Format._shiftNumber = function (num, exp) {
    var pos = num.indexOf(".");
    if (pos < 0) {
        pos = num.length;
    } else {
        num = num.substring(0, pos) + num.substring(pos + 1);
    }
    pos += exp;
    while (pos <= 0 || (pos <= 1 && num.charAt(0) === "-")) {
        if (num.charAt(0) === "-") {
            num = "-0" + num.substring(1);
        } else {
            num = "0" + num;
        }
        pos++;
    }
    while (pos > num.length) {
        num += "0";
    }
    if (pos < num.length) {
        num = num.substring(0, pos) + "." + num.substring(pos);
    }
    while (/^0[^.]/.test(num)) {
        num = num.substring(1);
    }
    while (/^-0[^.]/.test(num)) {
        num = "-" + num.substring(2);
    }
    return num;
}

/** @id MochiKit.Format.percentFormat */
MochiKit.Format.percentFormat = function (aNumber) {
    return MochiKit.Format.twoDigitFloat(100 * aNumber) + '%';
};

MochiKit.Format.LOCALE = {
    en_US: {separator: ",", decimal: ".", percent: "%"},
    de_DE: {separator: ".", decimal: ",", percent: "%"},
    pt_BR: {separator: ".", decimal: ",", percent: "%"},
    fr_FR: {separator: " ", decimal: ",", percent: "%"},
    "default": "en_US",
    __export__: false
};

MochiKit.Format.__new__ = function () {
    MochiKit.Base.nameFunctions(this);
    var base = this.NAME + ".";
    var k, v, o;
    for (k in this.LOCALE) {
        o = this.LOCALE[k];
        if (typeof(o) == "object") {
            o.repr = function () { return this.NAME; };
            o.NAME = base + "LOCALE." + k;
        }
    }
};

MochiKit.Format.__new__();

MochiKit.Base._exportSymbols(this, MochiKit.Format);



///////////////////////// Text



/***

MochiKit.Text 1.5

See <http://mochikit.com/> for documentation, downloads, license, etc.

(c) 2008 Per Cederberg.  All rights Reserved.

***/

MochiKit.Base._module('Text', '1.5', ['Base', 'Format']);

/**
 * Checks if a text string starts with the specified substring. If
 * either of the two strings is null, false will be returned.
 *
 * @param {String} substr the substring to search for
 * @param {String} str the string to search in
 *
 * @return {Boolean} true if the string starts with the substring, or
 *         false otherwise
 */
MochiKit.Text.startsWith = function (substr, str) {
    return str != null && substr != null && str.indexOf(substr) == 0;
}

/**
 * Checks if a text string ends with the specified substring. If
 * either of the two strings is null, false will be returned.
 *
 * @param {String} substr the substring to search for
 * @param {String} str the string to search in
 *
 * @return {Boolean} true if the string ends with the substring, or
 *         false otherwise
 */
MochiKit.Text.endsWith = function (substr, str) {
    return str != null && substr != null &&
           str.lastIndexOf(substr) == Math.max(str.length - substr.length, 0);
}

/**
 * Checks if a text string contains the specified substring. If
 * either of the two strings is null, false will be returned.
 *
 * @param {String} substr the substring to search for
 * @param {String} str the string to search in
 *
 * @return {Boolean} true if the string contains the substring, or
 *         false otherwise
 */
MochiKit.Text.contains = function (substr, str) {
    return str != null && substr != null && str.indexOf(substr) >= 0;
}

/**
 * Adds a character to the left-hand side of a string until it
 * reaches the specified minimum length.
 *
 * @param {String} str the string to process
 * @param {Number} minLength the requested minimum length
 * @param {String} fillChar the padding character to add, defaults
 *            to a space
 *
 * @return {String} the padded string
 */
MochiKit.Text.padLeft = function (str, minLength, fillChar) {
    str = str || "";
    fillChar = fillChar || " ";
    while (str.length < minLength) {
        str = fillChar + str;
    }
    return str;
}

/**
 * Adds a character to the right-hand side of a string until it
 * reaches the specified minimum length.
 *
 * @param {String} str the string to process
 * @param {Number} minLength the requested minimum length
 * @param {String} fillChar the padding character to add, defaults
 *            to a space
 *
 * @return {String} the padded string
 */
MochiKit.Text.padRight = function (str, minLength, fillChar) {
    str = str || "";
    fillChar = fillChar || " ";
    while (str.length < minLength) {
        str += fillChar;
    }
    return str;
}

/**
 * Returns a truncated copy of a string. If the string is shorter
 * than the specified maximum length, the object will be returned
 * unmodified. If an optional tail string is specified, additional
 * elements will be removed in order to accomodate the tail (that
 * will be appended). This function also works on arrays.
 *
 * @param {String} str the string to truncate
 * @param {Number} maxLength the maximum length
 * @param {String} [tail] the tail to append on truncation
 *
 * @return {String} the truncated string
 */
MochiKit.Text.truncate = function (str, maxLength, tail) {
    if (str == null || str.length <= maxLength || maxLength < 0) {
        return str;
    } else if (tail != null) {
        str = str.slice(0, Math.max(0, maxLength - tail.length));
        if (typeof(str) == "string") {
            return str + tail;
        } else {
            return MochiKit.Base.extend(str, tail);
        }
    } else {
        return str.slice(0, maxLength);
    }
}

/**
 * Splits a text string using separator as the split point
 * If max is given, at most max splits are done, giving at most
 * max + 1 elements in the returned list.
 *
 * @param {String} str the string to split
 * @param {String} [separator] the separator character to use,
 *            defaults to newline
 * @param {Int} [max] the maximum number of parts to return
 * @return {Array} an array of parts of the string
 */
MochiKit.Text.split = function (str, separator, max) {
    if (str == null || str.length == 0) {
        return str;
    }
    separator = separator || '\n'
    var bits = str.split(separator);
    if ((typeof(max) == "undefined") || max >= bits.length-1) {
        return bits;
    }
    bits.splice(max, bits.length, bits.slice(max, bits.length).join(separator))
    return bits;
}

/**
 * Splits a text string using separator as the split point
 * If max is given, at most max splits are done,
 * using splits from the right
 *
 * @param {String} str the string to split
 * @param {String} [separator] the separator character to use,
 *            defaults to newline
 * @param {Int} [max] the maximum number of parts to return
 * @return {Array} an array of parts of the string
 */
MochiKit.Text.rsplit = function (str, separator, max) {
    if (str == null || str.length == 0) {
        return str;
    }
    separator = separator || '\n'
    var bits = str.split(separator);
    if ((typeof(max) == "undefined") || max >= bits.length-1){
	return bits;
    }
    bits.splice(0, bits.length-max, bits.slice(0, bits.length-max).join(separator))
    return bits;
}

/**
 * Creates a formatter function for the specified formatter pattern
 * and locale. The returned function takes as many arguments as the
 * formatter pattern requires. See separate documentation for
 * information about the formatter pattern syntax.
 *
 * @param {String} pattern the formatter pattern string
 * @param {Object} [locale] the locale to use, defaults to
 *            LOCALE.en_US
 *
 * @return {Function} the formatter function created
 *
 * @throws FormatPatternError if the format pattern was invalid
 */
MochiKit.Text.formatter = function (pattern, locale) {
    if (typeof(locale) == "undefined") {
        locale = MochiKit.Format.formatLocale();
    } else if (typeof(locale) == "string") {
        locale = MochiKit.Format.formatLocale(locale);
    }
    var parts = MochiKit.Text._parsePattern(pattern);
    return function() {
        var values = MochiKit.Base.extend([], arguments);
        var res = [];
        for (var i = 0; i < parts.length; i++) {
            if (typeof(parts[i]) == "string") {
                res.push(parts[i]);
            } else {
                res.push(MochiKit.Text.formatValue(parts[i], values, locale));
            }
        }
        return res.join("");
    }
}

/**
 * Formats the specified arguments according to a formatter pattern.
 * See separate documentation for information about the formatter
 * pattern syntax.
 *
 * @param {String} pattern the formatter pattern string
 * @param {Object} [...] the optional values to format
 *
 * @return {String} the formatted output string
 *
 * @throws FormatPatternError if the format pattern was invalid
 */
MochiKit.Text.format = function (pattern/*, ...*/) {
    var func = MochiKit.Text.formatter(pattern);
    return func.apply(this, MochiKit.Base.extend([], arguments, 1));
}

/**
 * Format a value with the specified format specifier.
 *
 * @param {String/Object} spec the format specifier string or parsed
 *            format specifier object
 * @param {Object} value the value to format
 * @param {Object} [locale] the locale to use, defaults to
 *            LOCALE.en_US
 *
 * @return {String} the formatted output string
 *
 * @throws FormatPatternError if the format pattern was invalid
 */
MochiKit.Text.formatValue = function (spec, value, locale) {
    var self = MochiKit.Text;
    if (typeof(spec) === "string") {
        spec = self._parseFormatFlags(spec, 0, spec.length - 1);
    }
    for (var i = 0; spec.path != null && i < spec.path.length; i++) {
        if (value != null) {
            value = value[spec.path[i]];
        }
    }
    if (typeof(locale) == "undefined") {
        locale = MochiKit.Format.formatLocale();
    } else if (typeof(locale) == "string") {
        locale = MochiKit.Format.formatLocale(locale);
    }
    var str = "";
    if (spec.numeric) {
        if (typeof(value) != "number" || isNaN(value)) {
            str = "";
        } else if (value === Number.POSITIVE_INFINITY) {
            str = "\u221e";
        } else if (value === Number.NEGATIVE_INFINITY) {
            str = "-\u221e";
        } else {
            var sign = (spec.sign === "-") ? "" : spec.sign;
            sign = (value < 0) ? "-" : sign;
            value = Math.abs(value);
            if (spec.format === "%") {
                str = self._truncToPercent(value, spec.precision);
            } else if (spec.format === "d") {
                str = MochiKit.Format.roundToFixed(value, 0);
            } else if (spec.radix != 10) {
                str = Math.floor(value).toString(spec.radix);
                if (spec.format === "x") {
                    str = str.toLowerCase();
                } else if (spec.format === "X") {
                    str = str.toUpperCase();
                }
            } else if (spec.precision >= 0) {
                str = MochiKit.Format.roundToFixed(value, spec.precision);
            } else {
                str = value.toString();
            }
            if (spec.padding === "0" && spec.format === "%") {
                str = self.padLeft(str, spec.width - sign.length - 1, "0");
            } else if (spec.padding == "0") {
                str = self.padLeft(str, spec.width - sign.length, "0");
            }
            str = self._localizeNumber(str, locale, spec.grouping);
            str = sign + str;
        }
        if (str !== "" && spec.format === "%") {
            str = str + locale.percent;
        }
    } else {
        if (spec.format == "r") {
            str = MochiKit.Base.repr(value);
        } else {
            str = (value == null) ? "null" : value.toString();
        }
        str = self.truncate(str, spec.precision);
    }
    if (spec.align == "<") {
        str = self.padRight(str, spec.width);
    } else {
        str = self.padLeft(str, spec.width);
    }
    return str;
}

/**
 * Adjust an already formatted numeric string for locale-specific
 * grouping and decimal separators. The grouping is optional and
 * will attempt to keep the number string length intact by removing
 * padded zeros (if possible).
 *
 * @param {String} num the formatted number string
 * @param {Object} locale the formatting locale to use
 * @param {Boolean} grouping the grouping flag
 *
 * @return {String} the localized number string
 */
MochiKit.Text._localizeNumber = function (num, locale, grouping) {
    var parts = num.split(/\./);
    var whole = parts[0];
    var frac = (parts.length == 1) ? "" : parts[1];
    var res = (frac.length > 0) ? locale.decimal : "";
    while (grouping && frac.length > 3) {
        res = res + frac.substring(0, 3) + locale.separator;
        frac = frac.substring(3);
        if (whole.charAt(0) == "0") {
            whole = whole.substring(1);
        }
    }
    if (frac.length > 0) {
        res += frac;
    }
    while (grouping && whole.length > 3) {
        var pos = whole.length - 3;
        res = locale.separator + whole.substring(pos) + res;
        whole = whole.substring((whole.charAt(0) == "0") ? 1 : 0, pos);
    }
    return whole + res;
}

/**
 * Parses a format pattern and returns an array of constant strings
 * and format info objects.
 *
 * @param {String} pattern the format pattern to analyze
 *
 * @return {Array} an array of strings and format info objects
 *
 * @throws FormatPatternError if the format pattern was invalid
 */
MochiKit.Text._parsePattern = function (pattern) {
    var self = MochiKit.Text;
    var parts = [];
    var start = 0;
    var pos = 0;
    for (pos = 0; pos < pattern.length; pos++) {
        if (pattern.charAt(pos) == "{") {
            if (pos + 1 >= pattern.length) {
                var msg = "unescaped { char, should be escaped as {{";
                throw new self.FormatPatternError(pattern, pos, msg);
            } else if (pattern.charAt(pos + 1) == "{") {
                parts.push(pattern.substring(start, pos + 1));
                start = pos + 2;
                pos++;
            } else {
                if (start < pos) {
                    parts.push(pattern.substring(start, pos));
                }
                start = pattern.indexOf("}", pos) + 1;
                if (start <= 0) {
                    var msg = "unmatched { char, not followed by a } char";
                    throw new self.FormatPatternError(pattern, pos, msg);
                }
                parts.push(self._parseFormat(pattern, pos + 1, start - 1));
                pos = start - 1;
            }
        } else if (pattern.charAt(pos) == "}") {
            if (pos + 1 >= pattern.length || pattern.charAt(pos + 1) != "}") {
                var msg = "unescaped } char, should be escaped as }}";
                throw new self.FormatPatternError(pattern, pos, msg);
            }
            parts.push(pattern.substring(start, pos + 1));
            start = pos + 2;
            pos++;
        }
    }
    if (start < pos) {
        parts.push(pattern.substring(start, pos));
    }
    return parts;
}

/**
 * Parses a format instruction and returns a format info object.
 *
 * @param {String} pattern the format pattern string
 * @param {Number} startPos the first index of the format instruction
 * @param {Number} endPos the last index of the format instruction
 *
 * @return {Object} the format info object
 *
 * @throws FormatPatternError if the format pattern was invalid
 */
MochiKit.Text._parseFormat = function (pattern, startPos, endPos) {
    var self = MochiKit.Text;
    var text = pattern.substring(startPos, endPos);
    var info;
    var pos = text.indexOf(":");
    if (pos == 0) {
        info = self._parseFormatFlags(pattern, startPos + 1, endPos);
        info.path = [];
    } else if (pos > 0) {
        info = self._parseFormatFlags(pattern, startPos + pos + 1, endPos);
        info.path = text.substring(0, pos).split(".");
    } else {
        info = self._parseFormatFlags(pattern, endPos, endPos);
        info.path = text.split(".");
    }
    var DIGITS = /^\d+$/;
    for (var i = 0; i < info.path.length; i++) {
        var e = info.path[i];
        // TODO: replace with MochiKit.Format.strip?
        e = e.replace(/^\s+/, "").replace(/\s+$/, "");
        if (e == "" && info.path.length == 1) {
            e = 0;
        } else if (e == "") {
            var msg = "format value path contains blanks";
            throw new self.FormatPatternError(pattern, startPos, msg);
        } else if (DIGITS.test(e)) {
            e = parseInt(e);
        }
        info.path[i] = e;
    }
    if (info.path.length < 0 || typeof(info.path[0]) != "number") {
        info.path.unshift(0);
    }
    return info;
}

/**
 * Parses a string with format flags and returns a format info object.
 *
 * @param {String} pattern the format pattern string
 * @param {Number} startPos the first index of the format instruction
 * @param {Number} endPos the last index of the format instruction
 *
 * @return {Object} the format info object
 *
 * @throws FormatPatternError if the format pattern was invalid
 */
MochiKit.Text._parseFormatFlags = function (pattern, startPos, endPos) {
    var update = MochiKit.Base.update;
    var info = { numeric: false, format: "s", width: 0, precision: -1,
                 align: ">", sign: "-", padding: " ", grouping: false };
    // TODO: replace with MochiKit.Format.rstrip?
    var flags = pattern.substring(startPos, endPos).replace(/\s+$/, "");
    while (flags.length > 0) {
        var chr = flags.charAt(0);
        var nextPos = 1;
        switch (chr) {
        case ">":
        case "<":
            update(info, { align: chr });
            break;
        case "+":
        case "-":
        case " ":
            update(info, { sign: chr });
            break;
        case ",":
            update(info, { grouping: true });
            break;
        case ".":
            var chars = /^\d*/.exec(flags.substring(1))[0];
            update(info, { precision: parseInt(chars) });
            nextPos = 1 + chars.length;
            break;
        case "0":
            update(info, { padding: chr });
            break;
        case "1":
        case "2":
        case "3":
        case "4":
        case "5":
        case "6":
        case "7":
        case "8":
        case "9":
            var chars = /^\d*/.exec(flags)[0];
            update(info, { width: parseInt(chars) });
            nextPos = chars.length;
            break;
        case "s":
        case "r":
            update(info, { format: chr });
            break;
        case "b":
            update(info, { numeric: true, format: chr, radix: 2 });
            break;
        case "o":
            update(info, { numeric: true, format: chr, radix: 8 });
            break;
        case "x":
        case "X":
            update(info, { numeric: true, format: chr, radix: 16 });
            break;
        case "d":
        case "f":
        case "%":
            update(info, { numeric: true, format: chr, radix: 10 });
            break;
        default:
            var msg = "unsupported format flag: " + chr;
            throw new MochiKit.Text.FormatPatternError(pattern, startPos, msg);
        }
        flags = flags.substring(nextPos);
    }
    return info;
}

/**
 * Formats a value as a percentage. This method avoids multiplication
 * by 100 since it leads to weird numeric rounding errors. Instead it
 * just move the decimal separator in the text string. It is ugly,
 * but works...
 *
 * @param {Number} value the value to format
 * @param {Number} precision the number of precision digits
 */
MochiKit.Text._truncToPercent = function (value, precision) {
    // TODO: This can be simplified by using MochiKit.Format._shiftNumber
    //       as roundToFixed does.
    var str;
    if (precision >= 0) {
        str = MochiKit.Format.roundToFixed(value, precision + 2);
    } else {
        str = (value == null) ? "0" : value.toString();
    }
    var arr = MochiKit.Text.split(str, ".", 2);
    var frac = MochiKit.Text.padRight(arr[1], 2, "0");
    var whole = arr[0] + frac.substring(0, 2);
    frac = frac.substring(2);
    while (/^0[0-9]/.test(whole)) {
        whole = whole.substring(1);
    }
    return (frac.length <= 0) ? whole : whole + "." + frac;
}

/**
 * Creates a new format pattern error.
 *
 * @param {String} pattern the format pattern string
 * @param {Number} pos the position of the error
 * @param {String} message the error message text
 *
 * @return {Error} the format pattern error
 *
 * @class The format pattern error class. This error is thrown when
 *     a syntax error is encountered inside a format string.
 * @property {String} pattern The format pattern string.
 * @property {Number} pos The position of the error.
 * @property {String} message The error message text.
 * @extends MochiKit.Base.NamedError
 */
MochiKit.Text.FormatPatternError = function (pattern, pos, message) {
    this.pattern = pattern;
    this.pos = pos;
    this.message = message;
}
MochiKit.Text.FormatPatternError.prototype =
    new MochiKit.Base.NamedError("MochiKit.Text.FormatPatternError");


//
//XXX: Internet Explorer exception handling blows
//
if (MochiKit.__export__) {
    formatter = MochiKit.Text.formatter;
    format = MochiKit.Text.format;
    formatValue = MochiKit.Text.formatValue;
}


MochiKit.Base.nameFunctions(MochiKit.Text);
MochiKit.Base._exportSymbols(this, MochiKit.Text);



///////////////////// Iter


/***

MochiKit.Iter 1.5

See <http://mochikit.com/> for documentation, downloads, license, etc.

(c) 2005 Bob Ippolito.  All rights Reserved.

***/

MochiKit.Base._module('Iter', '1.5', ['Base']);

MochiKit.Base.update(MochiKit.Iter, {
    /** @id MochiKit.Iter.registerIteratorFactory */
    registerIteratorFactory: function (name, check, iterfactory, /* optional */ override) {
        MochiKit.Iter.iteratorRegistry.register(name, check, iterfactory, override);
    },

    /** @id MochiKit.Iter.isIterable */
    isIterable: function(o) {
        return o != null &&
               (typeof(o.next) == "function" || typeof(o.iter) == "function");
    },

    /** @id MochiKit.Iter.iter */
    iter: function (iterable, /* optional */ sentinel) {
        var self = MochiKit.Iter;
        if (arguments.length == 2) {
            return self.takewhile(
                function (a) { return a != sentinel; },
                iterable
            );
        }
        if (typeof(iterable.next) == 'function') {
            return iterable;
        } else if (typeof(iterable.iter) == 'function') {
            return iterable.iter();
        /*
        }  else if (typeof(iterable.__iterator__) == 'function') {
            //
            // XXX: We can't support JavaScript 1.7 __iterator__ directly
            //      because of Object.prototype.__iterator__
            //
            return iterable.__iterator__();
        */
        }

        try {
            return self.iteratorRegistry.match(iterable);
        } catch (e) {
            var m = MochiKit.Base;
            if (e == m.NotFound) {
                e = new TypeError(typeof(iterable) + ": " + m.repr(iterable) + " is not iterable");
            }
            throw e;
        }
    },

    /** @id MochiKit.Iter.count */
    count: function (n) {
        if (!n) {
            n = 0;
        }
        var m = MochiKit.Base;
        return {
            repr: function () { return "count(" + n + ")"; },
            toString: m.forwardCall("repr"),
            next: m.counter(n)
        };
    },

    /** @id MochiKit.Iter.cycle */
    cycle: function (p) {
        var self = MochiKit.Iter;
        var m = MochiKit.Base;
        var lst = [];
        var iterator = self.iter(p);
        return {
            repr: function () { return "cycle(...)"; },
            toString: m.forwardCall("repr"),
            next: function () {
                try {
                    var rval = iterator.next();
                    lst.push(rval);
                    return rval;
                } catch (e) {
                    if (e != self.StopIteration) {
                        throw e;
                    }
                    if (lst.length === 0) {
                        this.next = function () {
                            throw self.StopIteration;
                        };
                    } else {
                        var i = -1;
                        this.next = function () {
                            i = (i + 1) % lst.length;
                            return lst[i];
                        };
                    }
                    return this.next();
                }
            }
        };
    },

    /** @id MochiKit.Iter.repeat */
    repeat: function (elem, /* optional */n) {
        var m = MochiKit.Base;
        if (typeof(n) == 'undefined') {
            return {
                repr: function () {
                    return "repeat(" + m.repr(elem) + ")";
                },
                toString: m.forwardCall("repr"),
                next: function () {
                    return elem;
                }
            };
        }
        return {
            repr: function () {
                return "repeat(" + m.repr(elem) + ", " + n + ")";
            },
            toString: m.forwardCall("repr"),
            next: function () {
                if (n <= 0) {
                    throw MochiKit.Iter.StopIteration;
                }
                n -= 1;
                return elem;
            }
        };
    },

    /** @id MochiKit.Iter.next */
    next: function (iterator) {
        return iterator.next();
    },

    /** @id MochiKit.Iter.izip */
    izip: function (p, q/*, ...*/) {
        var m = MochiKit.Base;
        var self = MochiKit.Iter;
        var next = self.next;
        var iterables = m.map(self.iter, arguments);
        return {
            repr: function () { return "izip(...)"; },
            toString: m.forwardCall("repr"),
            next: function () { return m.map(next, iterables); }
        };
    },

    /** @id MochiKit.Iter.ifilter */
    ifilter: function (pred, seq) {
        var m = MochiKit.Base;
        seq = MochiKit.Iter.iter(seq);
        if (pred === null) {
            pred = m.operator.truth;
        }
        return {
            repr: function () { return "ifilter(...)"; },
            toString: m.forwardCall("repr"),
            next: function () {
                while (true) {
                    var rval = seq.next();
                    if (pred(rval)) {
                        return rval;
                    }
                }
                // mozilla warnings aren't too bright
                return undefined;
            }
        };
    },

    /** @id MochiKit.Iter.ifilterfalse */
    ifilterfalse: function (pred, seq) {
        var m = MochiKit.Base;
        seq = MochiKit.Iter.iter(seq);
        if (pred === null) {
            pred = m.operator.truth;
        }
        return {
            repr: function () { return "ifilterfalse(...)"; },
            toString: m.forwardCall("repr"),
            next: function () {
                while (true) {
                    var rval = seq.next();
                    if (!pred(rval)) {
                        return rval;
                    }
                }
                // mozilla warnings aren't too bright
                return undefined;
            }
        };
    },

    /** @id MochiKit.Iter.islice */
    islice: function (seq/*, [start,] stop[, step] */) {
        var self = MochiKit.Iter;
        var m = MochiKit.Base;
        seq = self.iter(seq);
        var start = 0;
        var stop = 0;
        var step = 1;
        var i = -1;
        if (arguments.length == 2) {
            stop = arguments[1];
        } else if (arguments.length == 3) {
            start = arguments[1];
            stop = arguments[2];
        } else {
            start = arguments[1];
            stop = arguments[2];
            step = arguments[3];
        }
        return {
            repr: function () {
                return "islice(" + ["...", start, stop, step].join(", ") + ")";
            },
            toString: m.forwardCall("repr"),
            next: function () {
                var rval;
                while (i < start) {
                    rval = seq.next();
                    i++;
                }
                if (start >= stop) {
                    throw self.StopIteration;
                }
                start += step;
                return rval;
            }
        };
    },

    /** @id MochiKit.Iter.imap */
    imap: function (fun, p, q/*, ...*/) {
        var m = MochiKit.Base;
        var self = MochiKit.Iter;
        var iterables = m.map(self.iter, m.extend(null, arguments, 1));
        var map = m.map;
        var next = self.next;
        return {
            repr: function () { return "imap(...)"; },
            toString: m.forwardCall("repr"),
            next: function () {
                return fun.apply(this, map(next, iterables));
            }
        };
    },

    /** @id MochiKit.Iter.applymap */
    applymap: function (fun, seq, self) {
        seq = MochiKit.Iter.iter(seq);
        var m = MochiKit.Base;
        return {
            repr: function () { return "applymap(...)"; },
            toString: m.forwardCall("repr"),
            next: function () {
                return fun.apply(self, seq.next());
            }
        };
    },

    /** @id MochiKit.Iter.chain */
    chain: function (p, q/*, ...*/) {
        // dumb fast path
        var self = MochiKit.Iter;
        var m = MochiKit.Base;
        if (arguments.length == 1) {
            return self.iter(arguments[0]);
        }
        var argiter = m.map(self.iter, arguments);
        return {
            repr: function () { return "chain(...)"; },
            toString: m.forwardCall("repr"),
            next: function () {
                while (argiter.length > 1) {
                    try {
                        var result = argiter[0].next();
                        return result;
                    } catch (e) {
                        if (e != self.StopIteration) {
                            throw e;
                        }
                        argiter.shift();
                        var result = argiter[0].next();
                        return result;
                    }
                }
                if (argiter.length == 1) {
                    // optimize last element
                    var arg = argiter.shift();
                    this.next = m.bind("next", arg);
                    return this.next();
                }
                throw self.StopIteration;
            }
        };
    },

    /** @id MochiKit.Iter.takewhile */
    takewhile: function (pred, seq) {
        var self = MochiKit.Iter;
        seq = self.iter(seq);
        return {
            repr: function () { return "takewhile(...)"; },
            toString: MochiKit.Base.forwardCall("repr"),
            next: function () {
                var rval = seq.next();
                if (!pred(rval)) {
                    this.next = function () {
                        throw self.StopIteration;
                    };
                    this.next();
                }
                return rval;
            }
        };
    },

    /** @id MochiKit.Iter.dropwhile */
    dropwhile: function (pred, seq) {
        seq = MochiKit.Iter.iter(seq);
        var m = MochiKit.Base;
        var bind = m.bind;
        return {
            "repr": function () { return "dropwhile(...)"; },
            "toString": m.forwardCall("repr"),
            "next": function () {
                while (true) {
                    var rval = seq.next();
                    if (!pred(rval)) {
                        break;
                    }
                }
                this.next = bind("next", seq);
                return rval;
            }
        };
    },

    _tee: function (ident, sync, iterable) {
        sync.pos[ident] = -1;
        var m = MochiKit.Base;
        var listMin = m.listMin;
        return {
            repr: function () { return "tee(" + ident + ", ...)"; },
            toString: m.forwardCall("repr"),
            next: function () {
                var rval;
                var i = sync.pos[ident];

                if (i == sync.max) {
                    rval = iterable.next();
                    sync.deque.push(rval);
                    sync.max += 1;
                    sync.pos[ident] += 1;
                } else {
                    rval = sync.deque[i - sync.min];
                    sync.pos[ident] += 1;
                    if (i == sync.min && listMin(sync.pos) != sync.min) {
                        sync.min += 1;
                        sync.deque.shift();
                    }
                }
                return rval;
            }
        };
    },

    /** @id MochiKit.Iter.tee */
    tee: function (iterable, n/* = 2 */) {
        var rval = [];
        var sync = {
            "pos": [],
            "deque": [],
            "max": -1,
            "min": -1
        };
        if (arguments.length == 1 || typeof(n) == "undefined" || n === null) {
            n = 2;
        }
        var self = MochiKit.Iter;
        iterable = self.iter(iterable);
        var _tee = self._tee;
        for (var i = 0; i < n; i++) {
            rval.push(_tee(i, sync, iterable));
        }
        return rval;
    },

    /** @id MochiKit.Iter.list */
    list: function (iterable) {
        // Fast-path for Array and Array-like
        var rval;
        if (iterable instanceof Array) {
            return iterable.slice();
        } 
        // this is necessary to avoid a Safari crash
        if (typeof(iterable) == "function" &&
                !(iterable instanceof Function) &&
                typeof(iterable.length) == 'number') {
            rval = [];
            for (var i = 0; i < iterable.length; i++) {
                rval.push(iterable[i]);
            }
            return rval;
        }

        var self = MochiKit.Iter;
        iterable = self.iter(iterable);
        var rval = [];
        var a_val;
        try {
            while (true) {
                a_val = iterable.next();
                rval.push(a_val);
            }
        } catch (e) {
            if (e != self.StopIteration) {
                throw e;
            }
            return rval;
        }
        // mozilla warnings aren't too bright
        return undefined;
    },


    /** @id MochiKit.Iter.reduce */
    reduce: function (fn, iterable, /* optional */initial) {
        var i = 0;
        var x = initial;
        var self = MochiKit.Iter;
        iterable = self.iter(iterable);
        if (arguments.length < 3) {
            try {
                x = iterable.next();
            } catch (e) {
                if (e == self.StopIteration) {
                    e = new TypeError("reduce() of empty sequence with no initial value");
                }
                throw e;
            }
            i++;
        }
        try {
            while (true) {
                x = fn(x, iterable.next());
            }
        } catch (e) {
            if (e != self.StopIteration) {
                throw e;
            }
        }
        return x;
    },

    /** @id MochiKit.Iter.range */
    range: function (/* [start,] stop[, step] */) {
        var start = 0;
        var stop = 0;
        var step = 1;
        if (arguments.length == 1) {
            stop = arguments[0];
        } else if (arguments.length == 2) {
            start = arguments[0];
            stop = arguments[1];
        } else if (arguments.length == 3) {
            start = arguments[0];
            stop = arguments[1];
            step = arguments[2];
        } else {
            throw new TypeError("range() takes 1, 2, or 3 arguments!");
        }
        if (step === 0) {
            throw new TypeError("range() step must not be 0");
        }
        return {
            next: function () {
                if ((step > 0 && start >= stop) || (step < 0 && start <= stop)) {
                    throw MochiKit.Iter.StopIteration;
                }
                var rval = start;
                start += step;
                return rval;
            },
            repr: function () {
                return "range(" + [start, stop, step].join(", ") + ")";
            },
            toString: MochiKit.Base.forwardCall("repr")
        };
    },

    /** @id MochiKit.Iter.sum */
    sum: function (iterable, start/* = 0 */) {
        if (typeof(start) == "undefined" || start === null) {
            start = 0;
        }
        var x = start;
        var self = MochiKit.Iter;
        iterable = self.iter(iterable);
        try {
            while (true) {
                x += iterable.next();
            }
        } catch (e) {
            if (e != self.StopIteration) {
                throw e;
            }
        }
        return x;
    },

    /** @id MochiKit.Iter.exhaust */
    exhaust: function (iterable) {
        var self = MochiKit.Iter;
        iterable = self.iter(iterable);
        try {
            while (true) {
                iterable.next();
            }
        } catch (e) {
            if (e != self.StopIteration) {
                throw e;
            }
        }
    },

    /** @id MochiKit.Iter.forEach */
    forEach: function (iterable, func, /* optional */obj) {
        var m = MochiKit.Base;
        var self = MochiKit.Iter;
        if (arguments.length > 2) {
            func = m.bind(func, obj);
        }
        // fast path for array
        if (m.isArrayLike(iterable) && !self.isIterable(iterable)) {
            try {
                for (var i = 0; i < iterable.length; i++) {
                    func(iterable[i]);
                }
            } catch (e) {
                if (e != self.StopIteration) {
                    throw e;
                }
            }
        } else {
            self.exhaust(self.imap(func, iterable));
        }
    },

    /** @id MochiKit.Iter.every */
    every: function (iterable, func) {
        var self = MochiKit.Iter;
        try {
            self.ifilterfalse(func, iterable).next();
            return false;
        } catch (e) {
            if (e != self.StopIteration) {
                throw e;
            }
            return true;
        }
    },

    /** @id MochiKit.Iter.sorted */
    sorted: function (iterable, /* optional */cmp) {
        var rval = MochiKit.Iter.list(iterable);
        if (arguments.length == 1) {
            cmp = MochiKit.Base.compare;
        }
        rval.sort(cmp);
        return rval;
    },

    /** @id MochiKit.Iter.reversed */
    reversed: function (iterable) {
        var rval = MochiKit.Iter.list(iterable);
        rval.reverse();
        return rval;
    },

    /** @id MochiKit.Iter.some */
    some: function (iterable, func) {
        var self = MochiKit.Iter;
        try {
            self.ifilter(func, iterable).next();
            return true;
        } catch (e) {
            if (e != self.StopIteration) {
                throw e;
            }
            return false;
        }
    },

    /** @id MochiKit.Iter.iextend */
    iextend: function (lst, iterable) {
        var m = MochiKit.Base;
        var self = MochiKit.Iter;
        if (m.isArrayLike(iterable) && !self.isIterable(iterable)) {
            // fast-path for array-like
            for (var i = 0; i < iterable.length; i++) {
                lst.push(iterable[i]);
            }
        } else {
            iterable = self.iter(iterable);
            try {
                while (true) {
                    lst.push(iterable.next());
                }
            } catch (e) {
                if (e != self.StopIteration) {
                    throw e;
                }
            }
        }
        return lst;
    },

    /** @id MochiKit.Iter.groupby */
    groupby: function(iterable, /* optional */ keyfunc) {
        var m = MochiKit.Base;
        var self = MochiKit.Iter;
        if (arguments.length < 2) {
            keyfunc = m.operator.identity;
        }
        iterable = self.iter(iterable);

        // shared
        var pk = undefined;
        var k = undefined;
        var v;

        function fetch() {
            v = iterable.next();
            k = keyfunc(v);
        };

        function eat() {
            var ret = v;
            v = undefined;
            return ret;
        };

        var first = true;
        var compare = m.compare;
        return {
            repr: function () { return "groupby(...)"; },
            next: function() {
                // iterator-next

                // iterate until meet next group
                while (compare(k, pk) === 0) {
                    fetch();
                    if (first) {
                        first = false;
                        break;
                    }
                }
                pk = k;
                return [k, {
                    next: function() {
                        // subiterator-next
                        if (v == undefined) { // Is there something to eat?
                            fetch();
                        }
                        if (compare(k, pk) !== 0) {
                            throw self.StopIteration;
                        }
                        return eat();
                    }
                }];
            }
        };
    },

    /** @id MochiKit.Iter.groupby_as_array */
    groupby_as_array: function (iterable, /* optional */ keyfunc) {
        var m = MochiKit.Base;
        var self = MochiKit.Iter;
        if (arguments.length < 2) {
            keyfunc = m.operator.identity;
        }

        iterable = self.iter(iterable);
        var result = [];
        var first = true;
        var prev_key;
        var compare = m.compare;
        while (true) {
            try {
                var value = iterable.next();
                var key = keyfunc(value);
            } catch (e) {
                if (e == self.StopIteration) {
                    break;
                }
                throw e;
            }
            if (first || compare(key, prev_key) !== 0) {
                var values = [];
                result.push([key, values]);
            }
            values.push(value);
            first = false;
            prev_key = key;
        }
        return result;
    },

    /** @id MochiKit.Iter.arrayLikeIter */
    arrayLikeIter: function (iterable) {
        var i = 0;
        return {
            repr: function () { return "arrayLikeIter(...)"; },
            toString: MochiKit.Base.forwardCall("repr"),
            next: function () {
                if (i >= iterable.length) {
                    throw MochiKit.Iter.StopIteration;
                }
                return iterable[i++];
            }
        };
    },

    /** @id MochiKit.Iter.hasIterateNext */
    hasIterateNext: function (iterable) {
        return (iterable && typeof(iterable.iterateNext) == "function");
    },

    /** @id MochiKit.Iter.iterateNextIter */
    iterateNextIter: function (iterable) {
        return {
            repr: function () { return "iterateNextIter(...)"; },
            toString: MochiKit.Base.forwardCall("repr"),
            next: function () {
                var rval = iterable.iterateNext();
                if (rval === null || rval === undefined) {
                    throw MochiKit.Iter.StopIteration;
                }
                return rval;
            }
        };
    }
});


MochiKit.Iter.__new__ = function () {
    var m = MochiKit.Base;
    // Re-use StopIteration if exists (e.g. SpiderMonkey)
    if (typeof(StopIteration) != "undefined") {
        this.StopIteration = StopIteration;
    } else {
        /** @id MochiKit.Iter.StopIteration */
        this.StopIteration = new m.NamedError("StopIteration");
    }
    this.iteratorRegistry = new m.AdapterRegistry();
    // Register the iterator factory for arrays
    this.registerIteratorFactory(
        "arrayLike",
        m.isArrayLike,
        this.arrayLikeIter
    );

    this.registerIteratorFactory(
        "iterateNext",
        this.hasIterateNext,
        this.iterateNextIter
    );

    m.nameFunctions(this);

};

MochiKit.Iter.__new__();

//
// XXX: Internet Explorer blows
//
if (MochiKit.__export__) {
    reduce = MochiKit.Iter.reduce;
}

MochiKit.Base._exportSymbols(this, MochiKit.Iter);

