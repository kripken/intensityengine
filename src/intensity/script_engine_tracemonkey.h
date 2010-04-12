
// Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
// This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

#include "jsapi.h"

class TraceMonkeyValue : public ScriptValue
{
    jsval value;
    std::string debugName;
public:
    TraceMonkeyValue(ScriptEngine* _engine);
    TraceMonkeyValue(ScriptEngine* _engine, int _value);
    TraceMonkeyValue(ScriptEngine* _engine, double _value);
//    TraceMonkeyValue(std::string _value);
    TraceMonkeyValue(ScriptEngine* _engine, bool internal, jsval _value); // For internal use. Two params, since jsval is an int.

    virtual void invalidate();

    virtual void setProperty(std::string propertyName, ScriptValuePtr propertyValue);
    virtual void setProperty(std::string propertyName, int propertyValue);
    virtual void setProperty(std::string propertyName, double propertyValue);

    virtual bool hasProperty(std::string propertyName);

    virtual ScriptValuePtr getProperty(std::string propertyName);
    virtual int getPropertyInt(std::string propertyName);
    virtual bool getPropertyBool(std::string propertyName);
    virtual double getPropertyFloat(std::string propertyName);
    virtual std::string getPropertyString(std::string propertyName);

    virtual int getInt();
    virtual bool getBool();
    virtual double getFloat();
    virtual std::string getString();

    virtual ScriptValuePtr call(std::string funcName);
    virtual ScriptValuePtr call(std::string funcName, ScriptValuePtr arg1);
    virtual ScriptValuePtr call(std::string funcName, int arg1);
    virtual ScriptValuePtr call(std::string funcName, double arg1);
    virtual ScriptValuePtr call(std::string funcName, std::string arg1);
    virtual ScriptValuePtr call(std::string funcName, ScriptValueArgs& args);

    virtual bool compare(ScriptValuePtr other);

    virtual void debugPrint();
};

class TraceMonkeyEngine : public ScriptEngine
{
public:
    static JSRuntime *runtime;
    static JSContext *context;
    static JSObject  *global;

    virtual void init();
    virtual void quit();

    virtual ScriptValuePtr createObject();
    virtual ScriptValuePtr createFunction(NativeFunction func, int numArgs);
    virtual ScriptValuePtr createScriptValue(int value);
    virtual ScriptValuePtr createScriptValue(double value);
    virtual ScriptValuePtr createScriptValue(std::string value);

    virtual ScriptValuePtr getGlobal();
    virtual ScriptValuePtr getNull();

    virtual ScriptValuePtr runScript(std::string script);

    static LogicEntityPtr getCLogicEntity(JSObject* scriptingEntity);
};


// Function wrappers. See comments in script_engine_manager.cpp,
// in particular for why we have _is etc. separately.

// Generic wrapper
#define TRACEMONKEY_FUNC_GEN(new_func, arguments_def, arguments_conv, wrapped_code) \
JSBool new_func(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) \
{ \
    Logging::log(Logging::INFO, "TMF: %s\r\n", #new_func); \
    *rval = JSVAL_VOID; /* If not changed later, do not return anything */ \
    arguments_def; \
 \
    if (!(arguments_conv)) { \
        JS_ReportError(cx, "Error getting parameters"); \
        return JS_FALSE; \
    } \
 \
    wrapped_code; \
 \
    return JS_TRUE; \
}

// Wrap a function with no parameters
#define TRACEMONKEY_FUNC_NOPARAM(new_func, wrapped_code) \
    TRACEMONKEY_FUNC_GEN(new_func, , true, wrapped_code);


// Wrap a function with one parameter
#define TRACEMONKEY_FUNC_1(new_func, type1, type_codes, wrapped_code) \
    TRACEMONKEY_FUNC_GEN(new_func, type1 arg1, (JS_ConvertArguments(cx, argc, argv, #type_codes, &arg1)), wrapped_code);

// int
#define TRACEMONKEY_FUNC_i(new_func, wrapped_code) \
    TRACEMONKEY_FUNC_1(new_func, int, i, wrapped_code);

// int
#define TRACEMONKEY_FUNC_s(new_func, wrapped_code) \
    TRACEMONKEY_FUNC_1(new_func, int, s, wrapped_code);

// object
#define TRACEMONKEY_FUNC_o(new_func, wrapped_code) \
    TRACEMONKEY_FUNC_1(new_func, JSObject*, o, wrapped_code);

//
// Wrap a function with 2 parameters
#define TRACEMONKEY_FUNC_2(new_func, type1, type2, type_codes, wrapped_code) \
    TRACEMONKEY_FUNC_GEN(new_func, type1 arg1; type2 arg2, (JS_ConvertArguments(cx, argc, argv, #type_codes, &arg1, &arg2)), wrapped_code);

// int, int
#define TRACEMONKEY_FUNC_ii(new_func, wrapped_code) \
    TRACEMONKEY_FUNC_2(new_func, int, int, ii, wrapped_code);

// int, string
#define TRACEMONKEY_FUNC_is(new_func, wrapped_code) \
    TRACEMONKEY_FUNC_2(new_func, int, char*, is, wrapped_code);

// object, int
#define TRACEMONKEY_FUNC_oi(new_func, wrapped_code) \
    TRACEMONKEY_FUNC_2(new_func, JSObject*, int, oi, wrapped_code);

// object, bool
#define TRACEMONKEY_FUNC_ob(new_func, wrapped_code) \
        TRACEMONKEY_FUNC_2(new_func, JSObject*, bool, ob, wrapped_code);

// object, string
#define TRACEMONKEY_FUNC_os(new_func, wrapped_code) \
    TRACEMONKEY_FUNC_2(new_func, JSObject*, char*, os, wrapped_code);

// object, double
#define TRACEMONKEY_FUNC_od(new_func, wrapped_code) \
    TRACEMONKEY_FUNC_2(new_func, JSObject*, double, od, wrapped_code);

//
// Wrap a function with 3 parameters
#define TRACEMONKEY_FUNC_3(new_func, type1, type2, type3, type_codes, wrapped_code) \
    TRACEMONKEY_FUNC_GEN(new_func, type1 arg1; type2 arg2; type3 arg3, (JS_ConvertArguments(cx, argc, argv, #type_codes, &arg1, &arg2, &arg3)), wrapped_code);

// iis
#define TRACEMONKEY_FUNC_iis(new_func, wrapped_code) \
    TRACEMONKEY_FUNC_3(new_func, int, int, char*, iis, wrapped_code);

//
// Wrap a function with 4 parameters
#define TRACEMONKEY_FUNC_4(new_func, type1, type2, type3, type4, type_codes, wrapped_code) \
    TRACEMONKEY_FUNC_GEN(new_func, type1 arg1; type2 arg2; type3 arg3; type4 arg4, (JS_ConvertArguments(cx, argc, argv, #type_codes, &arg1, &arg2, &arg3, &arg4)), wrapped_code);

// oddd
#define TRACEMONKEY_FUNC_oddd(new_func, wrapped_code) \
    TRACEMONKEY_FUNC_4(new_func, JSObject*, double, double, double, oddd, wrapped_code);

// dddd
#define TRACEMONKEY_FUNC_dddd(new_func, wrapped_code) \
    TRACEMONKEY_FUNC_4(new_func, double, double, double, double, dddd, wrapped_code);

// iddd
#define TRACEMONKEY_FUNC_iddd(new_func, wrapped_code) \
    TRACEMONKEY_FUNC_4(new_func, int, double, double, double, iddd, wrapped_code);

// iiss
#define TRACEMONKEY_FUNC_iiss(new_func, wrapped_code) \
    TRACEMONKEY_FUNC_4(new_func, int, int, char*, char*, iiss, wrapped_code);

// iiis
#define TRACEMONKEY_FUNC_iiis(new_func, wrapped_code) \
    TRACEMONKEY_FUNC_4(new_func, int, int, int, char*, iiis, wrapped_code);

//
// Wrap a function with 5 parameters
#define TRACEMONKEY_FUNC_5(new_func, type1, type2, type3, type4, type5, type_codes, wrapped_code) \
    TRACEMONKEY_FUNC_GEN(new_func, type1 arg1; type2 arg2; type3 arg3; type4 arg4; type5 arg5, (JS_ConvertArguments(cx, argc, argv, #type_codes, &arg1, &arg2, &arg3, &arg4, &arg5)), wrapped_code);

// sdddi
#define TRACEMONKEY_FUNC_sdddi(new_func, wrapped_code) \
    TRACEMONKEY_FUNC_5(new_func, char*, double, double, double, int, sdddi, wrapped_code);

// iiiss
#define TRACEMONKEY_FUNC_iiiss(new_func, wrapped_code) \
    TRACEMONKEY_FUNC_5(new_func, int, int, int, char*, char*, iiiss, wrapped_code);

//
// Wrap a function with 6 parameters
#define TRACEMONKEY_FUNC_6(new_func, type1, type2, type3, type4, type5, type6, type_codes, wrapped_code) \
    TRACEMONKEY_FUNC_GEN(new_func, type1 arg1; type2 arg2; type3 arg3; type4 arg4; type5 arg5; type6 arg6, (JS_ConvertArguments(cx, argc, argv, #type_codes, &arg1, &arg2, &arg3, &arg4, &arg5, &arg6)), wrapped_code);

// d^6
#define TRACEMONKEY_FUNC_dddddd(new_func, wrapped_code) \
    TRACEMONKEY_FUNC_6(new_func, double, double, double, double, double, double, dddddd, wrapped_code);

// i^3d^3
#define TRACEMONKEY_FUNC_iiiddd(new_func, wrapped_code) \
    TRACEMONKEY_FUNC_6(new_func, int, int, int, double, double, double, iiiddd, wrapped_code);

// d^4i^2
#define TRACEMONKEY_FUNC_ddddii(new_func, wrapped_code) \
    TRACEMONKEY_FUNC_6(new_func, double, double, double, double, int, int, ddddii, wrapped_code);

// idddsi
#define TRACEMONKEY_FUNC_idddsi(new_func, wrapped_code) \
    TRACEMONKEY_FUNC_6(new_func, int, double, double, double, char*, int, idddsi, wrapped_code);

// ssiiif
#define TRACEMONKEY_FUNC_ssiiif(new_func, wrapped_code) \
    TRACEMONKEY_FUNC_6(new_func, char*, char*, int, int, int, double, ssiiif, wrapped_code);

// Wrap a function with 7 parameters
#define TRACEMONKEY_FUNC_7(new_func, type1, type2, type3, type4, type5, type6, type7, type_codes, wrapped_code) \
    TRACEMONKEY_FUNC_GEN(new_func, type1 arg1; type2 arg2; type3 arg3; type4 arg4; type5 arg5; type6 arg6; type7 arg7, (JS_ConvertArguments(cx, argc, argv, #type_codes, &arg1, &arg2, &arg3, &arg4, &arg5, &arg6, &arg7)), wrapped_code);

// d^7
#define TRACEMONKEY_FUNC_ddddddd(new_func, wrapped_code) \
    TRACEMONKEY_FUNC_7(new_func, double, double, double, double, double, double, double, ddddddd, wrapped_code);

// iiiiddd
#define TRACEMONKEY_FUNC_iiiiddd(new_func, wrapped_code) \
    TRACEMONKEY_FUNC_7(new_func, int, int, int, int, double, double, double, iiiiddd, wrapped_code);

//
// Wrap a function with 8 parameters
#define TRACEMONKEY_FUNC_8(new_func, type1, type2, type3, type4, type5, type6, type7, type8, type_codes, wrapped_code) \
    TRACEMONKEY_FUNC_GEN(new_func, type1 arg1; type2 arg2; type3 arg3; type4 arg4; type5 arg5; type6 arg6; type7 arg7; type8 arg8, (JS_ConvertArguments(cx, argc, argv, #type_codes, &arg1, &arg2, &arg3, &arg4, &arg5, &arg6, &arg7, &arg8)), wrapped_code);

// ddddddii
#define TRACEMONKEY_FUNC_ddddddii(new_func, wrapped_code) \
    TRACEMONKEY_FUNC_8(new_func, double, double, double, double, double, double, int, int, ddddddii, wrapped_code);

//
// Wrap a function with 9 parameters
#define TRACEMONKEY_FUNC_9(new_func, type1, type2, type3, type4, type5, type6, type7, type8, type9, type_codes, wrapped_code) \
    TRACEMONKEY_FUNC_GEN(new_func, type1 arg1; type2 arg2; type3 arg3; type4 arg4; type5 arg5; type6 arg6; type7 arg7; type8 arg8; type9 arg9, (JS_ConvertArguments(cx, argc, argv, #type_codes, &arg1, &arg2, &arg3, &arg4, &arg5, &arg6, &arg7, &arg8, &arg9)), wrapped_code);

// object, idddiiii
#define TRACEMONKEY_FUNC_oidddiiii(new_func, wrapped_code) \
    TRACEMONKEY_FUNC_9(new_func, JSObject*, int, double, double, double, int, int, int, int, oidddiiii, wrapped_code);

// idddidddi
#define TRACEMONKEY_FUNC_idddidddi(new_func, wrapped_code) \
    TRACEMONKEY_FUNC_9(new_func, int, double, double, double, int, double, double, double, int, idddidddi, wrapped_code);

// Wrap a function with 11 parameters
#define TRACEMONKEY_FUNC_11(new_func, type1, type2, type3, type4, type5, type6, type7, type8, type9, type10, type11, type_codes, wrapped_code) \
    TRACEMONKEY_FUNC_GEN(new_func, type1 arg1; type2 arg2; type3 arg3; type4 arg4; type5 arg5; type6 arg6; type7 arg7; type8 arg8; type9 arg9; type10 arg10; type11 arg11, (JS_ConvertArguments(cx, argc, argv, #type_codes, &arg1, &arg2, &arg3, &arg4, &arg5, &arg6, &arg7, &arg8, &arg9, &arg10, &arg11)), wrapped_code);

// object, idddiiii
#define TRACEMONKEY_FUNC_iissdddiiii(new_func, wrapped_code) \
    TRACEMONKEY_FUNC_11(new_func, int, int, char*, char*, double, double, double, int, int, int, int, iissdddiiii, wrapped_code);

//
// Wrap a function with 12 parameters
#define TRACEMONKEY_FUNC_12(new_func, type1, type2, type3, type4, type5, type6, type7, type8, type9, type10, type11, type12, type_codes, wrapped_code) \
    TRACEMONKEY_FUNC_GEN(new_func, type1 arg1; type2 arg2; type3 arg3; type4 arg4; type5 arg5; type6 arg6; type7 arg7; type8 arg8; type9 arg9; type10 arg10; type11 arg11; type12 arg12, (JS_ConvertArguments(cx, argc, argv, #type_codes, &arg1, &arg2, &arg3, &arg4, &arg5, &arg6, &arg7, &arg8, &arg9, &arg10, &arg11, &arg12)), wrapped_code);

// object, idddiiii
#define TRACEMONKEY_FUNC_idddddddiiii(new_func, wrapped_code) \
    TRACEMONKEY_FUNC_12(new_func, int, double, double, double, double, double, double, double, int, int, int, int, idddddddiiii, wrapped_code);

//
// Wrap a function with 14 parameters
#define TRACEMONKEY_FUNC_14(new_func, type1, type2, type3, type4, type5, type6, type7, type8, type9, type10, type11, type12, type13, type14, type_codes, wrapped_code) \
    TRACEMONKEY_FUNC_GEN(new_func, type1 arg1; type2 arg2; type3 arg3; type4 arg4; type5 arg5; type6 arg6; type7 arg7; type8 arg8; type9 arg9; type10 arg10; type11 arg11; type12 arg12; type13 arg13; type14 arg14, (JS_ConvertArguments(cx, argc, argv, #type_codes, &arg1, &arg2, &arg3, &arg4, &arg5, &arg6, &arg7, &arg8, &arg9, &arg10, &arg11, &arg12, &arg13, &arg14)), wrapped_code);

// dddddddiiidddd
#define TRACEMONKEY_FUNC_dddddddiiidddd(new_func, wrapped_code) \
    TRACEMONKEY_FUNC_14(new_func, double, double, double, double, double, double, double, int, int, int, double, double, double, double, dddddddiiidddd, wrapped_code);

//
// Objects
//

// Wrap a function with a JSObject interpreted as "this", converted into a LogicEntityPtr, and some other parameters
#define TRACEMONKEY_FUNC_T(new_func, type_codes, wrapped_code) \
    TRACEMONKEY_FUNC_o##type_codes(new_func, { \
        Logging::log(Logging::INFO, "TMF_T: %s\r\n", #new_func); \
        LogicEntityPtr self = TraceMonkeyEngine::getCLogicEntity(arg1); \
        assert(self.get()); \
        wrapped_code; \
    });

// Wrap a function with a JSObject interpreted as "this", converted into a ScriptValuePtr, and some other parameters
#define TRACEMONKEY_FUNC_Z(new_func, type_codes, wrapped_code) \
    TRACEMONKEY_FUNC_o##type_codes(new_func, { \
        Logging::log(Logging::INFO, "TMF_Z: %s\r\n", #new_func); \
        ScriptValuePtr self(new TraceMonkeyValue(ScriptEngineManager::getEngine(), true, OBJECT_TO_JSVAL(arg1))); \
        wrapped_code; \
    });

// Return values
#define TRACEMONKEY_RETURN_INT(value)\
    { \
        *rval = INT_TO_JSVAL(value); \
    }

#define TRACEMONKEY_RETURN_DOUBLE(value)\
    { \
        *rval = DOUBLE_TO_JSVAL(value); \
    }

#define TRACEMONKEY_RETURN_BOOL(value)\
    { \
        *rval = BOOLEAN_TO_JSVAL(value); \
    }

