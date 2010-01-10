
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


#include <queue>

#ifdef USE_JPEG2000
#include "openjpeg.h"
#endif

#include "utility.h"


// 'Background' loading system for texture slots

static std::set<int> requested_slots;

Slot &lookuptexture(int slot, bool load)
{
    Slot &s = slots.inrange(slot) ? slots[slot] : (slots.empty() ? dummyslot : slots[0]);
    if (load && !s.loaded)
    {
        if (slots.inrange(slot))
        {
            if (requested_slots.count(slot) == 0)
            {
                requested_slots.insert(slot);
                loopv(s.sts) s.sts[i].t = notexture; // Until we load them, do not crash in rendering code
            }
        } else
            loadslot(s, false);
    }
    return s;
}

namespace IntensityTexture
{

void resetBackgroundLoading()
{
    requested_slots.clear();
}

void doBackgroundLoading(bool all)
{
    while (requested_slots.size() > 0)
    {
        int slot = *(requested_slots.begin());
        requested_slots.erase(slot);

        assert(slots.inrange(slot));
        Slot &s = slots[slot];
        loadslot(s, false); // for materials, would be true

        if (!all) break;
    }
}


#ifdef USE_JPEG2000
//#define JP2_ALLOW_HIGH_PRECISION

void info_callback(const char* msg, void*)
{
    Logging::log(Logging::DEBUG, "OpenJPEG: %s", msg);
}

void warning_callback(const char* msg, void*)
{
    Logging::log(Logging::WARNING, "OpenJPEG: %s", msg);
}

void error_callback(const char* msg, void*)
{
    Logging::log(Logging::ERROR, "OpenJPEG: %s", msg);
}

void genopenjpeg(char *infile, char *outfile)
{
    //=========================
    //= Load data
    //=========================

    stream *file = openfile(infile, "rb");
    assert(file);
    long size = file->size();
    uchar *data = new uchar[size];
    assert(data);
    int read = file->read(data, size);
    assert(read == size);

    Logging::log(Logging::DEBUG, "Converting jpeg2000 '%s', size: %ld\r\n", infile, size);

    //=========================
    //= Uncompress JPEG2000
    //=========================

    opj_dparameters_t parameters;
    opj_set_default_decoder_parameters(&parameters);

    opj_dinfo_t* dinfo = opj_create_decompress(CODEC_JP2);
    assert(dinfo);
    opj_setup_decoder(dinfo, &parameters);

    opj_event_mgr_t events;
    memset(&events, 0, sizeof(opj_event_mgr_t));
    events.info_handler = info_callback;
    events.warning_handler = warning_callback;
    events.error_handler = error_callback;
    opj_set_event_mgr((opj_common_ptr)dinfo, &events, stderr);            

    opj_cio_t *cio = opj_cio_open((opj_common_ptr)dinfo, data, size);
    assert(cio);

    opj_image_t *image = opj_decode(dinfo, cio);
    assert(image);

    opj_cio_close(cio);

    if(dinfo)
        opj_destroy_decompress(dinfo);

    assert(image->x0 == 0);
    assert(image->y0 == 0);

    int components = image->numcomps;
    assert(components);
    int width = image->x1;
    int height = image->y1;
    ImageData sauerimage(width, height, components);
    uchar *rawData = sauerimage.data;

    Logging::log(Logging::DEBUG, " components: %d  w: %d  h: %d\r\n", components, width, height);

    for (int i = 0; i < components; i++)
    {
        opj_image_comp_t& component = image->comps[i];
        int *componentData = component.data;
        assert(componentData);

        assert(component.w == width);
        assert(component.factor == 0);
        #ifndef JP2_ALLOW_HIGH_PRECISION
            assert(component.prec == 8);
        #endif

        Logging::log(Logging::DEBUG, " component %d:  prec: %d  bpp: %d  sgned: %d  resno_d: %d\r\n", i, component.prec, component.bpp, component.sgnd, component.resno_decoded);

        int offset = i;
        for (int y = (height-1); y >= 0; y--)
        {
            int yWidth = y*width;
            for (int x = 0; x < width; x++)
            {
                int value = componentData[yWidth + x];
                #ifdef JP2_ALLOW_HIGH_PRECISION
                    if (component.prec > 8)
                        value >>= (component.prec - 8); // Reduce precision into 8 bits
                #endif
                rawData[offset] = value;
                offset += components;
            }
        }
    }

    opj_image_destroy(image);

    //=========================
    //= Save as PNG
    //=========================

    if(!outfile[0])
    {
        static string buf;
        copystring(buf, infile);
        int len = strlen(buf);
        if(len > 4 && buf[len-4]=='.') memcpy(&buf[len-4], ".png", 4);
        else concatstring(buf, ".png");
        outfile = buf;
    }

    Logging::log(Logging::DEBUG, "Saving PNG data to: %s\r\n", outfile);

    saveimage(outfile, IMG_PNG, sauerimage, true);

    delete[] data;
}
COMMAND(genopenjpeg, "ss");
#endif

// Publics

#define FIX_PATH(s) \
    s = "packages/" + s; \
    static string __##s; \
    copystring(__##s, s.c_str()); \
    s = path(__##s); \
    if (!Utility::validateRelativePath(s)) { printf("Relative path not validated: %s\r\n", s.c_str()); assert(0); }; \
    std::string full_##s = findfile(s.c_str(), "wb");

#ifdef USE_JPEG2000
void convertJP2toPNG(std::string source, std::string dest)
{
    FIX_PATH(source);
    FIX_PATH(dest);

    REFLECT_PYTHON( check_newer_than );
    if (boost::python::extract<bool>(check_newer_than(full_dest, full_source)))
        return;

    Logging::log(Logging::DEBUG, "convertJP2toPNG: %s ==> %s\r\n", source.c_str(), dest.c_str());

    renderprogress(0, ("decompressing image: " + source).c_str());

    genopenjpeg((char*)source.c_str(), (char*)dest.c_str());
}
#endif

void convertPNGtoDDS(std::string source, std::string dest)
{
    Logging::log(Logging::WARNING, "Creating DDS files should not be done in this way!\r\n");

    FIX_PATH(source);
    FIX_PATH(dest);

    REFLECT_PYTHON( check_newer_than );
    if (boost::python::extract<bool>(check_newer_than(full_dest, full_source)))
        return;

    Logging::log(Logging::DEBUG, "convertPNGtoDDS: %s ==> %s\r\n", source.c_str(), dest.c_str());

    renderprogress(0, ("preparing dds image: " + source).c_str());

    if (hasTC) 
        gendds((char*)source.c_str(), (char*)dest.c_str());
}

void combineImages(std::string primary, std::string secondary, std::string dest)
{
    FIX_PATH(primary);
    FIX_PATH(secondary);
    FIX_PATH(dest);

    REFLECT_PYTHON( check_newer_than );
    if (boost::python::extract<bool>(check_newer_than(full_dest, full_primary, full_secondary)))
        return;

    Logging::log(Logging::DEBUG, "combineImages: %s + %s ==> %s\r\n", primary.c_str(), secondary.c_str(), dest.c_str());

    renderprogress(0, ("combining image: " + full_dest).c_str());

    ImageData rgb, a;
    if(!loadimage(primary.c_str(), rgb)) return;
    assert(rgb.bpp == 3);
    if(!loadimage(secondary.c_str(), a)) return;
    assert(a.bpp == 1 || a.bpp == 3);

    if (a.w != rgb.w || a.h != rgb.h) scaleimage(a, rgb.w, rgb.h);

    mergespec(rgb, a);

    saveimage(dest.c_str(), guessimageformat(dest.c_str(), IMG_PNG), rgb);
}

void uploadTextureData(std::string name, int x, int y, int w, int h, void *pixels)
{
    Texture *t = textures.access(path(name.c_str(), true));
    if (!t)
    {
        Logging::log(Logging::WARNING, "uploadTextureData: %s is missing\r\n", name.c_str());
        return;
    }

    GLenum format = texformat(t->bpp);
    setuptexparameters(t->id, pixels, t->clamp, 1, format, GL_TEXTURE_2D);
/*
    glBindTexture(GL_TEXTURE_2D, t->id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); //GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); //GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
*/

    glTexImage2D(GL_TEXTURE_2D, 0, format, t->w, t->h, 0, format, GL_UNSIGNED_BYTE, NULL);
    glTexSubImage2D (GL_TEXTURE_2D, 0, x, y, w, h, format, GL_UNSIGNED_BYTE, pixels);

/* FAIL:
    uchar *buf = new uchar[w*h*t->bpp];
    int currx = x, curry = y;
    int currw = w, currh = h;
    for (int level = 0; ; level++)
    {
        glTexImage2D(GL_TEXTURE_2D, level, format, t->w, t->h, 0, format, GL_UNSIGNED_BYTE, NULL);
        glTexSubImage2D (GL_TEXTURE_2D, level, x, y, currw, currh, format, GL_UNSIGNED_BYTE, level == 0 ? pixels : buf);

        currx /= 2; // using this causes hardware flicker
        curry /= 2;
        currw /= 2;
        currh /= 2;
        if((hasGM && hwmipmap) || max(currw, currh) <= 1) break;
        scaletexture((uchar*)pixels, w, h, t->bpp, 0, buf, currw, currh);
    }
    delete[] buf;
*/
}

}

