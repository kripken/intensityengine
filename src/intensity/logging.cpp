
// Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
// This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

#include "cube.h"
#include "engine.h"

#include "utility.h"


namespace Logging
{


//! The current level of logging. Only log messages with equal or higher severity will be shown
Level currLevel;

int currIndent = 0;

#define NUM_LEVELS 5

std::string    levelNames[NUM_LEVELS] = { "INFO",        "DEBUG",        "WARNING",        "ERROR",        "OFF"        };
Level levelEnums[NUM_LEVELS] = { INFO, DEBUG, WARNING, ERROR, OFF };

Level levelNameToEnum(std::string levelName)
{
    for (int i = 0; i < NUM_LEVELS; i++)
        if (levelNames[i] == levelName)
            return levelEnums[i];

    log(ERROR, "No such log level: %s\r\n", levelName.c_str());
    return levelEnums[0];
}


// Main

void setCurrLevel(Level level)
{
    assert(level >= 0 && level < NUM_LEVELS);

    currLevel = level;
    printf("<<< Setting logging level to %s >>>\r\n", levelNames[currLevel].c_str());
}

bool shouldShow(Level level)
{
    return level >= currLevel;
}

void init()
{
    std::string levelName = Utility::Config::getString("Logging", "level", levelNames[WARNING]);
    currLevel = levelNameToEnum(levelName);

    assert(currLevel >= 0 && currLevel < NUM_LEVELS);

    printf("<<< Setting logging level to %s >>>\r\n", levelNames[currLevel].c_str());
}

void log(Level level, const char *fmt, ...)
{
    assert(level >= 0 && level < NUM_LEVELS);

    if (shouldShow(level))
    {
        std::string levelString = "--UNKNOWN--";

        switch (level)
        {
            case INFO   : levelString = "INFO"   ; break;
            case DEBUG  : levelString = "DEBUG"  ; break;
            case WARNING: levelString = "WARNING"; break;
            case ERROR  : levelString = "ERROR"  ; break;
            case OFF    : levelString = "OFF"    ; break;
        }

        // Indent to the appropriate amount
        for (int i = 0; i < currIndent; i++)
            printf("   ");

        if (strlen(fmt) <= MAXSTRLEN)
        {
            defvformatstring(sf, fmt, fmt);

            // On the client, show errors in conoutf
            #ifdef CLIENT
            if (level == ERROR)
            {
                std::string total = "[[" + levelString + "]] - ";
                total += sf; 
                conoutf(CON_ERROR, total.c_str());
            }
            else
            #endif
                printf("[[%s]] - %s", levelString.c_str(), sf);
        } else
            printf("((%s)) - %s", levelString.c_str(), fmt);
    }

    fflush(stdout); // Ensure it all gets out
}

void log_noformat(int level, std::string text)
{
    assert(level >= 0 && level < NUM_LEVELS);

    for (unsigned int i = 0; i < text.size(); i++)
    {
        if (text[i] == '%')
            text[i] = '-'; // Prevent things that look like format markers
    }

    text = text + "\r\n";

    log(levelEnums[level], text.c_str());
}


Indent::Indent(Level level)
{
    if (shouldShow(level))
    {
        currIndent++;
        done = true;
    } else
        done = false;
}

Indent::~Indent()
{
    if (done)
        currIndent--;
}

} // namespace Logging


// For python
bool logging__should_show(int level)
{
    assert(level >= 0 && level < NUM_LEVELS);

    return Logging::shouldShow(Logging::levelEnums[level]);
}

