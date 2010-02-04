
/*
 *=============================================================================
 * Copyright (C) 2010 Alon Zakai ('Kripken') kripkensteiner@gmail.com
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


#include "SDL.h"

#include "intensity_plugin.h"
#include "intensity_plugin_listener.h"

#include "base/file_util.h"
#include "base/message_loop.h"
#include "base/time.h"


using namespace boost;

#define TO_STRING(type, post)                  \
std::string _toString##post(type val)          \
{                                        \
    std::stringstream ss;                \
    std::string ret;                     \
    ss << val;                           \
    return ss.str();                     \
}

TO_STRING(int,)
TO_STRING(long,)
TO_STRING(double,_)

std::string _toString(double val)
{
    std::string ret = _toString_(val);
    size_t i = ret.find(".");
    if (i < 0) return ret;
    return ret.substr(0, i+4);
}

SDLKey DOMSymToSDL(int key)
{
    if (key >= 65 && key <= 65+25)
        return (SDLKey)(key + 32);
    if (key >= 112 && key <= 112+11)
        return (SDLKey)(SDLK_F1 + key - 112);

    #define TRANS(k, v) case k: return v; break;
    switch(key)
    {
        TRANS(37, SDLK_LEFT)
        TRANS(38, SDLK_UP)
        TRANS(39, SDLK_RIGHT)
        TRANS(40, SDLK_DOWN)
        TRANS(191, SDLK_SLASH)
        TRANS(16, SDLK_RSHIFT)
        TRANS(17, SDLK_RCTRL)
        TRANS(18, SDLK_RALT)
        TRANS(46, SDLK_DELETE)
    }
    return (SDLKey)key;
}

ServerChannel *channel;

bool IntensityPluginObject::setWindow(NPWindow *window)
{
    printf("setWindow\r\n");

    if (!initialized)
        setupComm();

    printf("update window\r\n");

    window_ = window;

    printf("SetWindow: %d, %d\r\n", window->width, window->height);
    std::string message = "sw|" + _toString((int)window->width) + "|" + _toString((int)window->height);
    channel->write(message);

    if (!initialized)
    {
        initialize(window);
        initialized = true;
    }

    return true;
}

void IntensityPluginObject::initialize(NPWindow *window)
{
    printf("initialize\r\n");

    // SDL_WINDOWID hack
   	char* buffer = new char[1000];
    sprintf(buffer, "SDL_WINDOWID=%lu",(unsigned long)(window->window));
    putenv(buffer);
    printf("env: %s\r\n", buffer);

    // Start engine in other process
    printf("Launching process in %s\r\n", INTENSITY_INSTALL_ROOT);
    file_util::SetCurrentDirectory(FilePath(INTENSITY_INSTALL_ROOT));
    std::vector<std::string> argv;
    #ifdef LINUX
        argv.push_back("./intensity_client.sh");
    #else
        argv.push_back("intensity_client.bat");
    #endif
    argv.push_back("-P"); // Tell child process it should talk to us
    base::LaunchApp(CommandLine(argv), false, false, &processHandle);

    delete[] buffer;
}

void IntensityPluginObject::setupComm()
{
    printf("setupComm\r\n");
    channel = new ServerChannel();
}

void IntensityPluginObject::onMouseMove(double x, double y)
{
    double now = base::Time::Now().ToDoubleT();
    if (now - lastMouseMove < 0.005) return; // Max 200fps of mouse movements
    lastMouseMove = now;
    std::string message = "mm|" + _toString(x/window_->width) + "|" + _toString(y/window_->height);
    channel->write(message);
}

void IntensityPluginObject::onMouseButton(int button, bool down)
{
    std::string message = "mb|" + _toString(button) + "|" + _toString(down);
    channel->write(message);
}

void IntensityPluginObject::onKeyboard(int key, int unicode, bool down, bool isRepeat)
{
    key = DOMSymToSDL(key);
//printf("key: %d,%d\r\n", key, unicode);
    std::string message = "kb|" + _toString(key) + "|" + _toString(unicode) + "|" + _toString(down) + "|" + _toString(isRepeat);
    channel->write(message);
}

