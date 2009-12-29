
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


#include <string>
#include <map>

#include "cube.h"

#include "utility.h"

using namespace boost;


//==================================
// Utilities. Many of these will
// simply use boost/python for now,
// but we can replace them with
// suitable portable specific
// things later
//==================================


std::string Utility::SHA256(std::string text)
{
    EXEC_PYTHON("import hashlib");

    REFLECT_PYTHON_ALTNAME(hashlib.sha256, hashlib_sha256);

    return python::extract<std::string>( hashlib_sha256(text).attr("hexdigest")() );
}

//==============================
// String utils
//==============================

std::string Utility::toString(std::string val)
{
    return val;
}

#define TO_STRING(type)                  \
std::string Utility::toString(type val)  \
{                                        \
    std::stringstream ss;                \
    std::string ret;                     \
    ss << val;                           \
    return ss.str();                     \
}

TO_STRING(int)
TO_STRING(long)
TO_STRING(double)

bool Utility::validateAlphaNumeric(std::string input, std::string alsoAllow)
{
    EXEC_PYTHON("import re");

    REFLECT_PYTHON_ALTNAME( re.escape, re_escape);

    python::object original = python::object(input);
    for (unsigned int i = 0; i < alsoAllow.size(); i++)
    {
        original = original.attr("replace")(alsoAllow[i], ""); // Ignore the alsoAllow chars
    }

    python::object test = re_escape(original);

    bool ret = (test == original); // After escaping all non-alphanumeric, must be no change

    if (!ret)
        Logging::log(Logging::WARNING, "Validation of %s failed (using alphanumeric + %s)\r\n", input.c_str(), alsoAllow.c_str());

    return ret;
}

bool Utility::validateNotContaining(std::string input, std::string disallow)
{
    python::object original = python::object(input);
    int index = python::extract<int>(original.attr("find")(disallow));
    return index == -1; // -1 means it wasn't found, which is ok
}

bool Utility::validateRelativePath(std::string input)
{
    REFLECT_PYTHON( validate_relative_path );
    return python::extract<bool>(validate_relative_path(input));
}

std::string Utility::readFile(std::string name)
{
    REFLECT_PYTHON( open );
    try
    {
        python::object data = open(name, "r").attr("read")();
        return python::extract<std::string>(data);
    }
    catch(boost::python::error_already_set const &)
    {
        printf("Error in Python execution of readFile\r\n");
        PyErr_Print();
        assert(0 && "Halting on Python error");
    }

}


//==============================
// Config file utilities
//==============================

// Caches for speed; no need to call Python every time
typedef std::map<std::string, std::string> ConfigCacheString;
typedef std::map<std::string, int> ConfigCacheInt;
typedef std::map<std::string, float> ConfigCacheFloat;

ConfigCacheString configCacheString;
ConfigCacheInt configCacheInt;
ConfigCacheFloat configCacheFloat;

inline std::string getCacheKey(std::string section, std::string option)
    { return section + "|" + option; };

#define GET_CONFIG(type, Name, python_type)                                           \
type pythonGet##Name(std::string section, std::string option, type defaultVal)        \
{                                                                                     \
    Logging::log(Logging::DEBUG, "Config cache fail, going to Python: %s/%s\r\n",     \
                 section.c_str(), option.c_str());                                    \
    REFLECT_PYTHON(get_config);                                                       \
    REFLECT_PYTHON_ALTNAME(python_type, ptype);                                       \
    return python::extract<type>( ptype( get_config(section, option, defaultVal) ) ); \
} \
\
type Utility::Config::get##Name(std::string section, std::string option, type defaultVal)\
{ \
    std::string cacheKey = getCacheKey(section, option);\
    ConfigCache##Name::iterator iter = configCache##Name.find(cacheKey);\
    if (iter != configCache##Name.end())\
        return iter->second;\
    else {\
        type value = pythonGet##Name(section, option, defaultVal); \
        configCache##Name[cacheKey] = value; \
        return value; \
    } \
}

GET_CONFIG(std::string, String, str)
GET_CONFIG(int,         Int,    int)
GET_CONFIG(float,       Float,  float)

#define SET_CONFIG(type, Name)                                                  \
void Utility::Config::set##Name(std::string section, std::string option, type value) \
{                                                                               \
    REFLECT_PYTHON(set_config);                                                 \
    set_config(section, option, value);                                         \
    std::string cacheKey = getCacheKey(section, option); \
    configCache##Name[cacheKey] = value; \
}

SET_CONFIG(std::string, String)
SET_CONFIG(int,         Int)
SET_CONFIG(float,       Float)


// Cubescript accessibility

void get_config(char *section, char *option)
{
    result(Utility::Config::getString(section, option, "?").c_str());
}

COMMAND(get_config, "ss");


//==============================
// System Info
//==============================

extern int clockrealbase;

int Utility::SystemInfo::currTime()
{
#ifdef SERVER
    return enet_time_get();
#else // CLIENT
    return SDL_GetTicks() - clockrealbase;
#endif
// This old method only changes during calls to updateworld etc.!
//    extern int lastmillis;
//    return lastmillis; // We wrap around the sauer clock
}

