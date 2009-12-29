
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


#include "cube.h"
#include "engine.h"
#include "game.h"

#include "utility.h"

#include "script_engine_manager.h"
#include "script_engine_v8.h"

#ifdef SERVER
    #include "server_system.h"
#endif

//using namespace v8;

//#include "v8-debug.h"


///////// Debug

void printV8Value(Handle<Value> value, bool force=false)
{
    Logging::Level level = force ? Logging::ERROR : Logging::INFO;

    if (!Logging::shouldShow(level)) return;

    HandleScope handleScope;

    if (value.IsEmpty())
        Logging::log(level, "Empty handle\r\n");
    else if (value->IsInt32())
        Logging::log(level, "INT: %d\r\n", value->IntegerValue());
    else if (value->IsNull())
        Logging::log(level, "NULL (null)\r\n");
    else if (value->IsUndefined())
        Logging::log(level, "VOID (undefined)\r\n");
    else if (value->IsBoolean())
        Logging::log(level, "BOOLEAN: %d\r\n", value->BooleanValue());
    else if (value->IsNumber())
        Logging::log(level, "NUMBER: %f\r\n", value->NumberValue());
    else if (value->IsString())
        Logging::log(level, "STRING: ?\r\n");
    else if (value->IsObject())
        Logging::log(level, "OBJECT (object)\r\n");
    else
        Logging::log(level, "Uncertain V8 value\n");
}


void handleException(TryCatch& tc)
{
    Logging::log(Logging::ERROR, "V8 exception:\r\n");

    HandleScope handleScope;

    Handle<Object> exception = tc.Exception()->ToObject();
    String::AsciiValue exception_str(exception);

    Logging::log(Logging::ERROR, "            : %s\r\n", *exception_str);

/*
    Handle<Array> names = exception->GetPropertyNames();
    for (unsigned int i = 0; i < names->Length(); i++)
    {
        std::string strI = Utility::toString((int)i);
        Logging::log(Logging::ERROR, "    %d : %s : %s\r\n", i,
            *(v8::String::Utf8Value(names->Get(String::New(strI.c_str()))->ToString())),
            *(v8::String::Utf8Value(exception->Get(names->Get(String::New(strI.c_str()))->ToString())->ToString()))
        );
    }
*/

    Local<Message> message = tc.Message();

    Logging::log(Logging::ERROR, "Message: Get: %s\r\n", *(v8::String::Utf8Value( message->Get() )));
    Logging::log(Logging::ERROR, "Message: GetSourceLine: %s\r\n", *(v8::String::Utf8Value( message->GetSourceLine() )));
    Logging::log(Logging::ERROR, "Message: GetScriptResourceName: %s\r\n", *(v8::String::Utf8Value( message->GetScriptResourceName()->ToString() )));
    Logging::log(Logging::ERROR, "Message: GetLineNumber: %d\r\n", message->GetLineNumber() );

    Local<Value> stackTrace = tc.StackTrace();
    if (!stackTrace.IsEmpty())
    {
        Logging::log(Logging::ERROR, "Stack trace: %s\r\n", *(v8::String::Utf8Value( stackTrace->ToString() )));
        printf("\r\n\r\n^Stack trace^: %s\r\n", *(v8::String::Utf8Value( stackTrace->ToString() )));
    }
    else
        Logging::log(Logging::ERROR, "No stack trace available in C++ handler (see above for possible in-script stack trace)\r\n");

    #ifdef SERVER
        std::string clientMessage = *(v8::String::Utf8Value( message->Get() ));
        clientMessage += "  -  ";
        clientMessage += *(v8::String::Utf8Value( message->GetSourceLine() ));
        ServerSystem::fatalMessageToClients(clientMessage);
    #endif

//    assert(0);
    throw ScriptException("Bad!");
}


///////// Values

int __nameCounter = 0;

V8Value::V8Value(ScriptEngine* _engine) : ScriptValue(_engine)
{
    value = Persistent<Value>::New(Null());

    debugName = "NULL value no. " + Utility::toString(__nameCounter);

    Logging::log(Logging::INFO, "Going to create a V8 value of: %s\r\n", debugName.c_str());
    INDENT_LOG(Logging::INFO);

    printV8Value(value);

    Logging::log(Logging::INFO, "Created a V8 value of: %s\r\n", debugName.c_str());

    __nameCounter += 1;
}

V8Value::V8Value(ScriptEngine* _engine, int _value) : ScriptValue(_engine)
{
    debugName = "int value no. " + Utility::toString(__nameCounter);

    Logging::log(Logging::INFO, "Going to create a V8 value of: %s\r\n", debugName.c_str());
    INDENT_LOG(Logging::INFO);

    value = Persistent<Value>::New(Integer::New(_value));

    printV8Value(value);

    Logging::log(Logging::INFO, "Created a V8 value of: %s\r\n", debugName.c_str());

    __nameCounter += 1;
}

V8Value::V8Value(ScriptEngine* _engine, double _value) : ScriptValue(_engine)
{
    debugName = "double value no. " + Utility::toString(__nameCounter);

    Logging::log(Logging::INFO, "Going to create a V8 value of: %s\r\n", debugName.c_str());
    INDENT_LOG(Logging::INFO);

    value = Persistent<Value>::New(Number::New(_value));

    printV8Value(value);

    Logging::log(Logging::INFO, "Created a V8 value of: %s\r\n", debugName.c_str());

    __nameCounter += 1;
}

V8Value::V8Value(ScriptEngine* _engine, Handle<Value> _value) : ScriptValue(_engine)
{
    debugName = "Handle<Value> value no. " + Utility::toString(__nameCounter);

    Logging::log(Logging::INFO, "Going to create a V8 reference: %s\r\n", debugName.c_str());
    INDENT_LOG(Logging::INFO);

    value = Persistent<Value>::New(_value);

    printV8Value(value);

    Logging::log(Logging::INFO, "Created a V8 reference: %s\r\n", debugName.c_str());

    __nameCounter += 1;
}

/*
V8Value::V8Value(std::string _value) : ScriptValue(_engine)
{
    debugName = "int value no. " + Utility::toString(__nameCounter);

    Logging::log(Logging::INFO, "Going to create a V8 value of: %s\r\n", debugName.c_str());
    INDENT_LOG(Logging::INFO);

    value = String::New(_value);

    printV8Value(value);

    Logging::log(Logging::INFO, "Created a V8 value of: %s\r\n", debugName.c_str());

    __nameCounter += 1;
}
*/

V8Value::~V8Value()
{
    Logging::log(Logging::INFO, "~V8Value\r\n");

    Logging::log(Logging::INFO, "Removing V8 handle for %s\r\n", debugName.c_str());

//    if (value != Null()) XXX?
    value.Dispose();
}

void V8Value::setProperty(std::string propertyName, ScriptValuePtr propertyValue)
{
    assert(isValid());
    assert(value->IsObject() && !value->IsNull());

    HandleScope handleScope;

    Handle<Object> obj = value->ToObject();
    obj->Set(String::New(propertyName.c_str()), dynamic_cast<V8Value*>(propertyValue.get())->value);
}

void V8Value::setProperty(std::string propertyName, int propertyValue)
{
    HandleScope handleScope;
    setProperty(propertyName, ScriptValuePtr(new V8Value(engine, Integer::New(propertyValue))));
}

void V8Value::setProperty(std::string propertyName, double propertyValue)
{
    HandleScope handleScope;
    setProperty(propertyName, ScriptValuePtr(new V8Value(engine, Number::New(propertyValue))));
}

void V8Value::setProperty(std::string propertyName, std::string propertyValue)
{
    HandleScope handleScope;
    setProperty(propertyName, ScriptValuePtr(new V8Value(engine, String::New(propertyValue.c_str()))));
}

bool V8Value::hasProperty(std::string propertyName)
{
    HandleScope handleScope;

    Local<Object> obj = value->ToObject();

    Local<Value> val = obj->Get(String::New(propertyName.c_str()));
    return !(val->IsNull() || val->IsUndefined()); // These are not 'true' values, that we can return in e.g. getPropertyString
                                                   // (we can't return 'Null' there - so this is for both undefined and null)
}

ScriptValuePtr V8Value::getProperty(std::string propertyName)
{
    HandleScope handleScope;
    Local<Object> obj = value->ToObject();
    Local<Value> propertyValue = obj->Get(String::New(propertyName.c_str()));

    return ScriptValuePtr(new V8Value(engine, propertyValue));
}

#define ASSERT_VALID_PROPERTY assert(!(dynamic_cast<V8Value*>(getProperty(propertyName).get())->value->IsNull()) && !(dynamic_cast<V8Value*>(getProperty(propertyName).get())->value->IsUndefined()));

int V8Value::getPropertyInt(std::string propertyName)
{
    HandleScope handleScope;
    ASSERT_VALID_PROPERTY
    return dynamic_cast<V8Value*>(getProperty(propertyName).get())->value->IntegerValue();
}

bool V8Value::getPropertyBool(std::string propertyName)
{
    HandleScope handleScope;
    ASSERT_VALID_PROPERTY
    return dynamic_cast<V8Value*>(getProperty(propertyName).get())->value->BooleanValue();
}

double V8Value::getPropertyFloat(std::string propertyName)
{
//printf("getPropertyFloat: %s\r\n", propertyName.c_str()); // collisionRadiusWidth ?
    HandleScope handleScope;
    ASSERT_VALID_PROPERTY
    return dynamic_cast<V8Value*>(getProperty(propertyName).get())->value->NumberValue();
}

std::string V8Value::getPropertyString(std::string propertyName)
{
    HandleScope handleScope;
    ASSERT_VALID_PROPERTY
    V8Value *temp = dynamic_cast<V8Value*>(getProperty(propertyName).get());
    std::string ret = *(v8::String::Utf8Value(temp->value->ToString()));
    return ret;
}

#define ASSERT_VALID_VALUE assert(!value->IsNull() && !value->IsUndefined());

int V8Value::getInt()
{
    HandleScope handleScope;
    ASSERT_VALID_VALUE
    return value->IntegerValue();
}

bool V8Value::getBool()
{
    HandleScope handleScope;
    ASSERT_VALID_VALUE
    return value->BooleanValue();
}

double V8Value::getFloat()
{
    HandleScope handleScope;
    ASSERT_VALID_VALUE
    return value->NumberValue();
}

std::string V8Value::getString()
{
    HandleScope handleScope;
    ASSERT_VALID_VALUE
    std::string ret = *(v8::String::Utf8Value(value->ToString()));
    return ret;
}

ScriptValuePtr V8Value::call(std::string funcName)
{
    assert(isValid());

    Logging::log(Logging::INFO, "V8V::call(%s)\r\n", funcName.c_str());

    ScriptValueArgs args;
    return call(funcName, args);
}

ScriptValuePtr V8Value::call(std::string funcName, ScriptValuePtr arg1)
{
    assert(isValid());

    Logging::log(Logging::INFO, "V8V::call(%s (SV))\r\n", funcName.c_str());

    return call(funcName, ScriptValueArgs().append(arg1));
}

ScriptValuePtr V8Value::call(std::string funcName, int arg1)
{
    assert(isValid());

    Logging::log(Logging::INFO, "V8V::call(%s, int)\r\n", funcName.c_str());

    return call(funcName, ScriptValueArgs().append(arg1));
}

ScriptValuePtr V8Value::call(std::string funcName, double arg1)
{
    assert(isValid());

    Logging::log(Logging::INFO, "V8V::call(%s, double)\r\n", funcName.c_str());

    return call(funcName, ScriptValueArgs().append(arg1));
}

ScriptValuePtr V8Value::call(std::string funcName, std::string arg1)
{
    assert(isValid());

    return call(funcName, ScriptValueArgs().append(arg1));
}

ScriptValuePtr V8Value::call(std::string funcName, ScriptValueArgs& args)
{
    HandleScope handleScope;

    assert(isValid());

    Logging::log(Logging::INFO, "V8V::call(%s, (%d))\r\n", funcName.c_str(), args.args.size());

    printV8Value(value);

    assert(value->IsObject() && !value->IsNull());

    Local<Object> obj = value->ToObject();

    Local<Object> _func = obj->Get(String::New(funcName.c_str()))->ToObject();
    assert(_func->IsFunction());

    Local<Function> func = Function::Cast(*_func);

    int numArgs = args.args.size();
    Handle<Value>* v8Args = new Handle<Value>[numArgs+1]; // +1 for safety in case of 0 args
    for (int i = 0; i < numArgs; i++)
    {
        v8Args[i] = (dynamic_cast<V8Value*>(args.args[i].get()))->value;
    }

    TryCatch tc;
    Handle<Value> ret = func->Call(obj, numArgs, v8Args);
    delete[] v8Args;
    if (ret.IsEmpty())
        handleException(tc);

    Logging::log(Logging::INFO, "returning: \r\n"); printV8Value(ret);

    ScriptValuePtr retValue(new V8Value(engine, ret));

    return retValue;
}

bool V8Value::compare(ScriptValuePtr other)
{
    assert(isValid());
    assert(other->isValid());

    return value->StrictEquals( (dynamic_cast<V8Value*>(other.get()))->value );
}

void V8Value::debugPrint()
{
    if (isValid())
    {
        printV8Value(value);
    } else {
        Logging::log(Logging::DEBUG, "V8Value::debugPrint: inValidated value\r\n");
    }
}


// Internals

#if 0
void debuggerListener(DebugEvent event,
                       Handle<Object> exec_state,
                       Handle<Object> event_data,
                       Handle<Value> data)
{
    printf("************************ V8 debugger ***********************\r\n");

    HandleScope handleScope;

    Local<String> source = String::New(
        "temp = function (exec_state) { "
        "   log(WARNING, 'halp now'); "
        "   return serializeJSON(exec_state); "
        "} "
    );
    Local<Script> code = Script::Compile(source);
    Local<Value> result = code->Run();
//    printV8Value(result, true);
    assert(result->IsObject());
    Local<Object> obj = result->ToObject();
    assert(obj->IsFunction());
    Local<Function> func = Function::Cast(*obj);
    Handle<Value> output = Debug::Call(func, exec_state);
    printV8Value(output, true);


    printV8Value(exec_state, true);
    printV8Value(event_data, true);
    printV8Value(data, true);

/*

    Local<Object> _func = obj->Get(String::New(funcName.c_str()))->ToObject();
    assert(_func->IsFunction());

    Local<Function> func = Function::Cast(*_func);
*/


    assert(0);
}
#endif


// Statics (make instanced at some point?)

Persistent<Context> V8Engine::context;

// Globals

//Persistent<ObjectTemplate> _global;
ScriptValuePtr globalValue;

// Revealed

void V8Engine::init()
{
    Logging::log(Logging::DEBUG, "V8Engine::init:\r\n");
    INDENT_LOG(Logging::DEBUG);

    HandleScope handleScope;

    // Create a template for the global object.
    Logging::log(Logging::DEBUG, "Creating global\r\n");

//    _global = ObjectTemplate::New();

    Logging::log(Logging::DEBUG, "Creating context\r\n");

    context = Context::New(); //NULL, _global);

    context->Enter();

    // Create our internal wrappers

    Logging::log(Logging::DEBUG, "Creating wrapper for global\r\n");

    globalValue = ScriptValuePtr(new V8Value(this, context->Global()));

#if 0
    // Setup debugger, if required - XXX Doesn't work

//    Debug::SetDebugEventListener(debuggerListener);
//    runFile("src/thirdparty/v8/src/debug-delay.js"); // FIXME: remove hardcoded src/..
    runFile("src/javascript/intensity/V8Debugger.js"); // FIXME: remove hardcoded src/javascript
             
    v8::Handle<v8::Value> result =
        ((V8Value*)(getGlobal()->getProperty("__debuggerListener").get()))->value;
    assert(result->IsObject());
    Local<Object> debuggerListener = result->ToObject();
    assert(debuggerListener->IsFunction());

    Debug::SetDebugEventListener(debuggerListener);

    runScript("Debug.setBrzzzzzzzzzeakOnException()");

    Local<String> source = String::New(
        "temp = function () { Debug.setBreakOnException(); return 5098; } "
    );
    Local<Script> code = Script::Compile(source);
    Local<Value> result2 = code->Run();
    printV8Value(result2, true);
    assert(result2->IsObject());
    Local<Object> obj = result2->ToObject();
    assert(obj->IsFunction());
    Local<Function> func = Function::Cast(*obj);
    Handle<Value> output = Debug::Call(func);
    printV8Value(output, true);
assert(0);
#endif

    Logging::log(Logging::DEBUG, "V8Engine::init complete.\r\n");
}

void V8Engine::quit()
{
    Logging::log(Logging::DEBUG, "V8Engine::quit (0)\r\n");

    // Clean up our globals

    globalValue.reset();

    Logging::log(Logging::DEBUG, "V8Engine::quit (1)\r\n");

    context->Exit();

    Logging::log(Logging::DEBUG, "V8Engine::quit (2)\r\n");

    context.Dispose();

    Logging::log(Logging::DEBUG, "V8Engine::quit (3)\r\n");
}

ScriptValuePtr V8Engine::createObject()
{
    HandleScope handleScope;

    Local<Value> ret = Object::New();

    return ScriptValuePtr( new V8Value(this, ret) );
}

ScriptValuePtr V8Engine::createFunction(NativeFunction func, int numArgs)
{
    HandleScope handleScope;

    // Clumsy, but not sure how else
    Local<FunctionTemplate> t = FunctionTemplate::New((InvocationCallback)func);

    Local<Function> v8Func = t->GetFunction();

    return ScriptValuePtr( new V8Value(this, v8Func) );
}

ScriptValuePtr V8Engine::createScriptValue(int value)
{
    HandleScope handleScope;

    Local<Value> ret = Integer::New(value);

    return ScriptValuePtr( new V8Value(this, ret) );
}

ScriptValuePtr V8Engine::createScriptValue(double value)
{
    HandleScope handleScope;

    Local<Value> ret = Number::New(value);

    return ScriptValuePtr( new V8Value(this, ret) );
}

ScriptValuePtr V8Engine::createScriptValue(std::string value)
{
    HandleScope handleScope;

    Local<Value> ret = String::New(value.c_str());

    return ScriptValuePtr( new V8Value(this, ret) );
}

ScriptValuePtr V8Engine::getGlobal()
{
    assert(globalValue.get());

    Logging::log(Logging::INFO, "V8E::getGlobal\r\n");
//    ((V8Value*)(globalValue.get()))->debugPrint();

    return globalValue;
}

ScriptValuePtr V8Engine::getNull()
{
    return ScriptValuePtr( new V8Value(this) );
}

ScriptValuePtr V8Engine::runScript(std::string script, std::string identifier)
{
    HandleScope handleScope;
    TryCatch tc;

    Local<String> source = String::New(script.c_str());

    // Build data

    ScriptOrigin origin(String::New(identifier.c_str()));

    // Compile the source code.

    Local<Script> code = Script::Compile(source, &origin);
    if (code.IsEmpty())
        handleException(tc);
  
    // Run the script to get the result.

    Local<Value> result = code->Run();
    if (result.IsEmpty())
        handleException(tc);

    return ScriptValuePtr( new V8Value(this, result) );
}

std::string V8Engine::compileScript(std::string script)
{
    HandleScope handleScope;
    TryCatch tc;

    Local<String> source = String::New(script.c_str());

    // Compile the source code.

    Local<Script> code = Script::Compile(source);
    if (!code.IsEmpty())
        return "";

    // There were errors, return them

    std::string ret = "";

    Handle<Object> exception = tc.Exception()->ToObject();
    String::AsciiValue exception_str(exception);

    ret += *exception_str; ret += "\n";

    Local<Message> message = tc.Message();

    ret += *(v8::String::Utf8Value( message->Get() )); ret += "\n";
    ret += "Source line: "; ret += *(v8::String::Utf8Value( message->GetSourceLine() )); ret += "\n";
    ret += "Source line number: "; ret += Utility::toString(message->GetLineNumber()); ret += "\n";

    return ret;
}

////#include "system_manager.h"

LogicEntityPtr V8Engine::getCLogicEntity(Handle<Object> scriptingEntity)
{
////                static Benchmarker benchmarker;
////                benchmarker.start();
    LogicEntityPtr ret;

//  for (int i = 0; i < 10; i++) // For speed testing/profiling - simple way to see how much effect this has, by worsening it
  { 
    // We do this in a slow but sure manner: read the uniqueId from JS,
    // and look up the entity using that. In the future, speed this up
    // using private data or some other method

    int uniqueId = scriptingEntity->Get(String::New("uniqueId"))->IntegerValue();

    ret = LogicSystem::getLogicEntity(uniqueId);

    Logging::log(Logging::INFO, "V8 getting the CLE for UID %d\r\n", uniqueId);

////                benchmarker.stop();
////                SystemManager::showBenchmark("        ---V8lookup---", benchmarker);

    if (!ret.get())
    {
        Logging::log(Logging::ERROR, "Cannot find CLE for entity %d\r\n", uniqueId);
        printV8Value(scriptingEntity, true);
    }
  }
    return ret;
}

