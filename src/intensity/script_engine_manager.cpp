
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

#include "world_system.h"
#include "message_system.h"
#include "utility.h"
#include "fpsclient_interface.h"
#ifdef CLIENT
    #include "client_system.h"
    #include "targeting.h"
#endif

#include "script_engine_manager.h"

//#include "script_engine_tracemonkey.h"
#include "script_engine_v8.h"


// Constants

static std::string SCRIPT_DIR = "src/javascript/";


// Embedded functions: Stuff exposed to scripting.

#include "script_engine_embedding.h"



// Privates

ScriptEngine* ScriptEngineManager::engine = NULL;

// Statics

std::map<std::string, std::string> ScriptEngineManager::engineParameters;

/// Modules

/// Logging module
void ScriptEngineManager::setupLogging(bool runTests)
{
    ScriptValuePtr module = engine->createObject();
    engine->getGlobal()->setProperty("Logging", module);

    module->setProperty("INFO", Logging::INFO);
    module->setProperty("DEBUG", Logging::DEBUG);
    module->setProperty("WARNING", Logging::WARNING);
    module->setProperty("ERROR", Logging::ERROR);
    module->setProperty("OFF", Logging::OFF);

    module->setProperty("log", engine->createFunction((NativeFunction)__script__log, 2));

    engine->getGlobal()->setProperty("assert", engine->createObject()); // Dummy placeholder,
    runFile(SCRIPT_DIR + "LoggingExtras.js", true);                     // replaced here.

    if (runTests)
    {
        Logging::log(Logging::DEBUG, "Logging module tests starting.\r\n");
        INDENT_LOG(Logging::DEBUG);

// TODO: Apply        assert(Logging::shouldShow(Logging::DEBUG)); // We must run tests at debug level or more!
        Logging::log(Logging::DEBUG, "You should now see a warning and an error\r\n");
        runScript("Logging.log(Logging.WARNING, 'A warning which you can ignore.');");
        runScript("Logging.log(Logging.ERROR, 'An error which you can ignore.');");
        assert(runScript("typeof Logging == 'object';"));
        assert(runScript("Logging.INFO == " + Utility::toString(Logging::INFO)));
        assert(runScript("Logging.DEBUG == " + Utility::toString(Logging::DEBUG)));
        assert(runScript("Logging.WARNING == " + Utility::toString(Logging::WARNING)));
        assert(runScript("Logging.ERROR == " + Utility::toString(Logging::ERROR)));
        assert(runScript("Logging.OFF == " + Utility::toString(Logging::OFF)));
        assert(runScript("typeof Logging.log === 'function';"));

        Logging::log(Logging::DEBUG, "You should now see an assertion failure\r\n");
        runScript("try { assert('0'); } catch (e) { };");
        Logging::log(Logging::DEBUG, "You should now NOT see an assertion failure\r\n");
        runScript("assert('1');");

        Logging::log(Logging::DEBUG, "Logging module tests complete.\r\n");
    }
}

/// Signals module
void ScriptEngineManager::setupSignals(bool runTests)
{
    runFile(
        SCRIPT_DIR + "Signals.js",
        true,
        "Object.addSignalMethods = addSignalMethods;"
    );

    if (runTests)
    {
        Logging::log(Logging::DEBUG, "Signals module tests starting.\r\n");
        INDENT_LOG(Logging::DEBUG);

        runFile(SCRIPT_DIR + "Signals__test.js", true);

        Logging::log(Logging::DEBUG, "Signals module tests complete.\r\n");
    }
}

/// Inheritance module
void ScriptEngineManager::setupInheritance(bool runTests)
{
    ScriptValuePtr module = engine->createObject();
    engine->getGlobal()->setProperty("Class", module);

    runFile(
        SCRIPT_DIR + "SimpleInheritance.js",
        true
//        "Object.addSignalMethods = addSignalMethods;"
    );

    if (runTests)
    {
        Logging::log(Logging::DEBUG, "Inheritance module tests starting.\r\n");
        INDENT_LOG(Logging::DEBUG);

        runFile(SCRIPT_DIR + "SimpleInheritance__test.js", true);

        Logging::log(Logging::DEBUG, "Inheritance module tests complete.\r\n");
    }
}

/// MochiKit module(s)
void ScriptEngineManager::setupMochiKit(bool runTests)
{
    ScriptValuePtr module = engine->createObject();
    engine->getGlobal()->setProperty("MochiKit", module);

    runFile(SCRIPT_DIR + "MochiKit.js", true);

//    runScript("jsonRegistry.register('undefiney', function(item) { return item === undefined; }, function(item) { return 'UNDEFINED'; });"); // FAILs

    if (runTests)
    {
        Logging::log(Logging::DEBUG, "MochiKit module tests starting.\r\n");
        INDENT_LOG(Logging::DEBUG);

        runFile(SCRIPT_DIR + "MochiKit__test.js", true);

        Logging::log(Logging::DEBUG, "MochiKit module tests complete.\r\n");
    }
}

void ScriptEngineManager::setupIntensityModule(std::string scriptBaseName, bool runTests)
{
    Logging::log(Logging::DEBUG, "%s scripting module setting up...\r\n", scriptBaseName.c_str());
    INDENT_LOG(Logging::DEBUG);

    // Allow stack traces to know the source file
    runScript("__MODULE_SOURCE = '" + scriptBaseName + ".js';");

    runFile(SCRIPT_DIR + scriptBaseName + ".js", true);

    if (runTests)
    {
        Logging::log(Logging::DEBUG, "%s module tests starting.\r\n", scriptBaseName.c_str());

        runFile(SCRIPT_DIR + scriptBaseName + "__test.js", true);

        Logging::log(Logging::DEBUG, "%s module tests complete.\r\n", scriptBaseName.c_str());
    }
}

void ScriptEngineManager::setupCAPIModule(bool runTests)
{
    ScriptValuePtr module = engine->createObject();
    engine->getGlobal()->setProperty("CAPI", module);

    #include "script_engine_embedding_impl.h"

    // Tweaking and additions
    runFile(SCRIPT_DIR + "intensity/CAPIExtras.js", true);
}

//! Set up all the modules etc. for the embedding
void ScriptEngineManager::setupEmbedding()
{
    Logging::log(Logging::DEBUG, "ScriptEngineManager::setupEmbedding.\r\n");
    INDENT_LOG(Logging::DEBUG);

    bool runTests = Utility::Config::getInt("Logging", "scripting_tests", 1);

    setupLogging(runTests);
    setupSignals(runTests);
    setupInheritance(runTests);
    setupMochiKit(runTests);

    runScript("window={};"); setupIntensityModule("jsUnit", false); // jsUnit

    // Our modules
    setupIntensityModule("intensity/Platform", runTests);
    #ifdef CLIENT
        runScript("Global.initAsClient();");
    #else // SERVER
        runScript("Global.initAsServer();");
    #endif

    // Platform stuff in Global (created in Platform)
    REFLECT_PYTHON( INTENSITY_VERSION_STRING );
    getGlobal()->getProperty("Global")->setProperty("version", boost::python::extract<std::string>(INTENSITY_VERSION_STRING));

    // Core tests
    if (runTests) {
        setupIntensityModule("intensity/__Testing", false);
    }

    setupCAPIModule(runTests);

    setupIntensityModule("intensity/Utilities", runTests);
    setupIntensityModule("intensity/Actions", runTests);
    setupIntensityModule("intensity/Variables", runTests);
    setupIntensityModule("intensity/LogicEntity", runTests);
    setupIntensityModule("intensity/MessageSystem", runTests);
    setupIntensityModule("intensity/LogicEntityClasses", runTests);
    setupIntensityModule("intensity/LogicEntityStore", runTests);
    setupIntensityModule("intensity/ModelAttachments", runTests);
    setupIntensityModule("intensity/Animatable", runTests);
    setupIntensityModule("intensity/Character", runTests);
    setupIntensityModule("intensity/StaticEntity", runTests);
    setupIntensityModule("intensity/Sound", runTests);
    setupIntensityModule("intensity/LogicEntityUtilities", runTests);
    setupIntensityModule("intensity/Application", runTests);
    setupIntensityModule("intensity/Editing", runTests);
    setupIntensityModule("intensity/Effects", runTests);
    setupIntensityModule("intensity/Projectiles", runTests);
    setupIntensityModule("intensity/Steering", runTests);

    Logging::log(Logging::DEBUG, "ScriptEngineManager::setupEmbedding complete.\r\n");
}


// Publics

void ScriptEngineManager::createEngine()
{
    Logging::log(Logging::DEBUG, "ScriptEngineManager::createEngine()\r\n");

    engineParameters.clear();

    // Engine-specific creation (might want to generalize this, but just 1 line)
    engine = new V8Engine(); // new TraceMonkeyEngine();

    Logging::log(Logging::DEBUG, "ScriptEngineManager::createEngine(): Init engine modules\r\n");

    // Generic initialization code that runs specific initialization in engine
    engine->init();

    Logging::log(Logging::DEBUG, "ScriptEngineManager::createEngine(): Setup embedding\r\n");

    setupEmbedding();
}

void ScriptEngineManager::destroyEngine()
{
    Logging::log(Logging::DEBUG, "ScriptEngineManager::destroyEngine()\r\n");

    if (engine != NULL) {
        engine->quit();
        delete engine;
        engine = NULL;
    }
}

bool ScriptEngineManager::hasEngine()
{
    return (engine != NULL);
}

ScriptEngine* ScriptEngineManager::getEngine()
{
    return engine;
}

ScriptValuePtr ScriptEngineManager::runScript(std::string script, std::string identifier)
{
    assert(engine);
    Logging::log(Logging::INFO, "Running script: %s\r\n", script.c_str());
    // TODO: Try to compile with and without processing, and warn about discrepancies
    REFLECT_PYTHON( process_script );
    script = boost::python::extract<std::string>(process_script(script));
    return engine->runScript(script, identifier);
}

bool ScriptEngineManager::runFile(std::string name, bool msg, std::string postfix)
{
    assert(engine);
    Logging::log(Logging::INFO, "Running script file: %s\r\n", name.c_str());
    char *buf = loadfile(name.c_str(), NULL);
    if(!buf) 
    {
        Logging::log(msg ? Logging::WARNING : Logging::INFO, "Could not read script file \"%s\"\r\n", name.c_str());
        return false;
    }
    runScript(buf + postfix, name); // Execute with filename as identifier
    delete[] buf;
    return true;
}

void ScriptEngineManager::runScriptNoReturn(std::string script, std::string identifier)
{
    assert(engine);
    Logging::log(Logging::INFO, "Running script: %s\r\n", script.c_str());
    runScript(script, identifier);
}

//! cubescript command to execute a ScriptEngine script
void run_script(char* script)
{
    if (ScriptEngineManager::hasEngine())
        ScriptEngineManager::runScriptNoReturn(script, "run_script");
    else
        Logging::log(Logging::WARNING, "Trying to run script '%s' without an engine\r\n", script);
}
COMMAND(run_script, "s");

std::string ScriptEngineManager::runScriptString(std::string script, std::string identifier)
{
    assert(engine);
    Logging::log(Logging::INFO, "Running script: %s\r\n", script.c_str());
    return runScript(script, identifier)->getString();
}

int ScriptEngineManager::runScriptInt(std::string script, std::string identifier)
{
    assert(engine);
    Logging::log(Logging::INFO, "Running script: %s\r\n", script.c_str());
    return runScript(script, identifier)->getInt();
}

std::string ScriptEngineManager::compileScript(std::string script)
{
    assert(engine);
    return engine->compileScript(script);
}

ScriptValuePtr ScriptEngineManager::createScriptValue(int value)
{
    assert(engine);
    return engine->createScriptValue(value);
}

ScriptValuePtr ScriptEngineManager::createScriptValue(double value)
{
    assert(engine);
    return engine->createScriptValue(value);
}

ScriptValuePtr ScriptEngineManager::createScriptValue(std::string value)
{
    assert(engine);
    return engine->createScriptValue(value);
}

ScriptValuePtr ScriptEngineManager::createScriptObject()
{
    assert(engine);
    return engine->createObject();
}

ScriptValuePtr ScriptEngineManager::getGlobal()
{
    assert(engine);
    return engine->getGlobal();
}

ScriptValuePtr ScriptEngineManager::getNull()
{
    assert(engine);
    return engine->getNull();
}

