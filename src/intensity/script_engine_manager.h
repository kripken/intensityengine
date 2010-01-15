
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

class ScriptEngine;
//class ScriptValue;

class ScriptEngineManager
{
    static ScriptEngine* engine;

    static void setupLogging(bool runTests = false);
    static void setupSignals(bool runTests = false);
    static void setupInheritance(bool runTests = false);
    static void setupMochiKit(bool runTests = false);
    static void setupIntensityModule(std::string scriptBaseName, bool runTests = false);
    static void setupEmbedding();
    static void setupCAPIModule(bool runTests);

public:
    static std::map<std::string, std::string> engineParameters; //!< Internal parameters, not modifiable by scripts

    static void createEngine();
    static void destroyEngine();
    static bool hasEngine();
    static ScriptEngine* getEngine();

    static ScriptValuePtr runScript(std::string script, std::string identifier="");

    //! Runs a script from a file. Returns true if successful, false if file does not exist
    //! (follows sauer convention for execute in that sense).
    //! (the postfix is useful to make sure that the script results
    //! persist, by assigning to a module, etc.).
    static bool runFile(std::string name, bool msg=true, std::string postfix="");

    //! Convenience functions for Python access
    static void runScriptNoReturn(std::string script, std::string identifier="");
    static std::string runScriptString(std::string script, std::string identifier="");
    static int runScriptInt(std::string script, std::string identifier="");

    //! Compiles a script. Useful for testing syntax. Returns compilation errors, or ""
    static std::string compileScript(std::string script);

    static ScriptValuePtr createScriptValue(int value);
    static ScriptValuePtr createScriptValue(double value);
    static ScriptValuePtr createScriptValue(std::string value);
    static ScriptValuePtr createScriptObject();

    static ScriptValuePtr getGlobal();
    static ScriptValuePtr getNull();
};

