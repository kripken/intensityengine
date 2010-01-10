// texture.cpp: texture slot management

#include "engine.h"

#define FUNCNAME(name) name##1
#define DEFPIXEL uint OP(r, 0);
#define PIXELOP OP(r, 0);
#define BPP 1
#include "scale.h"

#define FUNCNAME(name) name##2
#define DEFPIXEL uint OP(r, 0), OP(g, 1);
#define PIXELOP OP(r, 0); OP(g, 1);
#define BPP 2
#include "scale.h"

#define FUNCNAME(name) name##3
#define DEFPIXEL uint OP(r, 0), OP(g, 1), OP(b, 2);
#define PIXELOP OP(r, 0); OP(g, 1); OP(b, 2);
#define BPP 3
#include "scale.h"

#define FUNCNAME(name) name##4
#define DEFPIXEL uint OP(r, 0), OP(g, 1), OP(b, 2), OP(a, 3);
#define PIXELOP OP(r, 0); OP(g, 1); OP(b, 2); OP(a, 3);
#define BPP 4
#include "scale.h"

static void scaletexture(uchar *src, uint sw, uint sh, uint bpp, uint pitch, uchar *dst, uint dw, uint dh)
{
    if(sw == dw*2 && sh == dh*2)
    {
        switch(bpp)
        {
            case 1: return halvetexture1(src, sw, sh, pitch, dst);
            case 2: return halvetexture2(src, sw, sh, pitch, dst);
            case 3: return halvetexture3(src, sw, sh, pitch, dst);
            case 4: return halvetexture4(src, sw, sh, pitch, dst);
        }
    }
    else if(sw < dw || sh < dh || sw&(sw-1) || sh&(sh-1))
    {
        switch(bpp)
        {
            case 1: return scaletexture1(src, sw, sh, pitch, dst, dw, dh);
            case 2: return scaletexture2(src, sw, sh, pitch, dst, dw, dh);
            case 3: return scaletexture3(src, sw, sh, pitch, dst, dw, dh);
            case 4: return scaletexture4(src, sw, sh, pitch, dst, dw, dh);
        }
    }
    else
    {
        switch(bpp)
        {
            case 1: return shifttexture1(src, sw, sh, pitch, dst, dw, dh);
            case 2: return shifttexture2(src, sw, sh, pitch, dst, dw, dh);
            case 3: return shifttexture3(src, sw, sh, pitch, dst, dw, dh);
            case 4: return shifttexture4(src, sw, sh, pitch, dst, dw, dh);
        }
    }
}

static inline void reorienttexture(uchar *src, int sw, int sh, int bpp, int stride, uchar *dst, bool flipx, bool flipy, bool swapxy, bool normals = false)
{
    int stridex = bpp, stridey = bpp;
    if(swapxy) stridex *= sh; else stridey *= sw;
    if(flipx) { dst += (sw-1)*stridex; stridex = -stridex; }
    if(flipy) { dst += (sh-1)*stridey; stridey = -stridey; }
    uchar *srcrow = src;
    loopi(sh)
    {
        for(uchar *curdst = dst, *src = srcrow, *end = &srcrow[sw*bpp]; src < end;)
        {
            loopk(bpp) curdst[k] = *src++;
            if(normals)
            {
                if(flipx) curdst[0] = 255-curdst[0];
                if(flipy) curdst[1] = 255-curdst[1];
                if(swapxy) swap(curdst[0], curdst[1]);
            }
            curdst += stridex;
        }
        srcrow += stride;
        dst += stridey;
    }
}

#define writetex(t, body) \
    { \
        uchar *dstrow = t.data; \
        loop(y, t.h) \
        { \
            for(uchar *dst = dstrow, *end = &dstrow[t.w*t.bpp]; dst < end; dst += t.bpp) \
            { \
                body; \
            } \
            dstrow += t.pitch; \
        } \
    }

#define readwritetex(t, s, body) \
    { \
        uchar *dstrow = t.data, *srcrow = s.data; \
        loop(y, t.h) \
        { \
            for(uchar *dst = dstrow, *src = srcrow, *end = &srcrow[s.w*s.bpp]; src < end; dst += t.bpp, src += s.bpp) \
            { \
                body; \
            } \
            dstrow += t.pitch; \
            srcrow += s.pitch; \
        } \
    }

#define read2writetex(t, s1, src1, s2, src2, body) \
    { \
        uchar *dstrow = t.data, *src1row = s1.data, *src2row = s2.data; \
        loop(y, t.h) \
        { \
            for(uchar *dst = dstrow, *end = &dstrow[t.w*t.bpp], *src1 = src1row, *src2 = src2row; dst < end; dst += t.bpp, src1 += s1.bpp, src2 += s2.bpp) \
            { \
                body; \
            } \
            dstrow += t.pitch; \
            src1row += s1.pitch; \
            src2row += s2.pitch; \
        } \
    }

void texreorient(ImageData &s, bool flipx, bool flipy, bool swapxy, int type = TEX_DIFFUSE)
{
    ImageData d(swapxy ? s.h : s.w, swapxy ? s.w : s.h, s.bpp);
    reorienttexture(s.data, s.w, s.h, s.bpp, s.pitch, d.data, flipx, flipy, swapxy, type==TEX_NORMAL);
    s.replace(d);
}

void texrotate(ImageData &s, int numrots, int type = TEX_DIFFUSE)
{
    // 1..3 rotate through 90..270 degrees, 4 flips X, 5 flips Y 
    if(numrots>=1 && numrots<=5) 
        texreorient(s, 
            numrots>=2 && numrots<=4, // flip X on 180/270 degrees
            numrots<=2 || numrots==5, // flip Y on 90/180 degrees
            (numrots&5)==1,           // swap X/Y on 90/270 degrees
            type);
}

void texoffset(ImageData &s, int xoffset, int yoffset)
{
    xoffset = max(xoffset, 0);
    xoffset %= s.w;
    yoffset = max(yoffset, 0);
    yoffset %= s.h;
    if(!xoffset && !yoffset) return;
    ImageData d(s.w, s.h, s.bpp);
    uchar *src = s.data;
    loop(y, s.h)
    {
        uchar *dst = (uchar *)d.data+((y+yoffset)%d.h)*d.pitch;
        memcpy(dst+xoffset*s.bpp, src, (s.w-xoffset)*s.bpp);
        memcpy(dst, src+(s.w-xoffset)*s.bpp, xoffset*s.bpp);
        src += s.pitch;
    }
    s.replace(d);
}

void texmad(ImageData &s, const vec &mul, const vec &add)
{
    int maxk = min(int(s.bpp), 3);
    writetex(s,
        loopk(maxk) dst[k] = uchar(clamp(dst[k]*mul[k] + 255*add[k], 0.0f, 255.0f));
    );
}

void texffmask(ImageData &s, int minval)
{
    if(renderpath!=R_FIXEDFUNCTION) return;
    if(nomasks || s.bpp<3) { s.cleanup(); return; }
    bool glow = false, envmap = true;
    writetex(s,
        if(dst[1]>minval) glow = true;
        if(dst[2]>minval) { glow = envmap = true; goto needmask; }
    );
    if(!glow && !envmap) { s.cleanup(); return; }
needmask:
    ImageData m(s.w, s.h, envmap ? 2 : 1);
    readwritetex(m, s,
        dst[0] = src[1];
        if(envmap) dst[1] = src[2];
    );
    s.replace(m);
}

void texdup(ImageData &s, int srcchan, int dstchan)
{
    if(srcchan==dstchan || max(srcchan, dstchan) >= s.bpp) return;
    writetex(s, dst[dstchan] = dst[srcchan]);
}

void texdecal(ImageData &s)
{
    if(renderpath!=R_FIXEDFUNCTION || hasTE) return;
    ImageData m(s.w, s.w, 2);
    readwritetex(m, s,
        dst[0] = src[0];
        dst[1] = 255 - src[0];
    );
    s.replace(m);
}

void texmix(ImageData &s, int c1, int c2, int c3, int c4)
{
    int numchans = c1 < 0 ? 0 : (c2 < 0 ? 1 : (c3 < 0 ? 2 : (c4 < 0 ? 3 : 4)));
    if(numchans <= 0) return;
    ImageData d(s.w, s.h, numchans);
    readwritetex(d, s,
        switch(numchans)
        {
            case 4: dst[3] = src[c4];
            case 3: dst[2] = src[c3];
            case 2: dst[1] = src[c2];
            case 1: dst[0] = src[c1];
        }
    );
    s.replace(d);
}

VAR(hwtexsize, 1, 0, 0);
VAR(hwcubetexsize, 1, 0, 0);
VAR(hwmaxaniso, 1, 0, 0);
VARFP(maxtexsize, 0, 0, 1<<12, initwarning("texture quality", INIT_LOAD));
VARFP(reducefilter, 0, 1, 1, initwarning("texture quality", INIT_LOAD));
VARFP(texreduce, 0, 0, 12, initwarning("texture quality", INIT_LOAD));
VARFP(texcompress, 0, 1<<10, 1<<12, initwarning("texture quality", INIT_LOAD));
VARFP(texcompressquality, -1, -1, 1, setuptexcompress());
VARFP(trilinear, 0, 1, 1, initwarning("texture filtering", INIT_LOAD));
VARFP(bilinear, 0, 1, 1, initwarning("texture filtering", INIT_LOAD));
VARFP(aniso, 0, 0, 16, initwarning("texture filtering", INIT_LOAD));

void setuptexcompress()
{
    if(!hasTC) return;

    GLenum hint = GL_DONT_CARE;
    switch(texcompressquality)
    {
        case 1: hint = GL_NICEST; break;
        case 0: hint = GL_FASTEST; break;
    }
    glHint(GL_TEXTURE_COMPRESSION_HINT_ARB, hint);
}

GLenum compressedformat(GLenum format, int w, int h, int force = 0)
{
    if(hasTC && texcompress && (force || max(w, h) >= texcompress)) switch(format)
    {
        case GL_RGB5:
        case GL_RGB8:
#ifdef __APPLE__
        case GL_RGB: return GL_COMPRESSED_RGB_ARB;
        case GL_RGBA: return GL_COMPRESSED_RGBA_ARB;
#else
        case GL_RGB: return GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
        case GL_RGBA: return GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
#endif
    }
    return format;
}

int formatsize(GLenum format)
{
    switch(format)
    {
        case GL_LUMINANCE:
        case GL_ALPHA: return 1;
        case GL_LUMINANCE_ALPHA: return 2;
        case GL_RGB: return 3;
        case GL_RGBA: return 4;
        default: return 4;
    }
}

VARFP(hwmipmap, 0, 0, 1, initwarning("texture filtering", INIT_LOAD));
VARFP(usenp2, 0, 0, 1, initwarning("texture quality", INIT_LOAD));

void resizetexture(int w, int h, bool mipmap, bool canreduce, GLenum target, int compress, int &tw, int &th)
{
    int hwlimit = target==GL_TEXTURE_CUBE_MAP_ARB ? hwcubetexsize : hwtexsize,
        sizelimit = mipmap && maxtexsize ? min(maxtexsize, hwlimit) : hwlimit;
    if(compress && !hasTC)
    {
        w = max(w/compress, 1);
        h = max(h/compress, 1);
    }
    if(canreduce && texreduce)
    {
        w = max(w>>texreduce, 1);
        h = max(h>>texreduce, 1);
    }
    w = min(w, sizelimit);
    h = min(h, sizelimit);
    if((!hasNP2 || !usenp2) && target!=GL_TEXTURE_RECTANGLE_ARB && (w&(w-1) || h&(h-1)))
    {
        tw = th = 1;
        while(tw < w) tw *= 2;
        while(th < h) th *= 2;
        if(w < tw - tw/4) tw /= 2;
        if(h < th - th/4) th /= 2;
    }
    else
    {
        tw = w;
        th = h;
    }
}

void uploadtexture(GLenum target, GLenum internal, int tw, int th, GLenum format, GLenum type, void *pixels, int pw, int ph, int pitch, bool mipmap)
{
    int bpp = formatsize(format), row = 0, rowalign = 0;
    if(!pitch) pitch = pw*bpp; 
    uchar *buf = NULL;
    if(pw!=tw || ph!=th) 
    {
        buf = new uchar[tw*th*bpp];
        scaletexture((uchar *)pixels, pw, ph, bpp, pitch, buf, tw, th);
    }
    else if(tw*bpp != pitch)
    {
        row = pitch/bpp;
        rowalign = texalign(pixels, pitch, 1);
        while(rowalign > 0 && ((row*bpp + rowalign - 1)/rowalign)*rowalign != pitch) rowalign >>= 1;
        if(!rowalign)
        {
            row = 0;
            buf = new uchar[tw*th*bpp];
            loopi(th) memcpy(&buf[i*tw*bpp], &((uchar *)pixels)[i*pitch], tw*bpp);
        }
    }
    for(int level = 0, align = 0;; level++)
    {
        uchar *src = buf ? buf : (uchar *)pixels;
        if(buf) pitch = tw*bpp;
        int srcalign = row > 0 ? rowalign : texalign(src, pitch, 1);
        if(align != srcalign) glPixelStorei(GL_UNPACK_ALIGNMENT, align = srcalign);
        if(row > 0) glPixelStorei(GL_UNPACK_ROW_LENGTH, row);
        extern int ati_teximage_bug;
        if(ati_teximage_bug && (internal==GL_RGB || internal==GL_RGB8) && mipmap && src && !level)
        {
            if(target==GL_TEXTURE_1D) 
            {
                glTexImage1D(target, level, internal, tw, 0, format, type, NULL);
                glTexSubImage1D(target, level, 0, tw, format, type, src);
            }
            else 
            {
                glTexImage2D(target, level, internal, tw, th, 0, format, type, NULL);
                glTexSubImage2D(target, level, 0, 0, tw, th, format, type, src);
            }
        }
        else if(target==GL_TEXTURE_1D) glTexImage1D(target, level, internal, tw, 0, format, type, src);
        else glTexImage2D(target, level, internal, tw, th, 0, format, type, src);
        if(row > 0) glPixelStorei(GL_UNPACK_ROW_LENGTH, row = 0);
        if(!mipmap || (hasGM && hwmipmap) || max(tw, th) <= 1) break;
        int srcw = tw, srch = th;
        if(tw > 1) tw /= 2;
        if(th > 1) th /= 2;
        if(!buf) buf = new uchar[tw*th*bpp];
        scaletexture(src, srcw, srch, bpp, pitch, buf, tw, th);
    }
    if(buf) delete[] buf;
}

void uploadcompressedtexture(GLenum target, GLenum format, int w, int h, uchar *data, int align, int blocksize, int levels, bool mipmap)
{
    int hwlimit = target==GL_TEXTURE_CUBE_MAP_ARB ? hwcubetexsize : hwtexsize,
        sizelimit = levels > 1 && maxtexsize ? min(maxtexsize, hwlimit) : hwlimit;
    int level = 0;
    loopi(levels)
    {
        int size = ((w + align-1)/align) * ((h + align-1)/align) * blocksize;
        if(w <= sizelimit && h <= sizelimit)
        {
            if(target==GL_TEXTURE_1D) glCompressedTexImage1D_(target, level, format, w, 0, size, data);
            else glCompressedTexImage2D_(target, level, format, w, h, 0, size, data);
            level++;
            if(!mipmap) break;
        }
        if(max(w, h) <= 1) break;
        if(w > 1) w /= 2;
        if(h > 1) h /= 2;
        data += size;
    }
}

GLenum textarget(GLenum subtarget)
{
    switch(subtarget)
    {
        case GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB:
        case GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB:
        case GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB:
        case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB:
        case GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB:
        case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB:
            return GL_TEXTURE_CUBE_MAP_ARB;
            break;
    }
    return subtarget;
}

GLenum uncompressedformat(GLenum format)
{
    switch(format)
    {
        case GL_COMPRESSED_RGB_ARB:
        case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
            return GL_RGB;
        case GL_COMPRESSED_RGBA_ARB:
        case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
        case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
        case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
            return GL_RGBA;
    }
    return GL_FALSE;
}
    
void setuptexparameters(int tnum, void *pixels, int clamp, int filter, GLenum format, GLenum target)
{
    glBindTexture(target, tnum);
    glTexParameteri(target, GL_TEXTURE_WRAP_S, clamp&1 ? GL_CLAMP_TO_EDGE : GL_REPEAT);
    if(target!=GL_TEXTURE_1D) glTexParameteri(target, GL_TEXTURE_WRAP_T, clamp&2 ? GL_CLAMP_TO_EDGE : GL_REPEAT);
    if(target==GL_TEXTURE_2D && hasAF && min(aniso, hwmaxaniso) > 0) glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, min(aniso, hwmaxaniso));
    glTexParameteri(target, GL_TEXTURE_MAG_FILTER, filter && bilinear ? GL_LINEAR : GL_NEAREST);
    glTexParameteri(target, GL_TEXTURE_MIN_FILTER,
        filter > 1 ?
            (trilinear ?
                (bilinear ? GL_LINEAR_MIPMAP_LINEAR : GL_NEAREST_MIPMAP_LINEAR) :
                (bilinear ? GL_LINEAR_MIPMAP_NEAREST : GL_NEAREST_MIPMAP_NEAREST)) :
            (filter && bilinear ? GL_LINEAR : GL_NEAREST));
    if(hasGM && filter > 1 && pixels && hwmipmap && !uncompressedformat(format))
        glTexParameteri(target, GL_GENERATE_MIPMAP_SGIS, GL_TRUE);
}

void createtexture(int tnum, int w, int h, void *pixels, int clamp, int filter, GLenum component, GLenum subtarget, int pw, int ph, int pitch, bool resize)
{
    GLenum target = textarget(subtarget), format = component, type = GL_UNSIGNED_BYTE;
    switch(component)
    {
        case GL_FLOAT_RG16_NV:
        case GL_FLOAT_R32_NV:
        case GL_RGB16F_ARB:
        case GL_RGB32F_ARB:
            format = GL_RGB;
            type = GL_FLOAT;
            break;

        case GL_RGBA16F_ARB:
        case GL_RGBA32F_ARB:
            format = GL_RGBA;
            type = GL_FLOAT;
            break;

        case GL_DEPTH_COMPONENT16:
        case GL_DEPTH_COMPONENT24:
        case GL_DEPTH_COMPONENT32:
            format = GL_DEPTH_COMPONENT;
            break;

        case GL_RGB5:
        case GL_RGB8:
        case GL_RGB16:
        case GL_COMPRESSED_RGB_ARB:
        case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
            format = GL_RGB;
            break;

        case GL_RGBA8:
        case GL_RGBA16:
        case GL_COMPRESSED_RGBA_ARB:
        case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
        case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
        case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
            format = GL_RGBA;
            break;
    }
    if(tnum) setuptexparameters(tnum, pixels, clamp, filter, format, target);
    if(!pw) pw = w;
    if(!ph) ph = h;
    int tw = w, th = h;
    bool mipmap = filter > 1 && pixels;
    if(resize && pixels) 
    {
        resizetexture(w, h, mipmap, false, target, 0, tw, th);
        if(mipmap) component = compressedformat(component, tw, th);
    }
    uploadtexture(subtarget, component, tw, th, format, type, pixels, pw, ph, pitch, mipmap); 
}

void createcompressedtexture(int tnum, int w, int h, uchar *data, int align, int blocksize, int levels, int clamp, int filter, GLenum format, GLenum subtarget)
{
    GLenum target = textarget(subtarget);
    if(tnum) setuptexparameters(tnum, data, clamp, filter, format, target);
    uploadcompressedtexture(target, format, w, h, data, align, blocksize, levels, filter > 1); 
}

hashtable<char *, Texture> textures;

Texture *notexture = NULL; // used as default, ensured to be loaded

static GLenum texformat(int bpp)
{
    switch(bpp)
    {
        case 1: return GL_LUMINANCE;
        case 2: return GL_LUMINANCE_ALPHA;
        case 3: return GL_RGB;
        case 4: return GL_RGBA;
        default: return 0;
    }
}

int texalign(void *data, int w, int bpp)
{
    size_t address = size_t(data) | (w*bpp);
    if(address&1) return 1;
    if(address&2) return 2;
    if(address&4) return 4;
    return 8;
}
    
static Texture *newtexture(Texture *t, const char *rname, ImageData &s, int clamp = 0, bool mipit = true, bool canreduce = false, bool transient = false, int compress = 0)
{
    if(!t)
    {
        char *key = newstring(rname);
        t = &textures[key];
        t->name = key;
    }

    t->clamp = clamp;
    t->mipmap = mipit;
    t->type = Texture::IMAGE;
    if(transient) t->type |= Texture::TRANSIENT;
    if(!s.data)
    {
        t->type |= Texture::STUB;
        t->w = t->h = t->xs = t->ys = t->bpp = 0;
        return t;
    }
    t->bpp = s.compressed ? formatsize(uncompressedformat(s.compressed)) : s.bpp;
    t->w = t->xs = s.w;
    t->h = t->ys = s.h;

    int filter = !canreduce || reducefilter ? (mipit ? 2 : 1) : 0;
    glGenTextures(1, &t->id);
    if(s.compressed)
    {
        uchar *data = s.data;
        int levels = s.levels, level = 0;
        if(canreduce && texreduce) loopi(min(texreduce, s.levels-1))
        {
            data += s.calclevelsize(level++);
            levels--;
            if(t->w > 1) t->w /= 2;
            if(t->h > 1) t->h /= 2;
        } 
        int sizelimit = mipit && maxtexsize ? min(maxtexsize, hwtexsize) : hwtexsize;
        while(t->w > sizelimit || t->h > sizelimit)
        {
            data += s.calclevelsize(level++);
            levels--;
            if(t->w > 1) t->w /= 2;
            if(t->h > 1) t->h /= 2;
        }
        createcompressedtexture(t->id, t->w, t->h, data, s.align, s.bpp, levels, clamp, filter, s.compressed, GL_TEXTURE_2D);
    }
    else
    {
        resizetexture(t->w, t->h, mipit, canreduce, GL_TEXTURE_2D, compress, t->w, t->h);
        GLenum format = compressedformat(texformat(t->bpp), t->w, t->h, compress);
        createtexture(t->id, t->w, t->h, s.data, clamp, filter, format, GL_TEXTURE_2D, t->xs, t->ys, s.pitch, false);
    }
    return t;
}

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
#define RGBAMASKS 0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff
#define RGBMASKS  0xff0000, 0x00ff00, 0x0000ff, 0
#else
#define RGBAMASKS 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000
#define RGBMASKS  0x0000ff, 0x00ff00, 0xff0000, 0
#endif

SDL_Surface *wrapsurface(void *data, int width, int height, int bpp)
{
    switch(bpp)
    {
        case 3: return SDL_CreateRGBSurfaceFrom(data, width, height, 8*bpp, bpp*width, RGBMASKS);
        case 4: return SDL_CreateRGBSurfaceFrom(data, width, height, 8*bpp, bpp*width, RGBAMASKS);
    }
    return NULL;
}

SDL_Surface *creatergbsurface(SDL_Surface *os)
{
    SDL_Surface *ns = SDL_CreateRGBSurface(SDL_SWSURFACE, os->w, os->h, 24, RGBMASKS);
    if(ns) SDL_BlitSurface(os, NULL, ns, NULL);
    SDL_FreeSurface(os);
    return ns;
}

SDL_Surface *creatergbasurface(SDL_Surface *os)
{
    SDL_Surface *ns = SDL_CreateRGBSurface(SDL_SWSURFACE, os->w, os->h, 32, RGBAMASKS);
    if(ns) 
    {
        SDL_SetAlpha(os, 0, 0);
        SDL_BlitSurface(os, NULL, ns, NULL);
    }
    SDL_FreeSurface(os);
    return ns;
}

bool checkgrayscale(SDL_Surface *s)
{
    // gray scale images have 256 levels, no colorkey, and the palette is a ramp
    if(s->format->palette)
    {
        if(s->format->palette->ncolors != 256 || s->format->colorkey) return false;
        const SDL_Color *colors = s->format->palette->colors;
        loopi(256) if(colors[i].r != i || colors[i].g != i || colors[i].b != i) return false;
    }
    return true;
}

SDL_Surface *fixsurfaceformat(SDL_Surface *s)
{
    if(!s) return NULL;
    if(!s->pixels || min(s->w, s->h) <= 0 || s->format->BytesPerPixel <= 0)
    { 
        SDL_FreeSurface(s); 
        return NULL; 
    }
    static const uint rgbmasks[] = { RGBMASKS }, rgbamasks[] = { RGBAMASKS };
    switch(s->format->BytesPerPixel)
    {
        case 1:
            if(!checkgrayscale(s)) return s->format->colorkey ? creatergbasurface(s) : creatergbsurface(s);
            break;
        case 3:
            if(s->format->Rmask != rgbmasks[0] || s->format->Gmask != rgbmasks[1] || s->format->Bmask != rgbmasks[2]) 
                return creatergbsurface(s);
            break;
        case 4:
            if(s->format->Rmask != rgbamasks[0] || s->format->Gmask != rgbamasks[1] || s->format->Bmask != rgbamasks[2] || s->format->Amask != rgbamasks[3])
                return creatergbasurface(s);
            break;
    }
    return s;
}

void texflip(ImageData &s)
{
    ImageData d(s.w, s.h, s.bpp);
    uchar *dst = d.data, *src = &s.data[s.pitch*s.h];
    loopi(s.h)
    {
        src -= s.pitch;
        memcpy(dst, src, s.bpp*s.w);
        dst += d.pitch;
    }
    s.replace(d);
}

void texnormal(ImageData &s, int emphasis)    
{
    ImageData d(s.w, s.h, 3);
    uchar *src = s.data, *dst = d.data;
    loop(y, s.h) loop(x, s.w)
    {
        vec normal(0.0f, 0.0f, 255.0f/emphasis);
        normal.x += src[y*s.pitch + ((x+s.w-1)%s.w)*s.bpp];
        normal.x -= src[y*s.pitch + ((x+1)%s.w)*s.bpp];
        normal.y += src[((y+s.h-1)%s.h)*s.pitch + x*s.bpp];
        normal.y -= src[((y+1)%s.h)*s.pitch + x*s.bpp];
        normal.normalize();
        *dst++ = uchar(127.5f + normal.x*127.5f);
        *dst++ = uchar(127.5f + normal.y*127.5f);
        *dst++ = uchar(127.5f + normal.z*127.5f);
    }
    s.replace(d);
}

void scaleimage(ImageData &s, int w, int h)
{
    ImageData d(w, h, s.bpp);
    scaletexture(s.data, s.w, s.h, s.bpp, s.pitch, d.data, w, h);
    s.replace(d);
}

#define readwritergbtex(t, s, body) \
    { \
        if(t.bpp >= 3) { readwritetex(t, s, body); } \
        else \
        { \
            ImageData rgb(t.w, t.h, 3); \
            read2writetex(rgb, t, orig, s, src, \
            { \
                switch(t.bpp) \
                { \
                    case 1: dst[0] = orig[0]; dst[1] = orig[0]; dst[2] = orig[0]; break; \
                    case 2: dst[0] = orig[0]; dst[1] = orig[1]; dst[2] = orig[1]; break; \
                } \
                body; \
            }); \
            t.replace(rgb); \
        } \
    }

void forcergbimage(ImageData &s)
{
    if(s.bpp >= 3) return;
    ImageData d(s.w, s.h, 3);
    readwritetex(d, s,
        switch(s.bpp)
        {
            case 1: dst[0] = src[0]; dst[1] = src[0]; dst[2] = src[0]; break;
            case 2: dst[0] = src[0]; dst[1] = src[1]; dst[2] = src[1]; break;
        }
    );
    s.replace(d);
}

#define readwritergbatex(t, s, body) \
    { \
        if(t.bpp >= 4) { readwritetex(t, s, body); } \
        else \
        { \
            ImageData rgba(t.w, t.h, 4); \
            read2writetex(rgba, t, orig, s, src, \
            { \
                switch(t.bpp) \
                { \
                    case 1: dst[0] = orig[0]; dst[1] = orig[0]; dst[2] = orig[0]; break; \
                    case 2: dst[0] = orig[0]; dst[1] = orig[1]; dst[2] = orig[1]; break; \
                    case 3: dst[0] = orig[0]; dst[1] = orig[1]; dst[2] = orig[2]; break; \
                } \
                body; \
            }); \
            t.replace(rgba); \
        } \
    }

void forcergbaimage(ImageData &s)
{
    if(s.bpp >= 4) return;
    ImageData d(s.w, s.h, 4);
    readwritetex(d, s,
        switch(s.bpp)
        {
            case 1: dst[0] = src[0]; dst[1] = src[0]; dst[2] = src[0]; break;
            case 2: dst[0] = src[0]; dst[1] = src[1]; dst[2] = src[1]; break;
            case 3: dst[0] = src[0]; dst[1] = src[1]; dst[2] = src[2]; break;
        }
    );
    s.replace(d);
}

SDL_Surface *loadsurface(const char *name)
{
    SDL_Surface *s = NULL;
    stream *z = openzipfile(name, "rb");
    if(z)
    {
        SDL_RWops *rw = z->rwops();
        if(rw) 
        {
            s = IMG_Load_RW(rw, 0);
            SDL_FreeRW(rw);
        }
        delete z;
    }
    if(!s) s = IMG_Load(findfile(name, "rb"));
    return fixsurfaceformat(s);
}
   
static vec parsevec(const char *arg)
{
    vec v(0, 0, 0);
    int i = 0;
    for(; arg[0] && (!i || arg[0]=='/') && i<3; arg += strcspn(arg, "/,><"), i++)
    {
        if(i) arg++;
        v[i] = atof(arg);
    }
    if(i==1) v.y = v.z = v.x;
    return v;
}

VAR(usedds, 0, 1, 1);

static bool texturedata(ImageData &d, const char *tname, Slot::Tex *tex = NULL, bool msg = true, int *compress = NULL)
{
    const char *cmds = NULL, *file = tname;

    if(!tname)
    {
        if(!tex) return false;
        if(tex->name[0]=='<') 
        {
            cmds = tex->name;
            file = strrchr(tex->name, '>');
            if(!file) { if(msg) conoutf(CON_ERROR, "could not load texture packages/%s", tex->name); return false; }
            file++;
        }
        else file = tex->name;
        
        static string pname;
        formatstring(pname)("packages/%s", file);
        file = path(pname);
    }
    else if(tname[0]=='<') 
    {
        cmds = tname;
        file = strrchr(tname, '>');
        if(!file) { if(msg) conoutf(CON_ERROR, "could not load texture %s", tname); return NULL; }
        file++;
    }

    bool raw = !usedds || !compress, dds = false;
    for(const char *pcmds = cmds; pcmds;)
    {
        #define PARSETEXCOMMANDS(cmds) \
            const char *cmd = NULL, *end = NULL, *arg[4] = { NULL, NULL, NULL, NULL }; \
            cmd = &cmds[1]; \
            end = strchr(cmd, '>'); \
            if(!end) break; \
            cmds = strchr(cmd, '<'); \
            size_t len = strcspn(cmd, ":,><"); \
            loopi(4) \
            { \
                arg[i] = strchr(i ? arg[i-1] : cmd, i ? ',' : ':'); \
                if(!arg[i] || arg[i] >= end) arg[i] = ""; \
                else arg[i]++; \
            }
        PARSETEXCOMMANDS(pcmds);
        if(!strncmp(cmd, "noff", len))
        {
            if(renderpath==R_FIXEDFUNCTION) return true;
        }
        else if(!strncmp(cmd, "ffcolor", len) || !strncmp(cmd, "ffmask", len))
        {
            if(renderpath==R_FIXEDFUNCTION) raw = true;
        }
        else if(!strncmp(cmd, "decal", len))
        {
            if(renderpath==R_FIXEDFUNCTION && !hasTE) raw = true;
        }
        else if(!strncmp(cmd, "dds", len)) dds = true;
    }

    if(msg) renderprogress(loadprogress, file);

    int flen = strlen(file);
    if(flen >= 4 && (!strcasecmp(file + flen - 4, ".dds") || dds))
    {
        string dfile;
        copystring(dfile, file);
        memcpy(dfile + flen - 4, ".dds", 4);
        if(!raw && hasTC && loaddds(dfile, d)) return true;
        if(!dds) { if(msg) conoutf(CON_ERROR, "could not load texture %s", dfile); return false; }
    }
        
    SDL_Surface *s = loadsurface(file);
    if(!s) { if(msg) conoutf(CON_ERROR, "could not load texture %s", file); return false; }
    int bpp = s->format->BitsPerPixel;
    if(bpp%8 || !texformat(bpp/8)) { SDL_FreeSurface(s); conoutf(CON_ERROR, "texture must be 8, 16, 24, or 32 bpp: %s", file); return false; }
    if(max(s->w, s->h) > (1<<12)) { SDL_FreeSurface(s); conoutf(CON_ERROR, "texture size exceeded %dx%d pixels: %s", 1<<12, 1<<12, file); return false; }
    d.wrap(s);

    while(cmds)
    {
        PARSETEXCOMMANDS(cmds);
        if(!strncmp(cmd, "mad", len)) texmad(d, parsevec(arg[0]), parsevec(arg[1])); 
        else if(!strncmp(cmd, "ffcolor", len))
        {
            if(renderpath==R_FIXEDFUNCTION) texmad(d, parsevec(arg[0]), parsevec(arg[1]));
        }
        else if(!strncmp(cmd, "ffmask", len)) 
        {
            texffmask(d, atoi(arg[0]));
            if(!d.data) return true;
        }
        else if(!strncmp(cmd, "normal", len)) 
        {
            int emphasis = atoi(arg[0]);
            texnormal(d, emphasis > 0 ? emphasis : 3);
        }
        else if(!strncmp(cmd, "dup", len)) texdup(d, atoi(arg[0]), atoi(arg[1]));
        else if(!strncmp(cmd, "decal", len)) texdecal(d);
        else if(!strncmp(cmd, "offset", len)) texoffset(d, atoi(arg[0]), atoi(arg[1]));
        else if(!strncmp(cmd, "rotate", len)) texrotate(d, atoi(arg[0]), tex ? tex->type : 0);
        else if(!strncmp(cmd, "reorient", len)) texreorient(d, atoi(arg[0])>0, atoi(arg[1])>0, atoi(arg[2])>0, tex ? tex->type : TEX_DIFFUSE);
        else if(!strncmp(cmd, "mix", len)) texmix(d, *arg[0] ? atoi(arg[0]) : -1, *arg[1] ? atoi(arg[1]) : -1, *arg[2] ? atoi(arg[2]) : -1, *arg[3] ? atoi(arg[3]) : -1);
        else if(!strncmp(cmd, "compress", len) || !strncmp(cmd, "dds", len)) 
        { 
            int scale = atoi(arg[0]);
            if(scale <= 0) scale = 2;
            if(compress) *compress = scale;
        }
        else if(!strncmp(cmd, "thumbnail", len))
        {
            int w = atoi(arg[0]), h = atoi(arg[1]);
            if(w <= 0 || w > (1<<12)) w = 64;
            if(h <= 0 || h > (1<<12)) h = w;
            if(d.w > w || d.h > h) scaleimage(d, w, h);
        }
    }

    return true;
}

void loadalphamask(Texture *t)
{
    if(t->alphamask || t->bpp!=4) return;
    ImageData s;
    if(!texturedata(s, t->name, NULL, false) || !s.data || s.bpp!=4 || s.compressed) return;
    t->alphamask = new uchar[s.h * ((s.w+7)/8)];
    uchar *srcrow = s.data, *dst = t->alphamask-1;
    loop(y, s.h)
    {
        uchar *src = srcrow+s.bpp-1;
        loop(x, s.w)
        {
            int offset = x%8;
            if(!offset) *++dst = 0;
            if(*src) *dst |= 1<<offset;
            src += s.bpp;
        }
        srcrow += s.pitch;
    }
}

Texture *textureload(const char *name, int clamp, bool mipit, bool msg)
{
    string tname;
    copystring(tname, name);
    Texture *t = textures.access(path(tname));
    if(t) return t;
    int compress = 0;
    ImageData s;
    if(texturedata(s, tname, NULL, msg, &compress)) return newtexture(NULL, tname, s, clamp, mipit, false, false, compress);
    return notexture;
}

void settexture(const char *name, int clamp)
{
    glBindTexture(GL_TEXTURE_2D, textureload(name, clamp, true, false)->id);
}

vector<Slot> slots;
Slot materialslots[MATF_VOLUME+1];

VAR(curtexnum, 1, 0, 0);
int curmatslot = -1;

void texturereset(int *n)
{
    if(!overrideidents && !game::allowedittoggle()) return;
    resetslotshader();
    curtexnum = clamp(*n, 0, curtexnum);
    slots.setsize(curtexnum);
}

COMMAND(texturereset, "i");

void materialreset()
{
    if(!overrideidents && !game::allowedittoggle()) return;
    loopi(MATF_VOLUME+1) materialslots[i].reset();
}

COMMAND(materialreset, "");

void texture(char *type, char *name, int *rot, int *xoffset, int *yoffset, float *scale, int *forcedindex) // INTENSITY: forcedindex
{
    if(curtexnum<0 || curtexnum>=0x10000) return;
    struct { const char *name; int type; } types[] =
    {
        {"c", TEX_DIFFUSE},
        {"u", TEX_UNKNOWN},
        {"d", TEX_DECAL},
        {"n", TEX_NORMAL},
        {"g", TEX_GLOW},
        {"s", TEX_SPEC},
        {"z", TEX_DEPTH},
        {"e", TEX_ENVMAP}
    };
    int tnum = -1, matslot = findmaterial(type);
    loopi(sizeof(types)/sizeof(types[0])) if(!strcmp(types[i].name, type)) { tnum = i; break; }
    if(tnum<0) tnum = atoi(type);
    if(tnum==TEX_DIFFUSE)
    {
        if(matslot>=0) curmatslot = matslot;
        else { curmatslot = -1; curtexnum++; }
    }
    else if(curmatslot>=0) matslot = curmatslot;
    else if(!curtexnum) return;

    assert(*forcedindex <= 0 || slots.inrange(*forcedindex)); // INTENSITY
    if (*forcedindex > 0 && tnum==TEX_DIFFUSE) // INTENSITY: reset old slots we force the index of
        slots[*forcedindex].reset();
    Slot &s = matslot>=0 ? materialslots[matslot] : (*forcedindex <= 0 ? (tnum!=TEX_DIFFUSE ? slots.last() : slots.add()) : slots[*forcedindex]); // INTENSITY: Allow forced indexes

    s.loaded = false;
    s.texmask |= 1<<tnum;
    if(s.sts.length()>=8) conoutf(CON_WARN, "warning: too many textures in slot %d", curtexnum);
    Slot::Tex &st = s.sts.add();
    st.type = tnum;
    st.combined = -1;
    st.t = NULL;
    copystring(st.name, name);
    path(st.name);
    if(tnum==TEX_DIFFUSE)
    {
        setslotshader(s);
        s.rotation = clamp(*rot, 0, 5);
        s.xoffset = max(*xoffset, 0);
        s.yoffset = max(*yoffset, 0);
        s.scale = *scale <= 0 ? 1 : *scale;
    }
}

COMMAND(texture, "ssiiifi");

void autograss(char *name)
{
    Slot &s = slots.last();
    DELETEA(s.autograss);
    s.autograss = name[0] ? newstring(makerelpath("packages", name)) : NULL;
}
COMMAND(autograss, "s");

void texscroll(float *scrollS, float *scrollT)
{
    if(slots.empty()) return;
    Slot &s = slots.last();
    s.scrollS = *scrollS/1000.0f;
    s.scrollT = *scrollT/1000.0f;
}
COMMAND(texscroll, "ff");

void texoffset_(int *xoffset, int *yoffset)
{
    if(slots.empty()) return;
    Slot &s = slots.last();
    s.xoffset = max(*xoffset, 0);
    s.yoffset = max(*yoffset, 0);
}
COMMANDN(texoffset, texoffset_, "ii");

void texrotate_(int *rot)
{
    if(slots.empty()) return;
    Slot &s = slots.last();
    s.rotation = clamp(*rot, 0, 5);
}
COMMANDN(texrotate, texrotate_, "i");

void texscale(float *scale)
{
    if(slots.empty()) return;
    Slot &s = slots.last();
    s.scale = *scale <= 0 ? 1 : *scale;
}
COMMAND(texscale, "f");

void texlayer(int *layer, char *name, int *mode, float *scale)
{
    if(slots.empty()) return;
    Slot &s = slots.last();
    s.layer = *layer < 0 ? max(slots.length()-1+*layer, 0) : *layer;
    s.layermaskname = name[0] ? newstring(path(makerelpath("packages", name))) : NULL; 
    s.layermaskmode = *mode;
    s.layermaskscale = *scale <= 0 ? 1 : *scale;
}
COMMAND(texlayer, "isif");

static int findtextype(Slot &s, int type, int last = -1)
{
    for(int i = last+1; i<s.sts.length(); i++) if((type&(1<<s.sts[i].type)) && s.sts[i].combined<0) return i;
    return -1;
}

static void addbump(ImageData &c, ImageData &n)
{
    if(n.bpp < 3) return;
    readwritergbtex(c, n,
        loopk(3) dst[k] = int(dst[k])*(int(src[2])*2-255)/255;
    );
}

static void addglow(ImageData &c, ImageData &g, const vec &glowcolor)
{
    if(g.bpp < 3)
    {
        readwritergbtex(c, g,
            loopk(3) dst[k] = clamp(int(dst[k]) + int(src[0]*glowcolor[k]), 0, 255);
        );
    }
    else
    {
        readwritergbtex(c, g,
            loopk(3) dst[k] = clamp(int(dst[k]) + int(src[k]*glowcolor[k]), 0, 255);
        );
    }
}

static void blenddecal(ImageData &c, ImageData &d)
{
    if(d.bpp < 4) return;
    readwritergbtex(c, d,
        uchar a = src[3];
        loopk(3) dst[k] = (int(src[k])*int(a) + int(dst[k])*int(255-a))/255;
    );
}

static void mergespec(ImageData &c, ImageData &s)
{
    if(s.bpp < 3)
    {
        readwritergbatex(c, s,
            dst[3] = src[0];
        );
    }
    else
    {
        readwritergbatex(c, s,
            dst[3] = (int(src[0]) + int(src[1]) + int(src[2]))/3;
        );
    }
}

static void mergedepth(ImageData &c, ImageData &z)
{
    readwritergbatex(c, z,
        dst[3] = src[0];
    );
}

static void addname(vector<char> &key, Slot &slot, Slot::Tex &t, bool combined = false, const char *prefix = NULL)
{
    if(combined) key.add('&');
    if(prefix) { while(*prefix) key.add(*prefix++); }
    defformatstring(tname)("packages/%s", t.name);
    for(const char *s = path(tname); *s; key.add(*s++));
}

static void texcombine(Slot &s, int index, Slot::Tex &t, bool forceload = false)
{
    if(renderpath==R_FIXEDFUNCTION && t.type!=TEX_DIFFUSE && t.type!=TEX_GLOW && !forceload) { t.t = notexture; return; }
    vector<char> key; 
    addname(key, s, t);
    switch(t.type)
    {
        case TEX_DIFFUSE:
            if(renderpath==R_FIXEDFUNCTION)
            {
                for(int i = -1; (i = findtextype(s, (1<<TEX_DECAL)|(1<<TEX_NORMAL), i))>=0;)
                {
                    s.sts[i].combined = index;
                    addname(key, s, s.sts[i], true);
                }
                break;
            } // fall through to shader case

        case TEX_NORMAL:
        {
            if(renderpath==R_FIXEDFUNCTION) break;
            int i = findtextype(s, t.type==TEX_DIFFUSE ? (1<<TEX_SPEC) : (1<<TEX_DEPTH));
            if(i<0) break;
            s.sts[i].combined = index;
            addname(key, s, s.sts[i], true);
            break;
        }
    }
    key.add('\0');
    t.t = textures.access(key.getbuf());
    if(t.t) return;
    int compress = 0;
    ImageData ts;
    if(!texturedata(ts, NULL, &t, true, &compress)) { t.t = notexture; return; }
    switch(t.type)
    {
        case TEX_DIFFUSE:
            if(renderpath==R_FIXEDFUNCTION)
            {
                if(!ts.compressed) loopv(s.sts)
                {
                    Slot::Tex &b = s.sts[i];
                    if(b.combined!=index) continue;
                    ImageData bs;
                    if(!texturedata(bs, NULL, &b)) continue;
                    if(bs.w!=ts.w || bs.h!=ts.h) scaleimage(bs, ts.w, ts.h);
                    switch(b.type)
                    {
                        case TEX_DECAL: blenddecal(ts, bs); break;
                        case TEX_NORMAL: addbump(ts, bs); break;
                    }
                }
                break;
            } // fall through to shader case

        case TEX_NORMAL:
            if(!ts.compressed) loopv(s.sts)
            {
                Slot::Tex &a = s.sts[i];
                if(a.combined!=index) continue;
                ImageData as;
                if(!texturedata(as, NULL, &a)) continue;
                //if(ts.bpp!=4) forcergbaimage(ts);
                if(as.w!=ts.w || as.h!=ts.h) scaleimage(as, ts.w, ts.h);
                switch(a.type)
                {
                    case TEX_SPEC: mergespec(ts, as); break;
                    case TEX_DEPTH: mergedepth(ts, as); break;
                }
                break; // only one combination
            }
            break;
    }
    t.t = newtexture(NULL, key.getbuf(), ts, 0, true, true, true, compress);
}

Slot dummyslot;

static Slot &loadslot(Slot &s, bool forceload)
{
    linkslotshader(s);
    loopv(s.sts)
    {
        Slot::Tex &t = s.sts[i];
        if(t.combined>=0) continue;
        switch(t.type)
        {
            case TEX_ENVMAP:
                if(hasCM && (renderpath!=R_FIXEDFUNCTION || forceload)) t.t = cubemapload(t.name);
                break;

            default:
                texcombine(s, i, t, forceload);
                break;
        }
    }
    s.loaded = true;
    return s;
}

Slot &lookupmaterialslot(int slot, bool load)
{
    Slot &s = materialslots[slot];
    return s.loaded || !load ? s : loadslot(s, true);
}

Slot &lookuptexture(int slot, bool load)
{
    Slot &s = slots.inrange(slot) ? slots[slot] : (slots.empty() ? dummyslot : slots[0]);
    return s.loaded || !load ? s : loadslot(s, false);
}

void linkslotshaders()
{
    loopv(slots) if(slots[i].loaded) linkslotshader(slots[i]);
    loopi(MATF_VOLUME+1) if(materialslots[i].loaded) linkslotshader(materialslots[i]);
}

Texture *loadthumbnail(Slot &slot)
{
    if(slot.thumbnail) return slot.thumbnail;
    linkslotshader(slot, false);
    vector<char> name;
    addname(name, slot, slot.sts[0], false, "<thumbnail>");
    int glow = -1;
    if(slot.texmask&(1<<TEX_GLOW)) 
    { 
        loopvj(slot.sts) if(slot.sts[j].type==TEX_GLOW) { glow = j; break; } 
        if(glow >= 0) 
        {
            defformatstring(prefix)("<mad:%.2f/%.2f/%.2f>", slot.glowcolor.x, slot.glowcolor.y, slot.glowcolor.z); 
            addname(name, slot, slot.sts[glow], true, prefix);
        }
    }
    Slot *layer = slot.layer ? &lookuptexture(slot.layer, false) : NULL;
    if(layer) addname(name, *layer, layer->sts[0], true, "<layer>");
    name.add('\0');
    Texture *t = textures.access(path(name.getbuf()));
    if(t) slot.thumbnail = t;
    else
    {
        ImageData s, g, l;
        texturedata(s, NULL, &slot.sts[0], false);
        if(glow >= 0) texturedata(g, NULL, &slot.sts[glow], false);
        if(layer) texturedata(l, NULL, &layer->sts[0], false);
        if(!s.data) slot.thumbnail = notexture;
        else
        {
            int xs = s.w, ys = s.h;
            if(s.w > 64 || s.h > 64) scaleimage(s, min(s.w, 64), min(s.h, 64));
            if(g.data)
            {
                if(g.w != s.w || g.h != s.h) scaleimage(g, s.w, s.h);
                addglow(s, g, slot.glowcolor);
            }
            if(l.data)
            {
                if(l.w != s.w/2 || l.h != s.h/2) scaleimage(l, s.w/2, s.h/2);
                forcergbimage(s);
                forcergbimage(l); 
                uchar *dstrow = &s.data[s.pitch*l.h + s.bpp*l.w], *srcrow = l.data;
                loop(y, l.h) 
                {
                    for(uchar *dst = dstrow, *src = srcrow, *end = &srcrow[l.w*l.bpp]; src < end; dst += s.bpp, src += l.bpp)
                        loopk(3) dst[k] = src[k]; 
                    dstrow += s.pitch;
                    srcrow += l.pitch;
                }
            }
            t = newtexture(NULL, name.getbuf(), s, 0, false, false, true);
            t->xs = xs;
            t->ys = ys;
            slot.thumbnail = t;
        }
    }
    return t;
}

void loadlayermasks()
{
    loopv(slots)
    {
        Slot &slot = slots[i];
        if(slot.loaded && slot.layermaskname && !slot.layermask) 
        {
            slot.layermask = new ImageData;
            texturedata(*slot.layermask, slot.layermaskname);
            if(!slot.layermask->data) DELETEP(slot.layermask);
        }
    }
}

// environment mapped reflections

cubemapside cubemapsides[6] =
{
    { GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB, "lf", true,  true,  true  },
    { GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB, "rt", false, false, true  },
    { GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB, "ft", true,  false, false },
    { GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB, "bk", false, true,  false },
    { GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB, "dn", false, false, true  },
    { GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB, "up", false, false, true  },
};

VARFP(envmapsize, 4, 7, 10, setupmaterials());

Texture *cubemaploadwildcard(Texture *t, const char *name, bool mipit, bool msg, bool transient = false)
{
    if(!hasCM) return NULL;
    string tname;
    if(!name) copystring(tname, t->name);
    else
    {
        copystring(tname, name);
        t = textures.access(path(tname));
        if(t) 
        {
            if(!transient && t->type&Texture::TRANSIENT) t->type &= ~Texture::TRANSIENT;
            return t;
        }
    }
    char *wildcard = strchr(tname, '*');
    ImageData surface[6];
    string sname;
    if(!wildcard) copystring(sname, tname);
    GLenum format = GL_FALSE;
    int tsize = 0, compress = 0;
    loopi(6)
    {
        if(wildcard)
        {
            copystring(sname, tname, wildcard-tname+1);
            concatstring(sname, cubemapsides[i].name);
            concatstring(sname, wildcard+1);
        }
        ImageData &s = surface[i];
        texturedata(s, sname, NULL, msg, &compress);
        if(!s.data) return NULL;
        if(s.w != s.h)
        {
            if(msg) conoutf(CON_ERROR, "cubemap texture %s does not have square size", sname);
            return NULL;
        }
        if(!format) format = s.compressed ? s.compressed : texformat(s.bpp);
        else if((s.compressed ? s.compressed : texformat(s.bpp))!=format || (s.compressed && (s.w!=surface[0].w || s.h!=surface[0].h || s.levels!=surface[0].levels)))
        {
            if(msg) conoutf(CON_ERROR, "cubemap texture %s doesn't match other sides' format", sname);
            return NULL;
        }
        tsize = max(tsize, max(s.w, s.h));
    }
    if(name)
    {
        char *key = newstring(tname);
        t = &textures[key];
        t->name = key;
    }
    t->bpp = surface[0].compressed ? formatsize(uncompressedformat(format)) : surface[0].bpp;
    t->mipmap = mipit;
    t->clamp = 3;
    t->type = Texture::CUBEMAP | (transient ? Texture::TRANSIENT : 0);
    t->xs = t->ys = tsize;
    t->w = t->h = min(1<<envmapsize, tsize);
    resizetexture(t->w, t->h, mipit, false, GL_TEXTURE_CUBE_MAP_ARB, compress, t->w, t->h);
    format = compressedformat(format, t->w, t->h, compress);
    switch(format)
    {
        case GL_RGB: format = GL_RGB5; break;
    }
    glGenTextures(1, &t->id);
    int sizelimit = mipit && maxtexsize ? min(maxtexsize, hwcubetexsize) : hwcubetexsize;
    loopi(6)
    {
        ImageData &s = surface[i];
        cubemapside &side = cubemapsides[i];
        if(s.compressed)
        {
            int w = s.w, h = s.h, levels = s.levels, level = 0;
            uchar *data = s.data;
            while(levels > 1 && (w > sizelimit || h > sizelimit))
            {
                data += s.calclevelsize(level++);
                levels--;
                if(w > 1) w /= 2;
                if(h > 1) h /= 2;
            }
            createcompressedtexture(!i ? t->id : 0, w, h, data, s.align, s.bpp, levels, 3, mipit ? 2 : 1, s.compressed, side.target);
        }
        else
        {
            texreorient(s, side.flipx, side.flipy, side.swapxy);
            createtexture(!i ? t->id : 0, t->w, t->h, s.data, 3, mipit ? 2 : 1, format, side.target, s.w, s.h, s.pitch, false);
        }
    }
    return t;
}

Texture *cubemapload(const char *name, bool mipit, bool msg, bool transient)
{
    if(!hasCM) return NULL;
    string pname;
    copystring(pname, makerelpath("packages", name));
    path(pname);
    Texture *t = NULL;
    if(!strchr(pname, '*'))
    {
        defformatstring(jpgname)("%s_*.jpg", pname);
        t = cubemaploadwildcard(NULL, jpgname, mipit, false, transient);
        if(!t)
        {
            defformatstring(pngname)("%s_*.png", pname);
            t = cubemaploadwildcard(NULL, pngname, mipit, false, transient);
            if(!t && msg) conoutf(CON_ERROR, "could not load envmap %s", name);
        }
    }
    else t = cubemaploadwildcard(NULL, pname, mipit, msg, transient);
    return t;
}

VAR(envmapradius, 0, 128, 10000);

struct envmap
{
    int radius, size;
    vec o;
    GLuint tex;
};  

static vector<envmap> envmaps;
static Texture *skyenvmap = NULL;

void clearenvmaps()
{
    if(skyenvmap)
    {
        if(skyenvmap->type&Texture::TRANSIENT) cleanuptexture(skyenvmap);
        skyenvmap = NULL;
    }
    loopv(envmaps) glDeleteTextures(1, &envmaps[i].tex);
    envmaps.setsize(0);
}

VAR(aaenvmap, 0, 2, 4);

GLuint genenvmap(const vec &o, int envmapsize)
{
    int rendersize = 1<<(envmapsize+aaenvmap), sizelimit = min(hwcubetexsize, min(screen->w, screen->h));
    if(maxtexsize) sizelimit = min(sizelimit, maxtexsize);
    while(rendersize > sizelimit) rendersize /= 2;
    int texsize = min(rendersize, 1<<envmapsize);
    if(!aaenvmap) rendersize = texsize;
    GLuint tex;
    glGenTextures(1, &tex);
    glViewport(0, 0, rendersize, rendersize);
    float yaw = 0, pitch = 0;
    uchar *pixels = new uchar[3*rendersize*rendersize];
    glPixelStorei(GL_PACK_ALIGNMENT, texalign(pixels, rendersize, 3));
    loopi(6)
    {
        const cubemapside &side = cubemapsides[i];
        switch(side.target)
        {
            case GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB: // lf
                yaw = 270; pitch = 0; break;
            case GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB: // rt
                yaw = 90; pitch = 0; break;
            case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB: // ft
                yaw = 0; pitch = 0; break;
            case GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB: // bk
                yaw = 180; pitch = 0; break;
            case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB: // dn
                yaw = 90; pitch = -90; break;
            case GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB: // up
                yaw = 90; pitch = 90; break;
        }
        glFrontFace((side.flipx==side.flipy)!=side.swapxy ? GL_CCW : GL_CW);
        drawcubemap(rendersize, o, yaw, pitch, side);
        glReadPixels(0, 0, rendersize, rendersize, GL_RGB, GL_UNSIGNED_BYTE, pixels);
        createtexture(tex, texsize, texsize, pixels, 3, 2, GL_RGB5, side.target, rendersize, rendersize);
    }
    glFrontFace(GL_CCW);
    delete[] pixels;
    glViewport(0, 0, screen->w, screen->h);
    clientkeepalive();
    return tex;
}

void initenvmaps()
{
    if(!hasCM) return;
    clearenvmaps();
    extern char *skybox;
    skyenvmap = skybox[0] ? cubemapload(skybox, true, false, true) : NULL;
    const vector<extentity *> &ents = entities::getents();
    loopv(ents)
    {
        const extentity &ent = *ents[i];
        if(ent.type != ET_ENVMAP) continue;
        envmap &em = envmaps.add();
        em.radius = ent.attr1 ? max(0, min(10000, int(ent.attr1))) : envmapradius;
        em.size = ent.attr2 ? max(4, min(9, int(ent.attr2))) : 0;
        em.o = ent.o;
        em.tex = 0;
    }
}

void genenvmaps()
{
    if(envmaps.empty()) return;
    renderprogress(0, "generating environment maps...");
    int lastprogress = SDL_GetTicks();
    loopv(envmaps)
    {
        envmap &em = envmaps[i];
        em.tex = genenvmap(em.o, em.size ? em.size : envmapsize);
        if(renderedframe) continue;
        int millis = SDL_GetTicks();
        if(millis - lastprogress >= 250)
        {
            renderprogress(float(i+1)/envmaps.length(), "generating environment maps...", 0, true);
            lastprogress = millis;
        }
    }
}

ushort closestenvmap(const vec &o)
{
    ushort minemid = EMID_SKY;
    float mindist = 1e16f;
    loopv(envmaps)
    {
        envmap &em = envmaps[i];
        float dist = em.o.dist(o);
        if(dist < em.radius && dist < mindist)
        {
            minemid = EMID_RESERVED + i;
            mindist = dist;
        }
    }
    return minemid;
}

ushort closestenvmap(int orient, int x, int y, int z, int size)
{
    vec loc(x, y, z);
    int dim = dimension(orient);
    if(dimcoord(orient)) loc[dim] += size;
    loc[R[dim]] += size/2;
    loc[C[dim]] += size/2;
    return closestenvmap(loc);
}

GLuint lookupenvmap(Slot &slot)
{
    loopv(slot.sts) if(slot.sts[i].type==TEX_ENVMAP && slot.sts[i].t) return slot.sts[i].t->id;
    return skyenvmap ? skyenvmap->id : 0;
}

GLuint lookupenvmap(ushort emid)
{
    if(emid==EMID_SKY || emid==EMID_CUSTOM) return skyenvmap ? skyenvmap->id : 0;
    if(emid==EMID_NONE || !envmaps.inrange(emid-EMID_RESERVED)) return 0;
    GLuint tex = envmaps[emid-EMID_RESERVED].tex;
    return tex ? tex : (skyenvmap ? skyenvmap->id : 0);
}

void cleanuptexture(Texture *t)
{
    DELETEA(t->alphamask);
    if(t->id) { glDeleteTextures(1, &t->id); t->id = 0; }
    if(t->type&Texture::TRANSIENT) textures.remove(t->name); 
}

void cleanuptextures()
{
    clearenvmaps();
    loopv(slots) slots[i].cleanup();
    loopi(MATF_VOLUME+1) materialslots[i].cleanup();
    enumerate(textures, Texture, tex, cleanuptexture(&tex));
}

bool reloadtexture(const char *name)
{
    Texture *t = textures.access(path(name, true));
    if(t) return reloadtexture(*t);
    return true;
}

bool reloadtexture(Texture &tex)
{
    if(tex.id) return true;
    switch(tex.type&Texture::TYPE)
    {
        case Texture::STUB:
        case Texture::IMAGE:
        {
            int compress = 0;
            ImageData s;
            if(!texturedata(s, tex.name, NULL, true, &compress) || !newtexture(&tex, NULL, s, tex.clamp, tex.mipmap, false, false, compress)) return false;
            break;
        }

        case Texture::CUBEMAP:
            if(!cubemaploadwildcard(&tex, NULL, tex.mipmap, true)) return false;
            break;
    }    
    return true;
}

void reloadtex(char *name)
{
    Texture *t = textures.access(path(name, true));
    if(!t) { conoutf(CON_ERROR, "texture %s is not loaded", name); return; }
    if(t->type&Texture::TRANSIENT) { conoutf(CON_ERROR, "can't reload transient texture %s", name); return; }
    DELETEA(t->alphamask);
    Texture oldtex = *t;
    t->id = 0;
    if(!reloadtexture(*t))
    {
        if(t->id) glDeleteTextures(1, &t->id);
        *t = oldtex;
        conoutf(CON_ERROR, "failed to reload texture %s", name);
    }
}

COMMAND(reloadtex, "s");

void reloadtextures()
{
    int reloaded = 0;
    enumerate(textures, Texture, tex, 
    {
        loadprogress = float(++reloaded)/textures.numelems;
        reloadtexture(tex);
    });
    loadprogress = 0;
}

enum
{
    DDSD_CAPS                  = 0x00000001, 
    DDSD_HEIGHT                = 0x00000002,
    DDSD_WIDTH                 = 0x00000004, 
    DDSD_PITCH                 = 0x00000008, 
    DDSD_PIXELFORMAT           = 0x00001000, 
    DDSD_MIPMAPCOUNT           = 0x00020000, 
    DDSD_LINEARSIZE            = 0x00080000, 
    DDSD_BACKBUFFERCOUNT       = 0x00800000, 
    DDPF_ALPHAPIXELS           = 0x00000001, 
    DDPF_FOURCC                = 0x00000004, 
    DDPF_INDEXED               = 0x00000020, 
    DDPF_ALPHA                 = 0x00000002,
    DDPF_RGB                   = 0x00000040, 
    DDPF_COMPRESSED            = 0x00000080,
    DDPF_LUMINANCE             = 0x00020000,
    DDSCAPS_COMPLEX            = 0x00000008, 
    DDSCAPS_TEXTURE            = 0x00001000, 
    DDSCAPS_MIPMAP             = 0x00400000, 
    DDSCAPS2_CUBEMAP           = 0x00000200, 
    DDSCAPS2_CUBEMAP_POSITIVEX = 0x00000400, 
    DDSCAPS2_CUBEMAP_NEGATIVEX = 0x00000800, 
    DDSCAPS2_CUBEMAP_POSITIVEY = 0x00001000, 
    DDSCAPS2_CUBEMAP_NEGATIVEY = 0x00002000, 
    DDSCAPS2_CUBEMAP_POSITIVEZ = 0x00004000, 
    DDSCAPS2_CUBEMAP_NEGATIVEZ = 0x00008000, 
    DDSCAPS2_VOLUME            = 0x00200000,
    FOURCC_DXT1                = 0x31545844,
    FOURCC_DXT3                = 0x33545844,
    FOURCC_DXT5                = 0x35545844

};

struct DDCOLORKEY { uint dwColorSpaceLowValue, dwColorSpaceHighValue; };
struct DDPIXELFORMAT
{
    uint dwSize, dwFlags, dwFourCC;
    union { uint dwRGBBitCount, dwYUVBitCount, dwZBufferBitDepth, dwAlphaBitDepth, dwLuminanceBitCount, dwBumpBitCount, dwPrivateFormatBitCount; };
    union { uint dwRBitMask, dwYBitMask, dwStencilBitDepth, dwLuminanceBitMask, dwBumpDuBitMask, dwOperations; };
    union { uint dwGBitMask, dwUBitMask, dwZBitMask, dwBumpDvBitMask; struct { ushort wFlipMSTypes, wBltMSTypes; } MultiSampleCaps; };
    union { uint dwBBitMask, dwVBitMask, dwStencilBitMask, dwBumpLuminanceBitMask; };
    union { uint dwRGBAlphaBitMask, dwYUVAlphaBitMask, dwLuminanceAlphaBitMask, dwRGBZBitMask, dwYUVZBitMask; };

};
struct DDSCAPS2 { uint dwCaps, dwCaps2, dwCaps3, dwCaps4; };
struct DDSURFACEDESC2
{
    uint dwSize, dwFlags, dwHeight, dwWidth; 
    union { int lPitch; uint dwLinearSize; };
    uint dwBackBufferCount; 
    union { uint dwMipMapCount, dwRefreshRate, dwSrcVBHandle; };
    uint dwAlphaBitDepth, dwReserved, lpSurface; 
    union { DDCOLORKEY ddckCKDestOverlay; uint dwEmptyFaceColor; };
    DDCOLORKEY ddckCKDestBlt, ddckCKSrcOverlay, ddckCKSrcBlt;     
    union { DDPIXELFORMAT ddpfPixelFormat; uint dwFVF; };
    DDSCAPS2 ddsCaps;  
    uint dwTextureStage;   
};

VAR(dbgdds, 0, 0, 1);

bool loaddds(const char *filename, ImageData &image)
{
    stream *f = openfile(filename, "rb");
    if(!f) return false;
    GLenum format = GL_FALSE;
    uchar magic[4];
    if(f->read(magic, 4) != 4 || memcmp(magic, "DDS ", 4)) { delete f; return false; }
    DDSURFACEDESC2 d;
    if(f->read(&d, sizeof(d)) != sizeof(d)) { delete f; return false; }
    lilswap((uint *)&d, sizeof(d)/sizeof(uint));
    if(d.dwSize != sizeof(DDSURFACEDESC2) || d.ddpfPixelFormat.dwSize != sizeof(DDPIXELFORMAT)) { delete f; return false; }
    if(d.ddpfPixelFormat.dwFlags & DDPF_FOURCC)
    {
        switch(d.ddpfPixelFormat.dwFourCC)
        {
            case FOURCC_DXT1: format = d.ddpfPixelFormat.dwFlags & DDPF_ALPHAPIXELS ? GL_COMPRESSED_RGBA_S3TC_DXT1_EXT : GL_COMPRESSED_RGB_S3TC_DXT1_EXT; break;
            case FOURCC_DXT3: format = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT; break;
            case FOURCC_DXT5: format = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT; break;
        }        
    }
    if(!format) { delete f; return false; }
    if(dbgdds) conoutf(CON_DEBUG, "%s: format 0x%X, %d x %d, %d mipmaps", filename, format, d.dwWidth, d.dwHeight, d.dwMipMapCount);
    int bpp = 0;
    switch(format)
    {
        case GL_COMPRESSED_RGB_S3TC_DXT1_EXT: 
        case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT: bpp = 8; break;
        case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT: 
        case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT: bpp = 16; break;
    }
    image.setdata(NULL, d.dwWidth, d.dwHeight, bpp, d.dwMipMapCount, 4, format); 
    int size = image.calcsize();
    if(f->read(image.data, size) != size) { delete f; image.cleanup(); return false; }
    delete f;
    return true;
}

void gendds(char *infile, char *outfile)
{
    if(!hasTC) { conoutf(CON_ERROR, "OpenGL driver does not support texture compression"); return; }

    glHint(GL_TEXTURE_COMPRESSION_HINT_ARB, GL_NICEST);

    defformatstring(cfile)("<compress>%s", infile);
    extern void reloadtex(char *name);
    Texture *t = textures.access(path(cfile));
    bool preexisting = false; // INTENSITY: We clean up the texture, if it is not preexisting (usually the case)
    if(t) { reloadtex(cfile); preexisting = true; } // INTENSITY
    t = textureload(cfile);
    // INTENSITY:
    #define CLEANUPDDSTEX { if (!preexisting) { t->type |= Texture::TRANSIENT; cleanuptexture(t); } }
    if(t==notexture) { conoutf(CON_ERROR, "failed loading %s", infile); CLEANUPDDSTEX; return; } // INTENSITY

    glBindTexture(GL_TEXTURE_2D, t->id);
    GLint compressed = 0, format = 0, width = 0, height = 0; 
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_COMPRESSED_ARB, &compressed);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, &format);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);

    if(!compressed) { conoutf(CON_ERROR, "failed compressing %s", infile); CLEANUPDDSTEX; return; } // INTENSITY
    int fourcc = 0;
    switch(format)
    {
        case GL_COMPRESSED_RGB_S3TC_DXT1_EXT: fourcc = FOURCC_DXT1; conoutf("compressed as DXT1"); break;
        case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT: fourcc = FOURCC_DXT1; conoutf("compressed as DXT1a"); break;
        case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT: fourcc = FOURCC_DXT3; conoutf("compressed as DXT3"); break;
        case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT: fourcc = FOURCC_DXT5; conoutf("compressed as DXT5"); break;
        default:
            conoutf(CON_ERROR, "failed compressing %s: unknown format: 0x%X", infile, format); break;
            CLEANUPDDSTEX; // INTENSITY
            return;
    }

    if(!outfile[0])
    {
        static string buf;
        copystring(buf, infile);
        int len = strlen(buf);
        if(len > 4 && buf[len-4]=='.') memcpy(&buf[len-4], ".dds", 4);
        else concatstring(buf, ".dds");
        outfile = buf;
    }
    
    stream *f = openfile(path(outfile, true), "wb");
    if(!f) { conoutf(CON_ERROR, "failed writing to %s", outfile); CLEANUPDDSTEX; return; } // INTENSITY

    int csize = 0;
    for(int lw = width, lh = height, level = 0;;)
    {
        GLint size = 0;
        glGetTexLevelParameteriv(GL_TEXTURE_2D, level++, GL_TEXTURE_COMPRESSED_IMAGE_SIZE_ARB, &size);
        csize += size;
        if(max(lw, lh) <= 1) break;
        if(lw > 1) lw /= 2;
        if(lh > 1) lh /= 2;
    }

    DDSURFACEDESC2 d;
    memset(&d, 0, sizeof(d));
    d.dwSize = sizeof(DDSURFACEDESC2);
    d.dwWidth = width;
    d.dwHeight = height;
    d.dwLinearSize = csize;
    d.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT | DDSD_LINEARSIZE | DDSD_MIPMAPCOUNT;
    d.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_COMPLEX | DDSCAPS_MIPMAP;
    d.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
    d.ddpfPixelFormat.dwFlags = DDPF_FOURCC | (format!=GL_COMPRESSED_RGB_S3TC_DXT1_EXT ? DDPF_ALPHAPIXELS : 0);
    d.ddpfPixelFormat.dwFourCC = fourcc;
   
    uchar *data = new uchar[csize], *dst = data;
    for(int lw = width, lh = height;;)
    {
        GLint size;
        glGetTexLevelParameteriv(GL_TEXTURE_2D, d.dwMipMapCount, GL_TEXTURE_COMPRESSED_IMAGE_SIZE_ARB, &size);
        glGetCompressedTexImage_(GL_TEXTURE_2D, d.dwMipMapCount++, dst);
        dst += size;
        if(max(lw, lh) <= 1) break;
        if(lw > 1) lw /= 2;
        if(lh > 1) lh /= 2;
    }

    lilswap((uint *)&d, sizeof(d)/sizeof(uint));

    f->write("DDS ", 4);
    f->write(&d, sizeof(d));
    f->write(data, csize);
    delete f;
    
    delete[] data;

    CLEANUPDDSTEX; // INTENSITY

    conoutf("wrote DDS file %s", outfile);

    setuptexcompress();
}
COMMAND(gendds, "ss");

void writepngchunk(stream *f, const char *type, uchar *data = NULL, uint len = 0)
{
    f->putbig<uint>(len);
    f->write(type, 4);
    f->write(data, len);

    uint crc = crc32(0, Z_NULL, 0);
    crc = crc32(crc, (const Bytef *)type, 4);
    if(data) crc = crc32(crc, data, len);
    f->putbig<uint>(crc);
}

VARP(compresspng, 0, 9, 9);

void savepng(const char *filename, ImageData &image, bool flip)
{
    uchar ctype = 0;
    switch(image.bpp)
    {
        case 1: ctype = 0; break;
        case 2: ctype = 4; break;
        case 3: ctype = 2; break;
        case 4: ctype = 6; break;
        default: conoutf(CON_ERROR, "failed saving png to %s", filename); return;
    }
    stream *f = openfile(filename, "wb");
    if(!f) { conoutf(CON_ERROR, "could not write to %s", filename); return; }

    uchar signature[] = { 137, 80, 78, 71, 13, 10, 26, 10 };
    f->write(signature, sizeof(signature));

    uchar ihdr[] = { 0, 0, 0, 0, 0, 0, 0, 0, 8, ctype, 0, 0, 0 };
    *(uint *)ihdr = bigswap<uint>(image.w);
    *(uint *)(ihdr + 4) = bigswap<uint>(image.h);
    writepngchunk(f, "IHDR", ihdr, sizeof(ihdr));

    int idat = f->tell();
    uint len = 0;
    f->write("\0\0\0\0IDAT", 8);
    uint crc = crc32(0, Z_NULL, 0);
    crc = crc32(crc, (const Bytef *)"IDAT", 4);

    z_stream z;
    z.zalloc = NULL;
    z.zfree = NULL;
    z.opaque = NULL;

    if(deflateInit(&z, compresspng) != Z_OK)
        goto error;

    uchar buf[1<<12];
    z.next_out = (Bytef *)buf;
    z.avail_out = sizeof(buf);

    loopi(image.h)
    {
        uchar filter = 0;
        loopj(2)
        {
            z.next_in = j ? (Bytef *)image.data + (flip ? image.h-i-1 : i)*image.pitch : (Bytef *)&filter;
            z.avail_in = j ? image.w*image.bpp : 1;
            while(z.avail_in > 0)
            {
                if(deflate(&z, Z_NO_FLUSH) != Z_OK) goto cleanuperror;
                #define FLUSHZ do { \
                    int flush = sizeof(buf) - z.avail_out; \
                    crc = crc32(crc, buf, flush); \
                    len += flush; \
                    f->write(buf, flush); \
                    z.next_out = (Bytef *)buf; \
                    z.avail_out = sizeof(buf); \
                } while(0)
                FLUSHZ;
            }
        }
    }

    for(;;)
    {
        int err = deflate(&z, Z_FINISH);
        if(err != Z_OK && err != Z_STREAM_END) goto cleanuperror;
        FLUSHZ;
        if(err == Z_STREAM_END) break;
    }

    deflateEnd(&z);

    f->seek(idat, SEEK_SET);
    f->putbig<uint>(len);
    f->seek(0, SEEK_END);
    f->putbig<uint>(crc);

    writepngchunk(f, "IEND");

    delete f;
    return;

cleanuperror:
    deflateEnd(&z);

error:
    delete f;

    conoutf(CON_ERROR, "failed saving png to %s", filename);
}

struct tgaheader
{
    uchar  identsize;
    uchar  cmaptype;
    uchar  imagetype;
    uchar  cmaporigin[2];
    uchar  cmapsize[2];
    uchar  cmapentrysize;
    uchar  xorigin[2];
    uchar  yorigin[2];
    uchar  width[2];
    uchar  height[2];
    uchar  pixelsize;
    uchar  descbyte;
};

VARP(compresstga, 0, 1, 1);

void savetga(const char *filename, ImageData &image, bool flip)
{
    switch(image.bpp)
    {
        case 3: case 4: break;
        default: conoutf(CON_ERROR, "failed saving tga to %s", filename); return;
    }

    stream *f = openfile(filename, "wb");
    if(!f) { conoutf(CON_ERROR, "could not write to %s", filename); return; }

    tgaheader hdr;
    memset(&hdr, 0, sizeof(hdr));
    hdr.pixelsize = image.bpp*8;
    hdr.width[0] = image.w&0xFF;
    hdr.width[1] = (image.w>>8)&0xFF;
    hdr.height[0] = image.h&0xFF;
    hdr.height[1] = (image.h>>8)&0xFF;
    hdr.imagetype = compresstga ? 10 : 2;
    f->write(&hdr, sizeof(hdr));

    uchar buf[128*4];
    loopi(image.h)
    {
        uchar *src = image.data + (flip ? i : image.h - i - 1)*image.pitch;
        for(int remaining = image.w; remaining > 0;)
        {
            int raw = 1;
            if(compresstga)
            {
                int run = 1;
                for(uchar *scan = src; run < min(remaining, 128); run++)
                {
                    scan += image.bpp;
                    if(src[0]!=scan[0] || src[1]!=scan[1] || src[2]!=scan[2] || (image.bpp==4 && src[3]!=scan[3])) break;
                }
                if(run > 1)
                {
                    f->putchar(0x80 | (run-1));
                    f->putchar(src[2]); f->putchar(src[1]); f->putchar(src[0]);
                    if(image.bpp==4) f->putchar(src[3]);
                    src += run*image.bpp;
                    remaining -= run;
                    if(remaining <= 0) break;
                }
                for(uchar *scan = src; raw < min(remaining, 128); raw++)
                {
                    scan += image.bpp;
                    if(src[0]==scan[0] && src[1]==scan[1] && src[2]==scan[2] && (image.bpp!=4 || src[3]==scan[3])) break;
                }
                f->putchar(raw - 1);
            }
            else raw = min(remaining, 128);
            uchar *dst = buf;
            loopj(raw)
            {
                dst[0] = src[2];
                dst[1] = src[1];
                dst[2] = src[0];
                if(image.bpp==4) dst[3] = src[3];
                dst += image.bpp;
                src += image.bpp;
            }
            f->write(buf, raw*image.bpp);
            remaining -= raw;
        }
    }

    delete f;
}

enum
{
    IMG_BMP = 0,
    IMG_TGA = 1,
    IMG_PNG = 2,
    NUMIMG
};
 
VARP(screenshotformat, 0, IMG_PNG, NUMIMG-1);

const char *imageexts[NUMIMG] = { ".bmp", ".tga", ".png" };

int guessimageformat(const char *filename, int format = IMG_BMP)
{
    int len = strlen(filename);
    loopi(NUMIMG)
    {
        int extlen = strlen(imageexts[i]);
        if(len >= extlen && !strcasecmp(&filename[len-extlen], imageexts[i])) return i;
    }
    return format;
}

void saveimage(const char *filename, int format, ImageData &image, bool flip = false)
{
    switch(format)
    {
        case IMG_PNG: savepng(filename, image, flip); break;
        case IMG_TGA: savetga(filename, image, flip); break;
        default:
        {
            ImageData flipped(image.w, image.h, image.bpp, image.data);
            if(flip) texflip(flipped);
            SDL_Surface *s = wrapsurface(flipped.data, flipped.w, flipped.h, flipped.bpp);
            if(s) 
            {
                SDL_SaveBMP(s, findfile(filename, "wb"));
                SDL_FreeSurface(s);
            }
            break;
        }
    }
}

bool loadimage(const char *filename, ImageData &image)
{
    SDL_Surface *s = loadsurface(path(filename, true));
    if(!s) return false;
    image.wrap(s);
    return true;
}

void screenshot(char *filename)
{
    static string buf;
    int format = -1;
    if(filename[0])
    {
        path(filename);
        format = guessimageformat(filename, -1);
    }
    else
    {
        formatstring(buf)("screenshot_%d", totalmillis);
        filename = buf;
    }
    if(format < 0)
    {
        format = screenshotformat;
        if(filename != buf)
        {
            copystring(buf, filename);
            filename = buf;
        }
        concatstring(buf, imageexts[format]);         
    }

    ImageData image(screen->w, screen->h, 3);
    glPixelStorei(GL_PACK_ALIGNMENT, texalign(image.data, screen->w, 3));
    glReadPixels(0, 0, screen->w, screen->h, GL_RGB, GL_UNSIGNED_BYTE, image.data);
    saveimage(filename, format, image, true);
}

COMMAND(screenshot, "s");

void flipnormalmapy(char *destfile, char *normalfile) // jpg/png /tga-> tga
{
    ImageData ns;
    if(!loadimage(normalfile, ns)) return;
    ImageData d(ns.w, ns.h, 3);
    readwritetex(d, ns,
        dst[0] = src[0];
        dst[1] = 255 - src[1];
        dst[2] = src[2];
    );
    saveimage(destfile, guessimageformat(destfile, IMG_TGA), d);
}

void mergenormalmaps(char *heightfile, char *normalfile) // jpg/png/tga + tga -> tga
{
    ImageData hs, ns;
    if(!loadimage(heightfile, hs) || !loadimage(normalfile, ns) || hs.w != ns.w || hs.h != ns.h) return;
    ImageData d(ns.w, ns.h, 3);
    read2writetex(d, hs, srch, ns, srcn,
        *(bvec *)dst = bvec(((bvec *)srcn)->tovec().mul(2).add(((bvec *)srch)->tovec()).normalize());
    );
    saveimage(normalfile, guessimageformat(normalfile, IMG_TGA), d);
}

COMMAND(flipnormalmapy, "ss");
COMMAND(mergenormalmaps, "sss");

#include "intensity_texture.cpp" // INTENSITY

