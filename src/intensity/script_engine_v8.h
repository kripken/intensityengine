
// Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
// This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

#include "v8.h"

using namespace v8;

class V8Value : public ScriptValue
{
public:
    Persistent<Value> value;
    std::string debugName;

    V8Value(ScriptEngine* _engine);
    V8Value(ScriptEngine* _engine, int _value);
    V8Value(ScriptEngine* _engine, double _value);
//    V8Value(std::string _value);
    V8Value(ScriptEngine* _engine, Handle<Value> _value);

    ~V8Value();

    virtual void setProperty(std::string propertyName, ScriptValuePtr propertyValue);
    virtual void setProperty(std::string propertyName, int propertyValue);
    virtual void setProperty(std::string propertyName, double propertyValue);
    virtual void setProperty(std::string propertyName, std::string propertyValue);

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

class V8Engine : public ScriptEngine
{
public:
    static Persistent<Context> context;

    virtual void init();
    virtual void quit();

    virtual ScriptValuePtr createObject();
    virtual ScriptValuePtr createFunction(NativeFunction func, int numArgs);
    virtual ScriptValuePtr createScriptValue(int value);
    virtual ScriptValuePtr createScriptValue(double value);
    virtual ScriptValuePtr createScriptValue(std::string value);

    virtual ScriptValuePtr getGlobal();
    virtual ScriptValuePtr getNull();

    virtual ScriptValuePtr runScript(std::string script, std::string identifier="");

    virtual std::string compileScript(std::string script);

    static LogicEntityPtr getCLogicEntity(Handle<Object> scriptingEntity);
};


#define RAISE_SCRIPT_ERROR(text) \
    { \
        Logging::log(Logging::ERROR, #text); \
        ScriptEngineManager::runScript("eval(assert(' false '))"); \
    }

// Function wrappers. See comments in script_engine_manager.cpp,
// in particular for why we have _is etc. separately.

// Generic wrapper
#define V8_FUNC_GEN(new_func, arguments_def, wrapped_code) \
v8::Handle<v8::Value> new_func(const v8::Arguments& args) \
{ \
    Logging::log(Logging::INFO, "V8F: %s\r\n", #new_func); \
    arguments_def; \
 \
    wrapped_code; \
 \
    return Undefined(); /* if we didn't return anything before */ \
}

// Wrap a function with no parameters
#define V8_FUNC_NOPARAM(new_func, wrapped_code) \
    V8_FUNC_GEN(new_func, , wrapped_code);

#include "script_engine_v8_autogenerated.h"

//
// Objects
//

// Wrap a function with a Object interpreted as "this", converted into a LogicEntityPtr, and some other parameters
#define V8_FUNC_T(new_func, type_codes, wrapped_code) \
    V8_FUNC_o##type_codes(new_func, { \
        Logging::log(Logging::INFO, "V8F_T: %s\r\n", #new_func); \
        LogicEntityPtr self = V8Engine::getCLogicEntity(arg1); \
        if (!self.get()) \
        { \
            Logging::log(Logging::ERROR, "Cannot find LE for entity - aborting %s\r\n", #new_func); \
            RAISE_SCRIPT_ERROR("Aborted."); \
        } \
        wrapped_code; \
    });

// Wrap a function with a Object interpreted as "this", converted into a ScriptValuePtr, and some other parameters
#define V8_FUNC_Z(new_func, type_codes, wrapped_code) \
    V8_FUNC_o##type_codes(new_func, { \
        Logging::log(Logging::INFO, "V8F_Z: %s\r\n", #new_func); \
        ScriptValuePtr self(new V8Value(ScriptEngineManager::getEngine(), arg1)); \
        wrapped_code; \
    });

// Return values
#define V8_RETURN_NULL \
    { \
        return Null(); \
    }

#define V8_RETURN_INT(value)\
    { \
        return Integer::New(value); \
    }

#define V8_RETURN_DOUBLE(value)\
    { \
        return Number::New(value); \
    }

#define V8_RETURN_BOOL(value)\
    { \
        return Boolean::New(value); \
    }

#define V8_RETURN_STRING(value)\
    { \
        return String::New(value); \
    }

#define V8_RETURN_VALUE(_value)\
    { \
        return ((V8Value*)_value.get())->value; \
    }

#define V8_RETURN_FARRAY(_value, num)\
    { \
        Handle<Object> _ret = Array::New(num); \
        for (unsigned int i = 0; i < num; i++) \
            _ret->Set(Number::New(i), Number::New(_value[i])); \
        return _ret; \
    }

