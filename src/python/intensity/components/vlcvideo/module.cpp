
// Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
// This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

// Based off of http://wiki.videolan.org/LibVLC_SampleCode_SDL (WTFPL licensed, Sam Hocevar)

#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <stdlib.h>

#include <boost/python.hpp>

#include <vlc/vlc.h>

#define VIDEOWIDTH 512
#define VIDEOHEIGHT 384

struct ctx_t
{
    char *pixels;
    bool fresh;
    ctx_t() : fresh(false) { };
};

ctx_t *ctx = NULL;

static void vlc_catch (libvlc_exception_t *ex)
{
    if(libvlc_exception_raised(ex))
    {
        fprintf(stderr, "VLC exception: %s\n", libvlc_exception_get_message(ex));
        exit(1);
    }

    libvlc_exception_clear(ex);
}

#ifdef VLC09X
static void * lock(struct ctx_t *ctx)
{
    return ctx->pixels;
}
#else
static void lock(struct ctx_t *ctx, void **pp_ret)
{
    *pp_ret = ctx->pixels;
}
#endif

static void unlock(struct ctx_t *ctx)
{
    // Mark the video data as being refreshed, so Python will read it
    ctx->fresh = true;
}

libvlc_instance_t *libvlc;
libvlc_media_player_t *mp;
libvlc_media_t *m;

void module_init()
{
    ctx = new ctx_t;
    ctx->pixels = new char[VIDEOWIDTH*VIDEOHEIGHT*40];

    char clock[64], cunlock[64], cdata[64];
    char width[32], height[32], pitch[32];
    libvlc_exception_t ex;

    char const *vlc_argv[] =
    {
        "-q",
        "-vvvvv",
//        "--plugin-path", VLC_TREE "/modules",
        "--ignore-config", /* Don't use VLC's config files */
//        "--novideo", or audio
        "--vout", "vmem",
        "--vmem-width", width,
        "--vmem-height", height,
        "--vmem-pitch", pitch,
        "--vmem-chroma", "RV24", // 24
        "--vmem-lock", clock,
        "--vmem-unlock", cunlock,
        "--vmem-data", cdata,
    };
    int vlc_argc = sizeof(vlc_argv) / sizeof(*vlc_argv);

    /*
     *  Initialise libVLC
     */
    sprintf(clock, "%lld", (long long int)(intptr_t)lock);
    sprintf(cunlock, "%lld", (long long int)(intptr_t)unlock);
    sprintf(cdata, "%lld", (long long int)(intptr_t)ctx);
    sprintf(width, "%i", VIDEOWIDTH);
    sprintf(height, "%i", VIDEOHEIGHT);
    sprintf(pitch, "%i", VIDEOWIDTH * 3);

    libvlc_exception_init(&ex);
    libvlc = libvlc_new(vlc_argc, vlc_argv, &ex);
    vlc_catch(&ex);

    m = libvlc_media_new(libvlc, "/media/_media_vault/Shows/The.Colbert.Report.2009.11.30.Cevin.Soling.PDTV.XviD-FQM.[VTV].avi", &ex);
//    m = libvlc_media_new(libvlc, "/home/alon/Downloads/02 - Slippin Away - U2.mp3", &ex);
    vlc_catch(&ex);

    mp = libvlc_media_player_new_from_media(m, &ex);
    vlc_catch(&ex);

    libvlc_media_player_play(mp, &ex);
    vlc_catch(&ex);

}

void module_quit()
{
    /*
     * Stop stream and clean up libVLC
     */
//printf("INITh\r\n");
    libvlc_exception_t ex;

    libvlc_media_player_stop(mp, &ex);
    vlc_catch(&ex);
//printf("INITAA\r\n");

    libvlc_media_release(m);
    vlc_catch(&ex);
//printf("INITBB\r\n");

    libvlc_media_player_release(mp);
    vlc_catch(&ex);
//printf("INITCC\r\n");
    libvlc_release(libvlc);
    vlc_catch(&ex);
//printf("INITk\r\n");

    // Clean up pixel storage
    delete ctx->pixels;
    delete ctx;
}


long long int get_pixels()
{
    if (ctx->fresh)
    {
        ctx->fresh = false;

        for (int x = 0; x < VIDEOWIDTH; x++)
            for (int y = 0; y < VIDEOHEIGHT; y++)
            {
                // Fix RGB vs BGR
                char r = ctx->pixels[(x+y*VIDEOWIDTH)*3];
                char b = ctx->pixels[(x+y*VIDEOWIDTH)*3 + 2];
                ctx->pixels[(x+y*VIDEOWIDTH)*3] = b;
                ctx->pixels[(x+y*VIDEOWIDTH)*3 + 2] = r;
            }

        return (long long int)(intptr_t)(ctx->pixels);
    }
    else
        return 0;
}

/* // For testing
int main()
{
    module_init();
printf("ok a\r\n");
    module_quit();
    return 1;
}
*/


#ifdef WIN32
BOOST_PYTHON_MODULE(module)
#else // LINUX (,OSX?)
BOOST_PYTHON_MODULE(libmodule)
#endif
{
    using namespace boost::python;

    def("init", module_init);
    def("quit", module_quit);

    def("get_pixels", get_pixels);
}

