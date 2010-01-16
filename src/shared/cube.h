#ifndef __CUBE_H__
#define __CUBE_H__

#ifdef __GNUC__
#define gamma __gamma
#endif

#include "python_wrap.h" // INTENSITY - must be first, as Python itself does some hacks

#include <math.h>

#ifdef __GNUC__
#undef gamma
#endif

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>
#include <limits.h>
#include <assert.h>
#ifdef __GNUC__
#include <new>
#else
#include <new.h>
#endif
#include <time.h>

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include "windows.h"
#endif

#ifndef STANDALONE
#include <SDL.h>
#include <SDL_image.h>

#define GL_GLEXT_LEGACY
#define __glext_h__
#define NO_SDL_GLEXT
#include <SDL_opengl.h>
#undef __glext_h__
#include "GL/glext.h"
#endif

#include <enet/enet.h>

#ifdef WIN32
  #define _WINDOWS
  #ifndef __GNUC__
    #define ZLIB_DLL
    #include <eh.h>
    #include <dbghelp.h>
  #endif
#endif
#include <zlib.h>

#ifdef __sun__
#undef sun
#undef MAXNAMELEN
#endif

#include "tools.h"
#include "geom.h"
#include "ents.h"
#include "command.h"

#include "iengine.h"
#include "igame.h"

#endif

