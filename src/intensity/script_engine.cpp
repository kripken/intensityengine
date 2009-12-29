
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


// Script Value Args

ScriptValueArgs::ScriptValueArgs()
{
    engine = ScriptEngineManager::getEngine();
}

ScriptValueArgs& ScriptValueArgs::append(int value)
{
    args.push_back( ScriptEngineManager::createScriptValue(value) ); // TODO: use engine directly
    return *this;
}

ScriptValueArgs& ScriptValueArgs::append(bool value)
{
    args.push_back( ScriptEngineManager::createScriptValue(value) );
    return *this;
}

ScriptValueArgs& ScriptValueArgs::append(double value)
{
    args.push_back( ScriptEngineManager::createScriptValue(value) );
    return *this;
}

ScriptValueArgs& ScriptValueArgs::append(std::string value)
{
    args.push_back( ScriptEngineManager::createScriptValue(value) );
    return *this;
}

ScriptValueArgs& ScriptValueArgs::append(ScriptValuePtr value)
{
    args.push_back( value );
    return *this;
}


// Script Value

ScriptValue::ScriptValue(ScriptEngine* _engine) : engine(_engine)
{
    engine->registerScriptValue(this);
}

ScriptValue::~ScriptValue()
{
    Logging::log(Logging::INFO, "~ScriptValue: %d\r\n", isValid());

    if (isValid())
        invalidate(); // C++: we are a base class here, so a derived class func would not be called
                      // "Never Call Virtual Functions during Construction or Destruction"
}

void ScriptValue::invalidate()
{
    Logging::log(Logging::INFO, "ScriptValue::invalidate %d\r\n", isValid());

    assert(isValid());

    engine->unregisterScriptValue(this);
    engine = NULL;

    assert(!isValid());
}

bool ScriptValue::isValid()
{
    return (engine != NULL);
}


// Script Engine

ScriptEngine::~ScriptEngine()
{
    Logging::log(Logging::DEBUG, "~ScriptEngine (0)\r\n");

    ScriptValueStore temp(registeredScriptValues); // Iterate on temp, as inside the next loop we operate on registeredScriptValues
    for (ScriptValueStore::iterator iter = temp.begin(); iter != temp.end(); iter++)
    {
        (*iter)->invalidate();
    }

    assert(registeredScriptValues.size() == 0); // They were all removed

    Logging::log(Logging::DEBUG, "~ScriptEngine (1)\r\n");
}

void ScriptEngine::registerScriptValue(ScriptValue* value)
{
    assert(registeredScriptValues.find(value) == registeredScriptValues.end());
    registeredScriptValues.insert(value);
}

void ScriptEngine::unregisterScriptValue(ScriptValue* value)
{
    assert(registeredScriptValues.find(value) != registeredScriptValues.end());
    registeredScriptValues.erase(value);
}

