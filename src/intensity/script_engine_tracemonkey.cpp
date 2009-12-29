
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

// TODO
//
// MochiKit's evalJSON is unsafe. Use the one from json.org.

#include "pch.h"
#include "cube.h"
#include "iengine.h"
#include "igame.h"

#include "logging.h"

#include "script_engine_manager.h"
#include "script_engine_tracemonkey.h"
#include "jsapi.h"
//#include "jsstr.h"

///////// DEBUG

/*
std::string JSSTRING_TO_STRING(JSString* jsstr)
{
    std::string ret = "";
    for (unsigned int i = 0; i < JSSTRING_LENGTH(jsstr); i++)
        ret += char(JSSTRING_CHARS(jsstr)[i]);
    return ret;
}
*/

void printJSVAL(jsval value)
{
    if (JSVAL_IS_INT(value))
        Logging::log(Logging::INFO, "INT: %d\r\n", JSVAL_TO_INT(value));
    else if (JSVAL_IS_NULL(value))
        Logging::log(Logging::INFO, "NULL (null)\r\n");
    else if (JSVAL_IS_VOID(value))
        Logging::log(Logging::INFO, "VOID (undefined)\r\n");
    else if (JSVAL_IS_BOOLEAN(value))
        Logging::log(Logging::INFO, "BOOLEAN: %d\r\n", JSVAL_TO_BOOLEAN(value));
    else if (JSVAL_IS_NUMBER(value))
        Logging::log(Logging::INFO, "NUMBER: %f\r\n", *JSVAL_TO_DOUBLE(value));
    else if (JSVAL_IS_DOUBLE(value))
        Logging::log(Logging::INFO, "DOUBLE: %f\r\n", *JSVAL_TO_DOUBLE(value));
    else if (JSVAL_IS_STRING(value))
    {
        JSString* str = JS_ValueToString(TraceMonkeyEngine::context, value);
        char* chr = JS_GetStringBytes(str);
        Logging::log(Logging::INFO, "STRING: %s\r\n", chr);

/*
        INDENT_LOG(Logging::DEBUG);

        jsval func;
        bool success = JS_GetProperty(TraceMonkeyEngine::context, TraceMonkeyEngine::global, "log", &func);
        assert(success);

        assert(JSVAL_IS_OBJECT(func));
        assert(!JSVAL_IS_NULL(func));

        jsval jsArgs[2];
        jsArgs[0] = INT_TO_JSVAL(Logging::DEBUG);
        jsArgs[1] = value;

        jsval ret;

        JS_CallFunctionValue(TraceMonkeyEngine::context, JSVAL_TO_OBJECT(value), func, 2, jsArgs, &ret);
*/
    }
    else if (JSVAL_IS_OBJECT(value))
    {
        Logging::log(Logging::INFO, "OBJECT (object)\r\n");

        assert(!JSVAL_IS_NULL(value));
        jsval ret;
        JS_GetProperty(TraceMonkeyEngine::context, JSVAL_TO_OBJECT(value), "uniqueId", &ret);

        if (JSVAL_IS_INT(ret))
            Logging::log(Logging::INFO, "OBJECT (object) has uniqueId: %d\r\n", JSVAL_TO_INT(ret));
        else
            Logging::log(Logging::INFO, "OBJECT (object) has no uniqueId\r\n");

/*
        if (TraceMonkeyEngine::global)
        {
            ScriptEngineManager::getGlobal()->call("log",
                ScriptValueArgs().append(Logging::DEBUG).append(
//                    ScriptEngineManager::getGlobal()->call("serializeJSON",
                        ScriptEngineManager::getGlobal()->call("keys", // Just print keys, to avoid recursions etc.
                            ScriptValuePtr(new TraceMonkeyValue(true, value))
                        )
//                    )
                )
            );
        }
*/
    }
    else {
        Logging::log(Logging::INFO, "Uncertain jsval\r\n");
    }
}

///////// Values

int __nameCounter = 0;

TraceMonkeyValue::TraceMonkeyValue(ScriptEngine* _engine) : ScriptValue(_engine)
{
    value = JSVAL_NULL;

    debugName = "NULL value no. " + Utility::toString(__nameCounter);

    Logging::log(Logging::INFO, "Going to create a TM value of: %s\r\n", debugName.c_str());
    INDENT_LOG(Logging::INFO);

    printJSVAL(value);

    bool ret = JS_AddNamedRoot(TraceMonkeyEngine::context, &(this->value), debugName.c_str()); // Ensure our value won't be GCed
    assert(ret);

    Logging::log(Logging::INFO, "Created a TM value of: %s\r\n", debugName.c_str());

    __nameCounter += 1;

    JS_GC(TraceMonkeyEngine::context); // XXX For debugging purposes 
    Logging::log(Logging::INFO, "post-creation GC ok.\r\n");
}

TraceMonkeyValue::TraceMonkeyValue(ScriptEngine* _engine, int _value) : ScriptValue(_engine)
{
    debugName = "int value no. " + Utility::toString(__nameCounter);

    Logging::log(Logging::INFO, "Going to create a TM value of: %s\r\n", debugName.c_str());
    INDENT_LOG(Logging::INFO);

    value = INT_TO_JSVAL(_value);
    bool ret = JS_AddNamedRoot(TraceMonkeyEngine::context, &(this->value), debugName.c_str()); // Ensure our value won't be GCed
    assert(ret);

    printJSVAL(value);

    Logging::log(Logging::INFO, "Created a TM value of: %s\r\n", debugName.c_str());

    __nameCounter += 1;

    JS_GC(TraceMonkeyEngine::context); // XXX For debugging purposes
    Logging::log(Logging::INFO, "post-creation GC ok.\r\n");
}

TraceMonkeyValue::TraceMonkeyValue(ScriptEngine* _engine, double _value) : ScriptValue(_engine)
{
    debugName = "double value no. " + Utility::toString(__nameCounter);

    Logging::log(Logging::INFO, "Going to create a TM value of: %s\r\n", debugName.c_str());
    INDENT_LOG(Logging::INFO);

    value = DOUBLE_TO_JSVAL(_value);
    bool ret = JS_AddNamedRoot(TraceMonkeyEngine::context, &(this->value), debugName.c_str()); // Ensure our value won't be GCed
    assert(ret);

    printJSVAL(value);

    Logging::log(Logging::INFO, "Created a TM value of: %s\r\n", debugName.c_str());

    __nameCounter += 1;

    JS_GC(TraceMonkeyEngine::context); // XXX For debugging purposes
    Logging::log(Logging::INFO, "post-creation GC ok.\r\n");
}

TraceMonkeyValue::TraceMonkeyValue(ScriptEngine* _engine, bool internal, jsval _value) : ScriptValue(_engine)
{
    assert(internal);

    debugName = "jsval value no. " + Utility::toString(__nameCounter);

    Logging::log(Logging::INFO, "Going to create a TM value of: %s\r\n", debugName.c_str());
    INDENT_LOG(Logging::INFO);

    value = _value;
    bool ret = JS_AddNamedRoot(TraceMonkeyEngine::context, &(this->value), debugName.c_str()); // Ensure our value won't be GCed
    assert(ret);

    printJSVAL(value);

    Logging::log(Logging::INFO, "Created a TM value of: %s\r\n", debugName.c_str());

    __nameCounter += 1;

    JS_GC(TraceMonkeyEngine::context); // XXX For debugging purposes
    Logging::log(Logging::INFO, "post-creation GC ok.\r\n");
}

/*
TraceMonkeyValue::TraceMonkeyValue(std::string _value) : ScriptValue(_engine)
{
    debugName = "int value no. " + Utility::toString(__nameCounter);

    Logging::log(Logging::INFO, "Going to create a TM value of: %s\r\n", debugName.c_str());
    INDENT_LOG(Logging::INFO);

    value = INT_TO_JSVAL(_value);
    bool ret = JS_AddNamedRoot(TraceMonkeyEngine::context, &(this->value), debugName.c_str()); // Ensure our value won't be GCed
    assert(ret);

    printJSVAL(value);

    Logging::log(Logging::INFO, "Created a TM value of: %s\r\n", debugName.c_str());

    __nameCounter += 1;

    JS_GC(TraceMonkeyEngine::context); // XXX For debugging purposes
    Logging::log(Logging::INFO, "post-creation GC ok.\r\n");
}
*/

void TraceMonkeyValue::invalidate()
{
    assert(isValid());

    Logging::log(Logging::INFO, "Removing TM root for %s\r\n", debugName.c_str());

    bool success = JS_RemoveRoot(TraceMonkeyEngine::context, &(this->value)); // Allow GCing (unless others use it)
    assert(success);

    JS_GC(TraceMonkeyEngine::context); // XXX For debugging purposes

    ScriptValue::invalidate(); // call parent
}

void TraceMonkeyValue::setProperty(std::string propertyName, ScriptValuePtr propertyValue)
{
    assert(isValid());

    assert(JSVAL_IS_OBJECT(value));
    jsval ret = (dynamic_cast<TraceMonkeyValue*>(propertyValue.get())->value);
    JS_SetProperty(TraceMonkeyEngine::context, JSVAL_TO_OBJECT(value), propertyName.c_str(), &ret);
}

void TraceMonkeyValue::setProperty(std::string propertyName, int propertyValue)
{
    assert(isValid());

    assert(JSVAL_IS_OBJECT(value));
    jsval ret = INT_TO_JSVAL(propertyValue);
    JS_SetProperty(TraceMonkeyEngine::context, JSVAL_TO_OBJECT(value), propertyName.c_str(), &ret);
}

void TraceMonkeyValue::setProperty(std::string propertyName, double propertyValue)
{
    assert(isValid());

    assert(JSVAL_IS_OBJECT(value));
    jsval ret = DOUBLE_TO_JSVAL(propertyValue);
    JS_SetProperty(TraceMonkeyEngine::context, JSVAL_TO_OBJECT(value), propertyName.c_str(), &ret);
}

bool TraceMonkeyValue::hasProperty(std::string propertyName)
{
    assert(isValid());

    assert(JSVAL_IS_OBJECT(value));
    jsval ret;
    assert(JS_GetProperty(TraceMonkeyEngine::context, JSVAL_TO_OBJECT(value), propertyName.c_str(), &ret));
    return JSVAL_IS_VOID(ret);
}

ScriptValuePtr TraceMonkeyValue::getProperty(std::string propertyName)
{
    assert(isValid());

    Logging::log(Logging::DEBUG, "TraceMonkeyValue::getProperty(%s): \r\n", propertyName.c_str()); printJSVAL(value);

    assert(JSVAL_IS_OBJECT(value));
    jsval ret;
    assert(JS_GetProperty(TraceMonkeyEngine::context, JSVAL_TO_OBJECT(value), propertyName.c_str(), &ret));
    bool success = JS_AddNamedRoot(TraceMonkeyEngine::context, &ret, "TraceMonkeyValue::getProperty temp val"); // Ensure our value won't be GCed
    assert(success);

    Logging::log(Logging::DEBUG, "returning: \r\n"); printJSVAL(ret);

    ScriptValuePtr retValue(new TraceMonkeyValue(engine, true, ret));

    success = JS_RemoveRoot(TraceMonkeyEngine::context, &ret); // Allow GCing, it is already marked in the retValue
    assert(success);

    return retValue;
}

int TraceMonkeyValue::getPropertyInt(std::string propertyName)
{
    assert(isValid());

    Logging::log(Logging::DEBUG, "TraceMonkeyValue::getPropertyInt(%s): \r\n", propertyName.c_str()); printJSVAL(value);

    assert(JSVAL_IS_OBJECT(value));
    jsval ret;
    bool success = JS_GetProperty(TraceMonkeyEngine::context, JSVAL_TO_OBJECT(value), propertyName.c_str(), &ret);
    assert(success);
    assert(JSVAL_IS_INT(ret));

    Logging::log(Logging::DEBUG, "returning: \r\n"); printJSVAL(ret);

    return JSVAL_TO_INT(ret);
}

bool TraceMonkeyValue::getPropertyBool(std::string propertyName)
{
    assert(isValid());

    Logging::log(Logging::DEBUG, "TraceMonkeyValue::getPropertyBool(%s): \r\n", propertyName.c_str()); printJSVAL(value);

    assert(JSVAL_IS_OBJECT(value));
    jsval ret;
    bool success =JS_GetProperty(TraceMonkeyEngine::context, JSVAL_TO_OBJECT(value), propertyName.c_str(), &ret);
    assert(success);
    assert(JSVAL_IS_BOOLEAN(ret));

    Logging::log(Logging::DEBUG, "returning: \r\n"); printJSVAL(ret);

    return JSVAL_TO_BOOLEAN(ret);
}

double TraceMonkeyValue::getPropertyFloat(std::string propertyName)
{
    assert(isValid());

    Logging::log(Logging::DEBUG, "TraceMonkeyValue::getPropertyFloat(%s): \r\n", propertyName.c_str()); printJSVAL(value);

    assert(JSVAL_IS_OBJECT(value));
    jsval ret;
    bool success = JS_GetProperty(TraceMonkeyEngine::context, JSVAL_TO_OBJECT(value), propertyName.c_str(), &ret);
    assert(success);
    assert(JSVAL_IS_DOUBLE(ret));

    Logging::log(Logging::DEBUG, "returning: \r\n"); printJSVAL(ret);

    return *(JSVAL_TO_DOUBLE(ret));
}

std::string TraceMonkeyValue::getPropertyString(std::string propertyName)
{
    assert(isValid());

    Logging::log(Logging::DEBUG, "TraceMonkeyValue::getPropertyString(%s): \r\n", propertyName.c_str()); printJSVAL(value);

    assert(JSVAL_IS_OBJECT(value));
    jsval ret;
    bool success = JS_GetProperty(TraceMonkeyEngine::context, JSVAL_TO_OBJECT(value), propertyName.c_str(), &ret);
    assert(success);
    assert(JSVAL_IS_STRING(ret));

    Logging::log(Logging::DEBUG, "returning: \r\n"); printJSVAL(ret);

    return JS_GetStringBytes(JSVAL_TO_STRING(ret));
}

int TraceMonkeyValue::getInt()
{
    assert(isValid());

    Logging::log(Logging::DEBUG, "TraceMonkeyValue::getInt: \r\n"); printJSVAL(value);

    assert(JSVAL_IS_INT(value));
    return JSVAL_TO_INT(value);
}

bool TraceMonkeyValue::getBool()
{
    assert(isValid());

    Logging::log(Logging::DEBUG, "TraceMonkeyValue::getBool: \r\n"); printJSVAL(value);

    assert(JSVAL_IS_BOOLEAN(value));
    return JSVAL_TO_BOOLEAN(value);
}

double TraceMonkeyValue::getFloat()
{
    assert(isValid());

    Logging::log(Logging::DEBUG, "TraceMonkeyValue::getFloat: \r\n"); printJSVAL(value);

    assert(JSVAL_IS_DOUBLE(value));
    return *(JSVAL_TO_DOUBLE(value));
}

std::string TraceMonkeyValue::getString()
{
    assert(isValid());

    Logging::log(Logging::DEBUG, "TraceMonkeyValue::getString: \r\n"); printJSVAL(value);

    assert(JSVAL_IS_STRING(value));
    return JS_GetStringBytes(JSVAL_TO_STRING(value));
}

ScriptValuePtr TraceMonkeyValue::call(std::string funcName)
{
    assert(isValid());

    Logging::log(Logging::DEBUG, "TMV::call(%s)\r\n", funcName.c_str());

    ScriptValueArgs args;
    return call(funcName, args);
}

ScriptValuePtr TraceMonkeyValue::call(std::string funcName, ScriptValuePtr arg1)
{
    assert(isValid());

    Logging::log(Logging::DEBUG, "TMV::call(%s (SV))\r\n", funcName.c_str());

    return call(funcName, ScriptValueArgs().append(arg1));
}

ScriptValuePtr TraceMonkeyValue::call(std::string funcName, int arg1)
{
    assert(isValid());

    Logging::log(Logging::DEBUG, "TMV::call(%s, int)\r\n", funcName.c_str());

    return call(funcName, ScriptValueArgs().append(arg1));
}

ScriptValuePtr TraceMonkeyValue::call(std::string funcName, double arg1)
{
    assert(isValid());

    Logging::log(Logging::DEBUG, "TMV::call(%s, double)\r\n", funcName.c_str());

    return call(funcName, ScriptValueArgs().append(arg1));
}

ScriptValuePtr TraceMonkeyValue::call(std::string funcName, std::string arg1)
{
    assert(isValid());

    return call(funcName, ScriptValueArgs().append(arg1));
}

ScriptValuePtr TraceMonkeyValue::call(std::string funcName, ScriptValueArgs& args)
{
    assert(isValid());

    Logging::log(Logging::DEBUG, "TMV::call(%s, (%d))\r\n", funcName.c_str(), args.args.size());

    printJSVAL(value);

    assert(JSVAL_IS_OBJECT(value));

    jsval func;
    bool success = JS_GetProperty(TraceMonkeyEngine::context, JSVAL_TO_OBJECT(value), funcName.c_str(), &func);
    assert(success);

    assert(JSVAL_IS_OBJECT(func));
    assert(!JSVAL_IS_NULL(func));

    int numArgs = args.args.size();
    jsval jsArgs[numArgs+1]; // +1 for safety in case of 0 args
    for (int i = 0; i < numArgs; i++)
    {
        jsArgs[i] = dynamic_cast<TraceMonkeyValue*>(args.args[i].get())->value;
    }

    jsval ret;
    JS_CallFunctionValue(TraceMonkeyEngine::context, JSVAL_TO_OBJECT(value), func, numArgs, jsArgs, &ret);
    success = JS_AddNamedRoot(TraceMonkeyEngine::context, &ret, "TraceMonkeyValue::call temp val"); // Ensure our value won't be GCed
    assert(success);
    Logging::log(Logging::DEBUG, "returning: \r\n"); printJSVAL(ret);

    ScriptValuePtr retValue(new TraceMonkeyValue(engine, true, ret));

    success = JS_RemoveRoot(TraceMonkeyEngine::context, &ret); // Allow GCing, it is already marked in the retValue
    assert(success);

    return retValue;
}

bool TraceMonkeyValue::compare(ScriptValuePtr other)
{
    assert(isValid());
    assert(other->isValid());

    jsval otherValue = (dynamic_cast<TraceMonkeyValue*>(other.get()))->value;

    if (JSVAL_IS_INT(value))
        return (JSVAL_IS_INT(otherValue) && JSVAL_TO_INT(value) == JSVAL_TO_INT(otherValue));
    else if (JSVAL_IS_NULL(value))
        return (JSVAL_IS_NULL(otherValue));
    else if (JSVAL_IS_VOID(value))
        return (JSVAL_IS_VOID(otherValue));
    else if (JSVAL_IS_BOOLEAN(value))
        return (JSVAL_IS_BOOLEAN(otherValue) && JSVAL_TO_BOOLEAN(value) == JSVAL_TO_BOOLEAN(otherValue));
    else if (JSVAL_IS_DOUBLE(value))
        return (JSVAL_IS_DOUBLE(otherValue) && *(JSVAL_TO_DOUBLE(value)) == *(JSVAL_TO_DOUBLE(otherValue)));
    else if (JSVAL_IS_STRING(value))
    {
        if (!JSVAL_IS_STRING(otherValue))
            return false;
        else
            assert(0 && "TODO: compare TM strings");
    }
    else if (JSVAL_IS_OBJECT(value))
    {
        if (JSVAL_IS_NULL(otherValue)) // NULLs are objects, in TraceMonkey
            return false;
        if (!JSVAL_IS_OBJECT(otherValue))
            return false;
        else
            assert(0 && "TODO: compare TM objects");
    }
    else {
        Logging::log(Logging::INFO, "Can't compare uncertain jsvals\r\n");
    }
    assert(0);
    return false;
}

void TraceMonkeyValue::debugPrint()
{
    if (isValid())
    {
        printJSVAL(value);
    } else {
        Logging::log(Logging::DEBUG, "TraceMonkeyValue::debugPrint: inValidated value\r\n");
    }
}


// Internals

void reportError(JSContext *context, const char *message, JSErrorReport *report)
{
    Logging::log(Logging::WARNING, "TraceMonkey error:\r\n");

    Logging::log(Logging::WARNING, "%s:%u:%s      Code: %s\r\n",
            report->filename ? report->filename : "<no filename>",
            (unsigned int) report->lineno,
            message,
            report->linebuf);

    assert(0 && "Quitting due to Scripting error"); // TODO: remove in the future
}

/* The class of the global object. */
static JSClass global_class = {
    "global", JSCLASS_GLOBAL_FLAGS,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
    JSCLASS_NO_OPTIONAL_MEMBERS
};

// Statics (make instanced at some point?)

JSRuntime *TraceMonkeyEngine::runtime = NULL;
JSContext *TraceMonkeyEngine::context = NULL;
JSObject  *TraceMonkeyEngine::global = NULL;

// Global wrappers

ScriptValuePtr globalValue;

// Revealed

void TraceMonkeyEngine::init()
{
    Logging::log(Logging::DEBUG, "TraceMonkeyEngine::init\r\n");
    INDENT_LOG(Logging::DEBUG);

    assert(runtime == NULL);

    runtime = JS_NewRuntime(8L * 1024L * 1024L); // Force GC after X MB.
    if (runtime == NULL)
    {
        Logging::log(Logging::ERROR, "Cannot create TraceMonkey runtime\r\n");
        assert(0);
    }

    /* Create a context. */
    context = JS_NewContext(runtime, 8192);
    if (context == NULL)
    {
        Logging::log(Logging::ERROR, "Cannot create TraceMonkey runtime\r\n");
        assert(0);
    }

    JS_SetOptions(context, JSOPTION_VAROBJFIX);
    JS_SetVersion(context, JSVERSION_ECMA_3);
    JS_SetErrorReporter(context, reportError);

    // Create the global object. Store it locally for a while until it is usable by others
    JSObject* _global = JS_NewObject(context, &global_class, NULL, NULL);
    if (_global == NULL)
    {
        Logging::log(Logging::ERROR, "Cannot create TraceMonkey runtime\r\n");
        assert(0);
    }

    /* Populate the global object with the standard globals,
       like Object and Array. */
    if (!JS_InitStandardClasses(context, _global))
    {
        Logging::log(Logging::ERROR, "Cannot create TraceMonkey runtime\r\n");
        assert(0);
    }

    // Create our internal wrappers

    globalValue = ScriptValuePtr(new TraceMonkeyValue( this, true, OBJECT_TO_JSVAL(_global) ));
    global = _global; // Now that globalValue is set, it is usable by others, so reveal it

    // JITting
//    JS_ToggleOptions(context, JSOPTION_JIT); // Might be buggy, wait for 1.8 to release

// JS_MaybeGC - call during idle time in the main loop? Or other method?

// Use JS_SetContextPrivate and JS_GetContextPrivate to associate application-specific data with a context.
//  - Useful for security checks, once we have a context for the system and sandboxed contexts
//    for user scripts

    // Debugging features

    JS_SetGCZeal(context, 2); // XXX This is 'extremely high' - make a parameter, see MDC docs
}

void TraceMonkeyEngine::quit()
{
    Logging::log(Logging::DEBUG, "TraceMonkeyEngine::quit\r\n");

    // Clean up our internal wrappers

    globalValue.reset();

    /* Cleanup. */
    JS_DestroyContext(context);
    context = NULL;

    JS_DestroyRuntime(runtime);
    runtime = NULL;

//    JS_ShutDown(); XXX Should be done only when NO runtimes remain, for final cleanup, as we are quitting probably
}

ScriptValuePtr TraceMonkeyEngine::createObject()
{
    JSObject* obj = JS_NewObject(context, NULL, NULL, NULL);
    bool success = JS_AddNamedRoot(context, &obj, "TraceMonkeyEngine::createObject temp val"); // Ensure our value won't be GCed
    assert(success);

    ScriptValuePtr ret( new TraceMonkeyValue( this,  true, OBJECT_TO_JSVAL(obj) ) );

    success = JS_RemoveRoot(context, &obj);
    assert(success);

    return ret;
}

ScriptValuePtr TraceMonkeyEngine::createFunction(NativeFunction func, int numArgs)
{
    JSFunction* jsFunc = JS_NewFunction(context, (JSNative)func, numArgs, 0, NULL, NULL);

    bool success = JS_AddNamedRoot(context, &jsFunc, "TraceMonkeyEngine::createFunction temp val");; // Ensure our value won't be GCed
    assert(success);

    assert(jsFunc != NULL);
    assert(JSVAL_IS_OBJECT(OBJECT_TO_JSVAL(jsFunc)));

    ScriptValuePtr ret( new TraceMonkeyValue( this, true, OBJECT_TO_JSVAL(jsFunc) ) );

    success = JS_RemoveRoot(context, &jsFunc);
    assert(success);

    return ret;

}

ScriptValuePtr TraceMonkeyEngine::createScriptValue(int value)
{
    return ScriptValuePtr( new TraceMonkeyValue( this, value ) );
}

ScriptValuePtr TraceMonkeyEngine::createScriptValue(double value)
{
    return ScriptValuePtr( new TraceMonkeyValue( this, value ) );
}

ScriptValuePtr TraceMonkeyEngine::createScriptValue(std::string value)
{
    // XXX Horrible, simply horrible... but sandboxed, so at least secure
    return runScript("'" + value + "'");
}

ScriptValuePtr TraceMonkeyEngine::getGlobal()
{
    assert(globalValue.get());

    Logging::log(Logging::DEBUG, "TME::getGlobal\r\n");
//    ((TraceMonkeyValue*)(globalValue.get()))->debugPrint();

    return globalValue;
}

ScriptValuePtr TraceMonkeyEngine::getNull()
{
    return ScriptValuePtr( new TraceMonkeyValue(this) );
}

ScriptValuePtr TraceMonkeyEngine::runScript(std::string script)
{
//    printf("Running script: \r\n%s\r\n", script.c_str());

    jsval ret;
    JS_EvaluateScript(context, global, script.c_str(), script.length(), "TraceMonkeyEngine", 0, &ret);
    bool success = JS_AddNamedRoot(context, &ret, "TraceMonkeyEngine::runScript temp val");; // Ensure our value won't be GCed
    assert(success);

//    printf("Script completed\r\n");

    printJSVAL(ret);

    ScriptValuePtr retValue;

    if ( !JSVAL_IS_VOID(ret) )
        retValue = ScriptValuePtr( new TraceMonkeyValue(this, true, ret) );

    success = JS_RemoveRoot(context, &ret);
    assert(success);

    return retValue;
}

LogicEntityPtr TraceMonkeyEngine::getCLogicEntity(JSObject* scriptingEntity)
{
    // We do this in a slow but sure manner: read the uniqueId from JS,
    // and look up the entity using that. In the future, speed this up
    // using private data or some other method

    jsval temp;
    bool success = JS_GetProperty(context, scriptingEntity, "uniqueId", &temp);
    assert(success);
    assert(JSVAL_IS_INT(temp));

    int uniqueId = JSVAL_TO_INT(temp);
    LogicEntityPtr ret = LogicSystem::getLogicEntity(uniqueId);

    Logging::log(Logging::DEBUG, "TraceMonkey getting the CLE for UID %d\r\n", uniqueId);

    assert(ret.get());

    return ret;
}

