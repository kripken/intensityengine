#include "engine.h"

VARNP(blobs, showblobs, 0, 1, 1);
VARFP(blobintensity, 0, 60, 100, resetblobs());
VARFP(blobheight, 1, 32, 128, resetblobs());
VARFP(blobfadelow, 1, 8, 32, resetblobs());
VARFP(blobfadehigh, 1, 8, 32, resetblobs());
VARFP(blobmargin, 0, 1, 16, resetblobs());

VAR(dbgblob, 0, 0, 1);

struct blobinfo
{
    vec o;
    float radius;
    int millis;
    uint startindex, endindex;
    ushort startvert, endvert;
};

struct blobvert
{
    vec pos;
    float u, v;
    bvec color;
    uchar alpha;
};

struct blobrenderer
{
    const char *texname;
    Texture *tex;
    blobinfo **cache;
    int cachesize;
    blobinfo *blobs;
    int maxblobs, startblob, endblob;
    blobvert *verts;
    int maxverts, startvert, endvert, availverts;
    ushort *indexes;
    int maxindexes, startindex, endindex, availindexes;
    
    blobinfo *lastblob, *flushblob;

    vec blobmin, blobmax;
    ivec bborigin, bbsize;
    float blobalphalow, blobalphahigh;
    uchar blobalpha;

    blobrenderer(const char *texname)
      : texname(texname), tex(NULL),
        cache(NULL), cachesize(0),
        blobs(NULL), maxblobs(0), startblob(0), endblob(0),
        verts(NULL), maxverts(0), startvert(0), endvert(0), availverts(0),
        indexes(NULL), maxindexes(0), startindex(0), endindex(0), availindexes(0),
        lastblob(NULL)
    {}

    void init(int tris)
    {
        if(cache)
        {
            DELETEA(cache);
            cachesize = 0;
        }
        if(blobs)
        {
            DELETEA(blobs);
            maxblobs = startblob = endblob = 0;
        }
        if(verts)
        {
            DELETEA(verts);
            maxverts = startvert = endvert = availverts = 0;
        }
        if(indexes)
        {
            DELETEA(indexes);
            maxindexes = startindex = endindex = availindexes = 0;
        }
        if(!tris) return;
        tex = textureload(texname, 3);
        cachesize = tris/2;
        cache = new blobinfo *[cachesize];
        memset(cache, 0, cachesize * sizeof(blobinfo *));
        maxblobs = tris/2;
        blobs = new blobinfo[maxblobs];
        memset(blobs, 0, maxblobs * sizeof(blobinfo));
        maxindexes = tris*3 + 3;
        availindexes = maxindexes - 3;
        indexes = new ushort[maxindexes];
        maxverts = min(tris*3/2 + 1, (1<<16)-1);
        availverts = maxverts - 1;
        verts = new blobvert[maxverts];
    }

    bool freeblob()
    {
        blobinfo &b = blobs[startblob];
        if(&b == lastblob) return false;

        startblob++;
        if(startblob >= maxblobs) startblob = 0;

        startvert = b.endvert;
        if(startvert>=maxverts) startvert = 0;
        availverts += b.endvert - b.startvert;

        startindex = b.endindex;
        if(startindex>=maxindexes) startindex = 0;
        availindexes += b.endindex - b.startindex;

        b.millis = 0;

        return true;
    }

    blobinfo &newblob(const vec &o, float radius)
    {
        blobinfo &b = blobs[endblob];
        int next = endblob + 1;
        if(next>=maxblobs) next = 0;
        if(next==startblob) 
        {
            lastblob = &b;
            freeblob();
        }
        endblob = next;
        b.o = o;
        b.radius = radius;
        b.millis = totalmillis;
        b.startindex = b.endindex = endindex;
        b.startvert = b.endvert = endvert;
        lastblob = &b;
        return b;
    }

    void clearblobs()
    {
        startblob = endblob = 0;
        startvert = endvert = 0;
        availverts = maxverts - 1;
        startindex = endindex = 0;
        availindexes = maxindexes - 3;
    }
    
   
    template<int C>
    static int split(const vec *in, int numin, float val, vec *out)
    {
        int numout = 0;
        const vec *n = in;
        float c = (*n)[C];
        loopi(numin-1)
        {
            const vec &p = *n++;
            float pc = c;
            c = (*n)[C];
            out[numout++] = p;
            if(pc < val ? c > val : pc > val && c < val) (out[numout++] = *n).sub(p).mul((pc - val) / (pc - c)).add(p);
        }
        float ic = (*in)[C];
        out[numout++] = *n;
        if(c < val ? ic > val : c > val && ic < val) (out[numout++] = *in).sub(*n).mul((c - val) / (c - ic)).add(*n);
        return numout;
    }

    template<int C>
    static int clipabove(const vec *in, int numin, float val, vec *out)
    {
        int numout = 0;
        const vec *n = in;
        float c = (*n)[C];
        loopi(numin-1)
        {
            const vec &p = *n++;
            float pc = c;
            c = (*n)[C];
            if(pc >= val) 
            {
                out[numout++] = p;
                if(pc > val && c < val) (out[numout++] = *n).sub(p).mul((pc - val) / (pc - c)).add(p);
            }
            else if(c > val) (out[numout++] = *n).sub(p).mul((pc - val) / (pc - c)).add(p);
        }
        float ic = (*in)[C];
        if(c >= val)
        {   
            out[numout++] = *n;
            if(c > val && ic < val) (out[numout++] = *in).sub(*n).mul((c - val) / (c - ic)).add(*n);
        }
        else if(ic > val) (out[numout++] = *in).sub(*n).mul((c - val) / (c - ic)).add(*n);
        return numout;
    }

    template<int C>
    static int clipbelow(const vec *in, int numin, float val, vec *out)
    {
        int numout = 0;
        const vec *n = in;
        float c = (*n)[C];
        loopi(numin-1)
        {
            const vec &p = *n++;
            float pc = c;
            c = (*n)[C];
            if(pc <= val)   
            {
                out[numout++] = p;
                if(pc < val && c > val) (out[numout++] = *n).sub(p).mul((pc - val) / (pc - c)).add(p);
            }
            else if(c < val) (out[numout++] = *n).sub(p).mul((pc - val) / (pc - c)).add(p);
        }
        float ic = (*in)[C];
        if(c <= val)
        { 
            out[numout++] = *n;
            if(c < val && ic > val) (out[numout++] = *in).sub(*n).mul((c - val) / (c - ic)).add(*n);
        }
        else if(ic < val) (out[numout++] = *in).sub(*n).mul((c - val) / (c - ic)).add(*n);
        return numout;
    }

    void dupblob()
    {
        if(lastblob->startvert >= lastblob->endvert) 
        {
            lastblob->startindex = lastblob->endindex = endindex;
            lastblob->startvert = lastblob->endvert = endvert;
            return; 
        }
        blobinfo &b = newblob(lastblob->o, lastblob->radius);
        b.millis = -1;
    }

    inline int addvert(const vec &pos)
    {
        blobvert &v = verts[endvert];
        v.pos = pos;
        v.u = (pos.x - blobmin.x) / (blobmax.x - blobmin.x);
        v.v = (pos.y - blobmin.y) / (blobmax.y - blobmin.y);
        v.color = bvec(255, 255, 255);
        if(pos.z < blobmin.z + blobfadelow) v.alpha = uchar(blobalphalow * (pos.z - blobmin.z));
        else if(pos.z > blobmax.z - blobfadehigh) v.alpha = uchar(blobalphahigh * (blobmax.z - pos.z));
        else v.alpha = blobalpha;
        return endvert++;
    }

    void addtris(const vec *v, int numv)
    {
        if(endvert != int(lastblob->endvert) || endindex != int(lastblob->endindex)) dupblob();
        for(const vec *cur = &v[2], *end = &v[numv];;)
        {
            int limit = maxverts - endvert - 2;
            if(limit <= 0)
            {
                while(availverts < limit+2) if(!freeblob()) return;
                availverts -= limit+2;
                lastblob->endvert = maxverts;
                endvert = 0;
                dupblob();
                limit = maxverts - 2;
            }
            limit = min(int(end - cur), min(limit, (maxindexes - endindex)/3));
            while(availverts < limit+2) if(!freeblob()) return;
            while(availindexes < limit*3) if(!freeblob()) return;

            int i1 = addvert(v[0]), i2 = addvert(cur[-1]);
            loopk(limit)
            {
                indexes[endindex++] = i1;
                indexes[endindex++] = i2;
                i2 = addvert(*cur++);
                indexes[endindex++] = i2; 
            }

            availverts -= endvert - lastblob->endvert;
            availindexes -= endindex - lastblob->endindex;
            lastblob->endvert = endvert;
            lastblob->endindex = endindex;
            if(endvert >= maxverts) endvert = 0;
            if(endindex >= maxindexes) endindex = 0;

            if(cur >= end) break;
            dupblob();
        }
    }

    void genflattris(cube &cu, int orient, vec *v, uint overlap)
    {
        int dim = dimension(orient);
        float c = v[fv[orient][0]][dim];
        if(c < blobmin[dim] || c > blobmax[dim]) return;
        #define CLIPSIDE(flag, clip, val, check) \
            if(overlap&(flag)) \
            { \
                vec *in = v; \
                v = in==v1 ? v2 : v1; \
                numv = clip(in, numv, val, v); \
                check \
            }
        static vec v1[16], v2[16];
        loopk(4) v1[k] = v[fv[orient][k]];
        v = v1;
        int numv = 4;
        overlap &= ~(3<<(2*dim));
        CLIPSIDE(1<<0, clipabove<0>, blobmin.x, { if(numv < 3) return; });
        CLIPSIDE(1<<1, clipbelow<0>, blobmax.x, { if(numv < 3) return; });
        CLIPSIDE(1<<2, clipabove<1>, blobmin.y, { if(numv < 3) return; });
        CLIPSIDE(1<<3, clipbelow<1>, blobmax.y, { if(numv < 3) return; });
        CLIPSIDE(1<<4, clipabove<2>, blobmin.z, { if(numv < 3) return; });
        CLIPSIDE(1<<5, clipbelow<2>, blobmax.z, { if(numv < 3) return; });
        if(dim!=2)
        {
            CLIPSIDE(1<<6, split<2>, blobmin.z + blobfadelow, );
            CLIPSIDE(1<<7, split<2>, blobmax.z - blobfadehigh, );
        }

        addtris(v, numv);
    }

    void genslopedtris(cube &cu, int orient, vec *v, uint overlap)
    {
        int convexity = faceconvexity(cu, orient), order = convexity < 0 ? 1 : 0;
        const vec &p0 = v[fv[orient][0 + order]],
                  &p1 = v[fv[orient][1 + order]],
                  &p2 = v[fv[orient][2 + order]],
                  &p3 = v[fv[orient][(3 + order)&3]];
        if(p0 == p2) return;
        static vec v1[16], v2[16];
        if(p0 != p1 && p1 != p2)
        {
            if((p1.x - p0.x)*(p2.y - p0.y) - (p1.y - p0.y)*(p2.x - p0.x) < 0) goto nexttri;
            v1[0] = p0; v1[1] = p1; v1[2] = p2;
            int numv = 3;
            if(!convexity && p0 != p3 && p2 != p3) { v1[3] = p3; numv = 4; } 
            v = v1;
            CLIPSIDE(1<<0, clipabove<0>, blobmin.x, { if(numv < 3) goto nexttri; });
            CLIPSIDE(1<<1, clipbelow<0>, blobmax.x, { if(numv < 3) goto nexttri; });
            CLIPSIDE(1<<2, clipabove<1>, blobmin.y, { if(numv < 3) goto nexttri; });
            CLIPSIDE(1<<3, clipbelow<1>, blobmax.y, { if(numv < 3) goto nexttri; });
            CLIPSIDE(1<<4, clipabove<2>, blobmin.z, { if(numv < 3) goto nexttri; });
            CLIPSIDE(1<<5, clipbelow<2>, blobmax.z, { if(numv < 3) goto nexttri; });
            CLIPSIDE(1<<6, split<2>, blobmin.z + blobfadelow, );
            CLIPSIDE(1<<7, split<2>, blobmax.z - blobfadehigh, );

            addtris(v, numv);
        }
        else convexity = 1;
    nexttri:
        if(convexity && p0 != p3 && p2 != p3)
        {
            if((p2.x - p0.x)*(p3.y - p0.y) - (p2.y - p0.y)*(p3.x - p0.x) < 0) return;
            v1[0] = p0; v1[1] = p2; v1[2] = p3;
            int numv = 3;
            v = v1;
            CLIPSIDE(1<<0, clipabove<0>, blobmin.x, { if(numv < 3) return; });
            CLIPSIDE(1<<1, clipbelow<0>, blobmax.x, { if(numv < 3) return; });
            CLIPSIDE(1<<2, clipabove<1>, blobmin.y, { if(numv < 3) return; });
            CLIPSIDE(1<<3, clipbelow<1>, blobmax.y, { if(numv < 3) return; });
            CLIPSIDE(1<<4, clipabove<2>, blobmin.z, { if(numv < 3) return; });
            CLIPSIDE(1<<5, clipbelow<2>, blobmax.z, { if(numv < 3) return; });
            CLIPSIDE(1<<6, split<2>, blobmin.z + blobfadelow, );
            CLIPSIDE(1<<7, split<2>, blobmax.z - blobfadehigh, );

            addtris(v, numv);
        }
    } 

    int checkoverlap(const ivec &o, int size)
    {
        int overlap = 0;
        if(o.x < blobmin.x) overlap |= 1<<0;
        if(o.x + size > blobmax.x) overlap |= 1<<1;
        if(o.y < blobmin.y) overlap |= 1<<2;
        if(o.y + size > blobmax.y) overlap |= 1<<3;
        if(o.z < blobmin.z) overlap |= 1<<4;
        if(o.z + size > blobmax.z) overlap |= 1<<5;
        if(o.z < blobmin.z + blobfadelow && o.z + size > blobmin.z + blobfadelow) overlap |= 1<<6;
        if(o.z < blobmax.z - blobfadehigh && o.z + size > blobmax.z - blobfadehigh) overlap |= 1<<7;
        return overlap;
    }

    void gentris(cube *cu, const ivec &o, int size, uchar *vismasks = NULL, uchar avoid = 1<<O_BOTTOM)
    {
        loopoctabox(o, size, bborigin, bbsize)
        {
            ivec co(i, o.x, o.y, o.z, size);
            if(cu[i].children)
            {
                uchar visclip = cu[i].vismask & cu[i].clipmask & ~avoid;
                if(visclip)
                {
                    uint overlap = checkoverlap(co, size);
                    uchar vertused = fvmasks[visclip];
                    vec v[8];
                    loopj(8) if(vertused&(1<<j)) calcvert(cu[i], co.x, co.y, co.z, size, v[j], j, true);
                    loopj(6) if(visclip&(1<<j)) genflattris(cu[i], j, v, overlap);
                }
                if(cu[i].vismask & ~avoid) gentris(cu[i].children, co, size>>1, cu[i].vismasks, avoid | visclip);
            }
            else if(vismasks)
            {
                uchar vismask = vismasks[i] & ~avoid;
                if(!vismask) continue;
                uint overlap = checkoverlap(co, size);
                uchar vertused = fvmasks[vismask];
                bool solid = cu[i].ext && isclipped(cu[i].ext->material&MATF_VOLUME);
                vec v[8];
                loopj(8) if(vertused&(1<<j)) calcvert(cu[i], co.x, co.y, co.z, size, v[j], j, solid);
                loopj(6) if(vismask&(1<<j))
                {
                    if(solid || (flataxisface(cu[i], j) && faceedges(cu[i], j)==F_SOLID)) genflattris(cu[i], j, v, overlap);
                    else genslopedtris(cu[i], j, v, overlap);
                }
            }
            else
            {
                bool solid = cu[i].ext && isclipped(cu[i].ext->material&MATF_VOLUME);
                uchar vismask = 0;
                loopj(6) if(!(avoid&(1<<j)) && (solid ? visiblematerial(cu[i], j, co.x, co.y, co.z, size)==MATSURF_VISIBLE : cu[i].texture[j]!=DEFAULT_SKY && visibleface(cu[i], j, co.x, co.y, co.z, size))) vismask |= 1<<j;
                if(!vismask) continue;
                uint overlap = checkoverlap(co, size);
                uchar vertused = fvmasks[vismask];
                vec v[8];
                loopj(8) if(vertused&(1<<j)) calcvert(cu[i], co.x, co.y, co.z, size, v[j], j, solid);
                loopj(6) if(vismask&(1<<j)) 
                {
                    if(solid || (flataxisface(cu[i], j) && faceedges(cu[i], j)==F_SOLID)) genflattris(cu[i], j, v, overlap);
                    else genslopedtris(cu[i], j, v, overlap);
                }
            }
        }
    }

    blobinfo *addblob(const vec &o, float radius, float fade)
    {
        lastblob = &blobs[endblob];
        blobinfo &b = newblob(o, radius);
        blobmin = blobmax = o;
        blobmin.x -= radius;
        blobmin.y -= radius;
        blobmin.z -= blobheight + blobfadelow;
        blobmax.x += radius;
        blobmax.y += radius;
        blobmax.z += blobfadehigh;
        (bborigin = blobmin).sub(2);
        (bbsize = blobmax).sub(blobmin).add(4);
        float scale =  fade*blobintensity*255/100.0f;
        blobalphalow = scale / blobfadelow;
        blobalphahigh = scale / blobfadehigh;
        blobalpha = uchar(scale);
        gentris(worldroot, ivec(0, 0, 0), worldsize>>1);
        return b.millis >= 0 ? &b : NULL;
    } 

    static void setuprenderstate()
    {
        if(renderpath!=R_FIXEDFUNCTION && fogging) setfogplane(1, reflectz);

        foggedshader->set();

        enablepolygonoffset(GL_POLYGON_OFFSET_FILL);

        glDepthMask(GL_FALSE);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        if(!dbgblob) glEnable(GL_BLEND);

        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glEnableClientState(GL_COLOR_ARRAY);
    }

    static void cleanuprenderstate()
    {
        glDisableClientState(GL_VERTEX_ARRAY);
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        glDisableClientState(GL_COLOR_ARRAY);

        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);

        disablepolygonoffset(GL_POLYGON_OFFSET_FILL);
    }

    static int lastreset;

    static void reset()
    {
        lastreset = totalmillis;
    }

    static blobrenderer *lastrender;

    void fadeblob(blobinfo *b, float fade)
    {
        float minz = b->o.z - (blobheight + blobfadelow), maxz = b->o.z + blobfadehigh,
              scale = fade*blobintensity*255/100.0f, scalelow = scale / blobfadelow, scalehigh = scale / blobfadehigh;
        uchar alpha = uchar(scale); 
        b->millis = totalmillis;
        do
        {
            if(b->endvert - b->startvert >= 3) for(blobvert *v = &verts[b->startvert], *end = &verts[b->endvert]; v < end; v++)
            {
                float z = v->pos.z;
                if(z < minz + blobfadelow) v->alpha = uchar(scalelow * (z - minz));
                else if(z > maxz - blobfadehigh) v->alpha = uchar(scalehigh * (maxz - z));
                else v->alpha = alpha;
            }
            int offset = b - &blobs[0] + 1;
            if(offset >= maxblobs) offset = 0;
            if(offset < endblob ? offset > startblob || startblob > endblob : offset > startblob) b = &blobs[offset];
            else break;
        } while(b->millis < 0);
    }

    void renderblob(const vec &o, float radius, float fade)
    {
        if(lastrender != this)
        {
            if(!lastrender) 
            {
                if(!blobs) initblobs();

                setuprenderstate();
            }
            glVertexPointer(3, GL_FLOAT, sizeof(blobvert), &verts->pos);
            glTexCoordPointer(2, GL_FLOAT, sizeof(blobvert), &verts->u);
            glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(blobvert), &verts->color);
            if(!lastrender || lastrender->tex != tex) glBindTexture(GL_TEXTURE_2D, tex->id);
            lastrender = this;
        }
    
        union { int i; float f; } ox, oy;
        ox.f = o.x; oy.f = o.y;
        uint hash = uint(ox.i^~oy.i^(INT_MAX-oy.i)^uint(radius));
        hash %= cachesize;
        blobinfo *b = cache[hash];
        if(!b || b->millis <= lastreset || b->o!=o || b->radius!=radius)
        {
            b = addblob(o, radius, fade);
            cache[hash] = b;
            if(!b) return;
        }
        else if(fade < 1 && b->millis < totalmillis) fadeblob(b, fade); 
        do
        {
            if(b->endvert - b->startvert >= 3)
            {
                if(hasDRE) glDrawRangeElements_(GL_TRIANGLES, b->startvert, b->endvert-1, b->endindex - b->startindex, GL_UNSIGNED_SHORT, &indexes[b->startindex]);
                else glDrawElements(GL_TRIANGLES, b->endindex - b->startindex, GL_UNSIGNED_SHORT, &indexes[b->startindex]);
                xtravertsva += b->endvert - b->startvert;
            }
            int offset = b - &blobs[0] + 1;
            if(offset >= maxblobs) offset = 0; 
            if(offset < endblob ? offset > startblob || startblob > endblob : offset > startblob) b = &blobs[offset];
            else break; 
        } while(b->millis < 0);
    }
};

int blobrenderer::lastreset = 0;
blobrenderer *blobrenderer::lastrender = NULL;

VARFP(blobstattris, 128, 4096, 1<<16, initblobs(BLOB_STATIC));
VARFP(blobdyntris, 128, 4096, 1<<16, initblobs(BLOB_DYNAMIC));

static blobrenderer blobs[] = 
{
    blobrenderer("packages/particles/blob.png"),
    blobrenderer("packages/particles/blob.png")
};

void initblobs(int type)
{
    if(type < 0 || (type==BLOB_STATIC && blobs[BLOB_STATIC].blobs)) blobs[BLOB_STATIC].init(showblobs ? blobstattris : 0);
    if(type < 0 || (type==BLOB_DYNAMIC && blobs[BLOB_DYNAMIC].blobs)) blobs[BLOB_DYNAMIC].init(showblobs ? blobdyntris : 0);
}

void resetblobs()
{
    blobrenderer::lastreset = totalmillis;
}

void renderblob(int type, const vec &o, float radius, float fade)
{
    if(!showblobs) return;
    if(refracting < 0 && o.z - blobheight - blobfadelow >= reflectz) return;
    blobs[type].renderblob(o, radius + blobmargin, fade);
}

void flushblobs()
{
    if(blobrenderer::lastrender) blobrenderer::cleanuprenderstate();
    blobrenderer::lastrender = NULL;
}

