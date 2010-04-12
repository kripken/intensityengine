
// Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
// This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

#include <vector>
#include <set>

class ScriptValue;

typedef boost::shared_ptr<ScriptValue> ScriptValuePtr;

class ScriptEngine;

struct ScriptValueArgs
{
    std::vector<ScriptValuePtr> args;
    ScriptEngine* engine;

    ScriptValueArgs();
    ScriptValueArgs(ScriptEngine* _engine) : engine(_engine) { };

    ScriptValueArgs& append(int value);
    ScriptValueArgs& append(bool value);
    ScriptValueArgs& append(double value);
    ScriptValueArgs& append(std::string value);
    ScriptValueArgs& append(ScriptValuePtr value);
};

//! A wrapper for a scripting value. We are wrapping scripting languages,
//! specifically dynamic ones, so these values can represent any possible
//! value in the scripting language.
class ScriptValue
{
protected:
    ScriptEngine* engine;
public:
    ScriptValue(ScriptEngine* _engine);
    virtual ~ScriptValue();

    // Called when the engine inside which we are a value has been destroyed. This
    // should perform necessary cleanup. In particular, this function in ScriptValue
    // marks the value as invalid, so calls to isValid return false.
    void invalidate();

    bool isValid();

    virtual void setProperty(std::string propertyName, ScriptValuePtr propertyValue) = 0;
    virtual void setProperty(std::string propertyName, int propertyValue) = 0;
    virtual void setProperty(std::string propertyName, double propertyValue) = 0;
    virtual void setProperty(std::string propertyName, std::string propertyValue) = 0;

    virtual bool hasProperty(std::string propertyName) = 0;

    virtual ScriptValuePtr getProperty(std::string propertyName) = 0;
    virtual int getPropertyInt(std::string propertyName) = 0;
    virtual bool getPropertyBool(std::string propertyName) = 0;
    virtual double getPropertyFloat(std::string propertyName) = 0;
    virtual std::string getPropertyString(std::string propertyName) = 0;

    virtual int getInt() = 0;
    virtual bool getBool() = 0;
    virtual double getFloat() = 0;
    virtual std::string getString() = 0;

    virtual ScriptValuePtr call(std::string funcName) = 0;
    virtual ScriptValuePtr call(std::string funcName, ScriptValuePtr arg1) = 0;
    virtual ScriptValuePtr call(std::string funcName, int arg1) = 0;
    virtual ScriptValuePtr call(std::string funcName, double arg1) = 0;
    virtual ScriptValuePtr call(std::string funcName, std::string arg1) = 0;
    virtual ScriptValuePtr call(std::string funcName, ScriptValueArgs& args) = 0;

    //! Returns true if equal to another ScriptValue
    virtual bool compare(ScriptValuePtr other) = 0;

    //! For debug purposes, dump the contents of this value to the log.
    virtual void debugPrint() = 0;
};

typedef void (*NativeFunction)();

//! Generic interface to a scripting engine
class ScriptEngine
{
    std::string scriptDir;

    typedef std::set<ScriptValue*> ScriptValueStore;
    ScriptValueStore registeredScriptValues;

public:
    ScriptEngine() : scriptDir("") { };
    ~ScriptEngine();

    virtual void init() = 0;
    virtual void quit() = 0;

    virtual ScriptValuePtr createObject() = 0;
    virtual ScriptValuePtr createFunction(NativeFunction func, int numArgs) = 0;
    virtual ScriptValuePtr createScriptValue(int value) = 0;
    virtual ScriptValuePtr createScriptValue(double value) = 0;
    virtual ScriptValuePtr createScriptValue(std::string value) = 0;

    virtual ScriptValuePtr getGlobal() = 0;
    virtual ScriptValuePtr getNull() = 0;

    void registerScriptValue(ScriptValue* value);
    void unregisterScriptValue(ScriptValue* value);

    virtual ScriptValuePtr runScript(std::string script, std::string identifier="") = 0;

    //! Compiles a script. Useful for testing syntax. Returns compilation errors, or ""
    virtual std::string compileScript(std::string script) = 0;
};


// Base exception for script errors
class ScriptException: public std::exception
{
public:
    std::string text;
    ScriptException(std::string _text) throw() { text = _text; };
    ~ScriptException() throw() { };
};

