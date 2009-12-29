#include "engine.h"

vector<LightMap> lightmaps;

VARR(lightprecision, 1, 32, 1024);
VARR(lighterror, 1, 8, 16);
VARR(bumperror, 1, 3, 16);
VARR(lightlod, 0, 0, 10);
bvec ambientcolor(0x19, 0x19, 0x19), skylightcolor(0, 0, 0);
HVARFR(ambient, 1, 0x191919, 0xFFFFFF, 
{
    if(ambient <= 255) ambient |= (ambient<<8) | (ambient<<16);
    ambientcolor = bvec((ambient>>16)&0xFF, (ambient>>8)&0xFF, ambient&0xFF);
});
HVARFR(skylight, 0, 0, 0xFFFFFF, 
{
    if(skylight <= 255) skylight |= (skylight<<8) | (skylight<<16);
    skylightcolor = bvec((skylight>>16)&0xFF, (skylight>>8)&0xFF, skylight&0xFF);
});

static surfaceinfo brightsurfaces[6] =
{
    {{0}, 0, 0, 0, 0, LMID_BRIGHT, LAYER_TOP},
    {{0}, 0, 0, 0, 0, LMID_BRIGHT, LAYER_TOP},
    {{0}, 0, 0, 0, 0, LMID_BRIGHT, LAYER_TOP},
    {{0}, 0, 0, 0, 0, LMID_BRIGHT, LAYER_TOP},
    {{0}, 0, 0, 0, 0, LMID_BRIGHT, LAYER_TOP},
    {{0}, 0, 0, 0, 0, LMID_BRIGHT, LAYER_TOP},
};

// quality parameters, set by the calclight arg
VARN(lmshadows, lmshadows_, 0, 2, 2);
VARN(lmaa, lmaa_, 0, 3, 3);
static int lmshadows = 2, lmaa = 3;

static Slot *lmslot = NULL;
static int lmtype, lmbpp, lmorient, lmrotate;
static uchar lm[4*LM_MAXW*LM_MAXH];
static vec lm_ray[LM_MAXW*LM_MAXH];
static int lm_w, lm_h;
static vector<const extentity *> lights1, lights2;
static uint progress = 0;
static GLuint progresstex = 0;
static int progresstexticks = 0;

bool calclight_canceled = false;
volatile bool check_calclight_progress = false;

void check_calclight_canceled()
{
    if(interceptkey(SDLK_ESCAPE)) calclight_canceled = true;
    if(!calclight_canceled) check_calclight_progress = false;
}

static int curlumels = 0;

void show_calclight_progress()
{
    int lumels = curlumels;
    loopv(lightmaps) lumels += lightmaps[i].lumels;
    float bar1 = float(progress) / float(allocnodes);
          
    defformatstring(text1)("%d%% using %d textures", int(bar1 * 100), lightmaps.length());

    if(LM_PACKW <= hwtexsize && !progresstex) 
    {
        glGenTextures(1, &progresstex); 
        createtexture(progresstex, LM_PACKW, LM_PACKH, NULL, 3, 1, GL_RGB);
    }
    // only update once a sec (4 * 250 ms ticks) to not kill performance
    if(progresstex && !calclight_canceled) 
    {
        loopvrev(lightmaps) if((lightmaps[i].type&LM_TYPE)!=LM_BUMPMAP1)
        {
            if(progresstexticks++ % 4) break;
            glBindTexture(GL_TEXTURE_2D, progresstex);
            glPixelStorei(GL_UNPACK_ALIGNMENT, texalign(lightmaps[i].data, LM_PACKW, lightmaps[i].bpp));
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, LM_PACKW, LM_PACKH, 
                            lightmaps[i].type&LM_ALPHA ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, lightmaps[i].data);
            break;
        }
    }
    renderprogress(bar1, text1, progresstexticks ? progresstex : 0);
}

#define CHECK_PROGRESS(exit) CHECK_CALCLIGHT_PROGRESS(exit, show_calclight_progress)

bool PackNode::insert(ushort &tx, ushort &ty, ushort tw, ushort th)
{
    if((available < tw && available < th) || w < tw || h < th)
        return false;
    if(child1)
    {
        bool inserted = child1->insert(tx, ty, tw, th) ||
                        child2->insert(tx, ty, tw, th);
        available = max(child1->available, child2->available);
        if(!available) clear();
        return inserted;    
    }
    if(w == tw && h == th)
    {
        available = 0;
        tx = x;
        ty = y;
        return true;
    }
    
    if(w - tw > h - th)
    {
        child1 = new PackNode(x, y, tw, h);
        child2 = new PackNode(x + tw, y, w - tw, h);
    }
    else
    {
        child1 = new PackNode(x, y, w, th);
        child2 = new PackNode(x, y + th, w, h - th);
    }

    bool inserted = child1->insert(tx, ty, tw, th);
    available = max(child1->available, child2->available);
    return inserted;
}

bool LightMap::insert(ushort &tx, ushort &ty, uchar *src, ushort tw, ushort th)
{
    if((type&LM_TYPE) != LM_BUMPMAP1 && !packroot.insert(tx, ty, tw, th))
        return false;

    copy(tx, ty, src, tw, th);
    return true;
}

void LightMap::copy(ushort tx, ushort ty, uchar *src, ushort tw, ushort th)
{
    uchar *dst = data + bpp * tx + ty * bpp * LM_PACKW;
    loopi(th)
    {
        memcpy(dst, src, bpp * tw);
        dst += bpp * LM_PACKW;
        src += bpp * tw;
    }
    ++lightmaps;
    lumels += tw * th;
}

void insert_unlit(int i)
{
    LightMap &l = lightmaps[i];
    if((l.type&LM_TYPE) == LM_BUMPMAP1)
    {
        l.unlitx = l.unlity = -1;
        return;
    }
    ushort x, y;
    uchar unlit[4] = { ambientcolor[0], ambientcolor[1], ambientcolor[2], 255 };
    if(l.insert(x, y, unlit, 1, 1))
    {
        if((l.type&LM_TYPE) == LM_BUMPMAP0)
        {
            bvec front(128, 128, 255);
            ASSERT(lightmaps[i+1].insert(x, y, front.v, 1, 1));
        }
        l.unlitx = x;
        l.unlity = y;
    }
}

void insert_lightmap(ushort &x, ushort &y, uchar &lmid)
{
    loopv(lightmaps)
    {
        if(lightmaps[i].type == lmtype && lightmaps[i].insert(x, y, lm, lm_w, lm_h))
        {
            lmid = i + LMID_RESERVED;
            if((lmtype&LM_TYPE) == LM_BUMPMAP0) ASSERT(lightmaps[i+1].insert(x, y, (uchar *)lm_ray, lm_w, lm_h));
            return;
        }
    }

    lmid = lightmaps.length() + LMID_RESERVED;
    LightMap &l = lightmaps.add();
    l.type = lmtype;
    l.bpp = lmbpp;
    l.data = new uchar[lmbpp*LM_PACKW*LM_PACKH];
    memset(l.data, 0, lmbpp*LM_PACKW*LM_PACKH);
    ASSERT(l.insert(x, y, lm, lm_w, lm_h));
    if((lmtype&LM_TYPE) == LM_BUMPMAP0)
    {
        LightMap &r = lightmaps.add();
        r.type = LM_BUMPMAP1 | (lmtype&~LM_TYPE);
        r.bpp = 3;
        r.data = new uchar[3*LM_PACKW*LM_PACKH];
        memset(r.data, 0, 3*LM_PACKW*LM_PACKH);
        ASSERT(r.insert(x, y, (uchar *)lm_ray, lm_w, lm_h));
    }
}

void copy_lightmap(surfaceinfo &surface)
{
    lightmaps[surface.lmid-LMID_RESERVED].copy(surface.x, surface.y, lm, lm_w, lm_h);
    if((lmtype&LM_TYPE)==LM_BUMPMAP0 && lightmaps.inrange(surface.lmid+1-LMID_RESERVED))
        lightmaps[surface.lmid+1-LMID_RESERVED].copy(surface.x, surface.y, (uchar *)lm_ray, lm_w, lm_h);
}

struct compresskey 
{ 
    ushort x, y, lmid;
    uchar w, h;

    compresskey() {}
    compresskey(const surfaceinfo &s) : x(s.x), y(s.y), lmid(s.lmid), w(s.w), h(s.h) {}
};

struct compressval 
{ 
    ushort x, y, lmid;

    compressval() {}
    compressval(const surfaceinfo &s) : x(s.x), y(s.y), lmid(s.lmid) {} 
};

static inline bool htcmp(const compresskey &x, const compresskey &y)
{
    if(lm_w != y.w || lm_h != y.h) return false;
    LightMap &ylm = lightmaps[y.lmid - LMID_RESERVED];
    if(lmtype != ylm.type) return false;
    const uchar *xcolor = lm, *ycolor = ylm.data + lmbpp*(y.x + y.y*LM_PACKW);
    loopi(lm_h)
    {
        if(memcmp(xcolor, ycolor, lmbpp*lm_w)) return false;
        xcolor += lmbpp*lm_w;
        ycolor += lmbpp*LM_PACKW;
    }
    if((lmtype&LM_TYPE) != LM_BUMPMAP0) return true;
    const bvec *xdir = (bvec *)lm_ray, *ydir = (bvec *)lightmaps[y.lmid+1 - LMID_RESERVED].data;
    loopi(lm_h)
    {
        if(memcmp(xdir, ydir, lm_w*sizeof(bvec))) return false;
        xdir += lm_w;
        ydir += LM_PACKW;
    }
    return true;
}
    
static inline uint hthash(const compresskey &k)
{
    uint hash = lm_w + (lm_h<<8);
    const uchar *color = lm;
    loopi(lm_w*lm_h)
    {
       hash ^= (color[0] + (color[1] << 8) + (color[2] << 16));
       color += lmbpp;
    }
    return hash;  
}

static hashtable<compresskey, compressval> compressed;

VAR(lightcompress, 0, 3, 6);

bool pack_lightmap(surfaceinfo &surface) 
{
    if((int)lm_w <= lightcompress && (int)lm_h <= lightcompress)
    {
        compressval *val = compressed.access(compresskey());
        if(!val)
        {
            insert_lightmap(surface.x, surface.y, surface.lmid);
            compressed[surface] = surface;
        }
        else
        {
            surface.x = val->x;
            surface.y = val->y;
            surface.lmid = val->lmid;
            return false;
        }
    }
    else insert_lightmap(surface.x, surface.y, surface.lmid);
    return true;
}

void update_lightmap(const surfaceinfo &surface)
{
    if(max(LM_PACKW, LM_PACKH) > hwtexsize) return;

    LightMap &lm = lightmaps[surface.lmid-LMID_RESERVED];
    if(lm.tex < 0)
    {
        lm.offsetx = lm.offsety = 0;
        lm.tex = lightmaptexs.length();
        LightMapTexture &tex = lightmaptexs.add();
        tex.type = renderpath==R_FIXEDFUNCTION ? (lm.type&~LM_TYPE) | LM_DIFFUSE : lm.type;
        tex.w = LM_PACKW;
        tex.h = LM_PACKH;
        tex.unlitx = lm.unlitx;
        tex.unlity = lm.unlity;
        glGenTextures(1, &tex.id);
        createtexture(tex.id, tex.w, tex.h, NULL, 3, 1, tex.type&LM_ALPHA ? GL_RGBA : GL_RGB);
        if(renderpath!=R_FIXEDFUNCTION && (lm.type&LM_TYPE)==LM_BUMPMAP0 && lightmaps.inrange(surface.lmid+1-LMID_RESERVED))
        {
            LightMap &lm2 = lightmaps[surface.lmid+1-LMID_RESERVED];
            lm2.offsetx = lm2.offsety = 0;
            lm2.tex = lightmaptexs.length();
            LightMapTexture &tex2 = lightmaptexs.add();
            tex2.type = (lm.type&~LM_TYPE) | LM_BUMPMAP0;
            tex2.w = LM_PACKW;
            tex2.h = LM_PACKH;
            tex2.unlitx = lm2.unlitx;
            tex2.unlity = lm2.unlity;
            glGenTextures(1, &tex2.id);
            createtexture(tex2.id, tex2.w, tex2.h, NULL, 3, 1, GL_RGB);
        }
    }

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, LM_PACKW);

    glBindTexture(GL_TEXTURE_2D, lightmaptexs[lm.tex].id);
    glTexSubImage2D(GL_TEXTURE_2D, 0, lm.offsetx + surface.x, lm.offsety + surface.y, surface.w, surface.h, lm.type&LM_ALPHA ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, &lm.data[(surface.y*LM_PACKW + surface.x)*lm.bpp]);
    if(renderpath!=R_FIXEDFUNCTION && (lm.type&LM_TYPE)==LM_BUMPMAP0 && lightmaps.inrange(surface.lmid+1-LMID_RESERVED))
    {
        LightMap &lm2 = lightmaps[surface.lmid+1-LMID_RESERVED];
        glBindTexture(GL_TEXTURE_2D, lightmaptexs[lm2.tex].id);
        glTexSubImage2D(GL_TEXTURE_2D, 0, lm2.offsetx + surface.x, lm2.offsety + surface.y, surface.w, surface.h, GL_RGB, GL_UNSIGNED_BYTE, &lm2.data[(surface.y*LM_PACKW + surface.x)*3]);
    }
 
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
}
 
        
void generate_lumel(const float tolerance, const vector<const extentity *> &lights, const vec &target, const vec &normal, vec &sample, int x, int y)
{
    vec avgray(0, 0, 0);
    float r = 0, g = 0, b = 0;
    loopv(lights)
    {
        const extentity &light = *lights[i];
        vec ray = target;
        ray.sub(light.o);
        float mag = ray.magnitude();
        if(!mag) continue;
        float attenuation = 1;
        if(light.attr1)
        {
            attenuation -= mag / float(light.attr1);
            if(attenuation <= 0) continue;
        }
        ray.mul(1.0f / mag);
        float angle = -ray.dot(normal);
        if(angle <= 0) continue;
        if(light.attached && light.attached->type==ET_SPOTLIGHT)
        {
            vec spot(vec(light.attached->o).sub(light.o).normalize());
            float maxatten = 1-cosf(max(1, min(90, int(light.attached->attr1)))*RAD);
            float spotatten = 1-(1-ray.dot(spot))/maxatten;
            if(spotatten <= 0) continue;
            attenuation *= spotatten;
        }
        if(lmshadows && mag)
        {
            float dist = shadowray(light.o, ray, mag - tolerance, RAY_SHADOW | (lmshadows > 1 ? RAY_ALPHAPOLY : 0));
            if(dist < mag - tolerance) continue;
        }
        float intensity;
        switch(lmtype&LM_TYPE)
        {
            case LM_BUMPMAP0: 
                intensity = attenuation; 
                avgray.add(ray.mul(-attenuation));
                break;
            default:
                intensity = angle * attenuation;
                break;
        }
        r += intensity * float(light.attr2);
        g += intensity * float(light.attr3);
        b += intensity * float(light.attr4);
    }
    switch(lmtype&LM_TYPE)
    {
        case LM_BUMPMAP0:
            if(avgray.iszero()) break;
            // transform to tangent space
            extern float orientation_tangent[6][3][4];
            extern float orientation_binormal[6][3][4];            
            vec S(orientation_tangent[lmrotate][dimension(lmorient)]),
                T(orientation_binormal[lmrotate][dimension(lmorient)]);
            normal.orthonormalize(S, T);
            avgray.normalize();
            lm_ray[y*lm_w+x].add(vec(S.dot(avgray), T.dot(avgray), normal.dot(avgray)));
            break;
    }
    sample.x = min(255.0f, max(r, float(ambientcolor[0])));
    sample.y = min(255.0f, max(g, float(ambientcolor[1])));
    sample.z = min(255.0f, max(b, float(ambientcolor[2])));
}

bool lumel_sample(const vec &sample, int aasample, int stride)
{
    if(sample.x >= int(ambientcolor[0])+1 || sample.y >= int(ambientcolor[1])+1 || sample.z >= int(ambientcolor[2])+1) return true;
#define NCHECK(n) \
    if((n).x >= int(ambientcolor[0])+1 || (n).y >= int(ambientcolor[1])+1 || (n).z >= int(ambientcolor[2])+1) \
        return true;
    const vec *n = &sample - stride - aasample;
    NCHECK(n[0]); NCHECK(n[aasample]); NCHECK(n[2*aasample]);
    n += stride;
    NCHECK(n[0]); NCHECK(n[2*aasample]);
    n += stride;
    NCHECK(n[0]); NCHECK(n[aasample]); NCHECK(n[2*aasample]);
    return false;
}

void calcskylight(const vec &o, const vec &normal, float tolerance, uchar *skylight, int flags = RAY_ALPHAPOLY, extentity *t = NULL)
{
    static const vec rays[17] =
    {
        vec(cosf(21*RAD)*cosf(50*RAD), sinf(21*RAD)*cosf(50*RAD), sinf(50*RAD)),
        vec(cosf(111*RAD)*cosf(50*RAD), sinf(111*RAD)*cosf(50*RAD), sinf(50*RAD)),
        vec(cosf(201*RAD)*cosf(50*RAD), sinf(201*RAD)*cosf(50*RAD), sinf(50*RAD)),
        vec(cosf(291*RAD)*cosf(50*RAD), sinf(291*RAD)*cosf(50*RAD), sinf(50*RAD)),

        vec(cosf(66*RAD)*cosf(70*RAD), sinf(66*RAD)*cosf(70*RAD), sinf(70*RAD)),
        vec(cosf(156*RAD)*cosf(70*RAD), sinf(156*RAD)*cosf(70*RAD), sinf(70*RAD)),
        vec(cosf(246*RAD)*cosf(70*RAD), sinf(246*RAD)*cosf(70*RAD), sinf(70*RAD)),
        vec(cosf(336*RAD)*cosf(70*RAD), sinf(336*RAD)*cosf(70*RAD), sinf(70*RAD)),
       
        vec(0, 0, 1),

        vec(cosf(43*RAD)*cosf(60*RAD), sinf(43*RAD)*cosf(60*RAD), sinf(60*RAD)),
        vec(cosf(133*RAD)*cosf(60*RAD), sinf(133*RAD)*cosf(60*RAD), sinf(60*RAD)),
        vec(cosf(223*RAD)*cosf(60*RAD), sinf(223*RAD)*cosf(60*RAD), sinf(60*RAD)),
        vec(cosf(313*RAD)*cosf(60*RAD), sinf(313*RAD)*cosf(60*RAD), sinf(60*RAD)),

        vec(cosf(88*RAD)*cosf(80*RAD), sinf(88*RAD)*cosf(80*RAD), sinf(80*RAD)),
        vec(cosf(178*RAD)*cosf(80*RAD), sinf(178*RAD)*cosf(80*RAD), sinf(80*RAD)),
        vec(cosf(268*RAD)*cosf(80*RAD), sinf(268*RAD)*cosf(80*RAD), sinf(80*RAD)),
        vec(cosf(358*RAD)*cosf(80*RAD), sinf(358*RAD)*cosf(80*RAD), sinf(80*RAD)),

    };
    int hit = 0;
    loopi(17) if(normal.dot(rays[i])>=0)
    {
        if(shadowray(vec(rays[i]).mul(tolerance).add(o), rays[i], 1e16f, RAY_SHADOW | flags, t)>1e15f) hit++;
    }

    loopk(3) skylight[k] = uchar(ambientcolor[k] + (max(skylightcolor[k], ambientcolor[k]) - ambientcolor[k])*hit/17.0f);
}

static inline bool hasskylight()
{
    return skylightcolor[0]>ambientcolor[0] || skylightcolor[1]>ambientcolor[1] || skylightcolor[2]>ambientcolor[2];
}

VARR(blurlms, 0, 0, 2);
VARR(blurskylight, 0, 0, 2);

void blurlightmap(int n)
{
    static uchar blur[4*LM_MAXW*LM_MAXH];
    static const int matrix3x3[9] =
    {
        1, 2, 1,
        2, 4, 2,
        1, 2, 1
    };
    static const int matrix3x3sum = 16;
    static const int matrix5x5[25] =
    {
        1, 1, 2, 1, 1,
        1, 2, 4, 2, 1,
        2, 4, 8, 4, 2,
        1, 2, 4, 2, 1,
        1, 1, 2, 1, 1
    };
    static const int matrix5x5sum = 52;
    uchar *src = lm, *dst = blur;
    int stride = lmbpp*lm_w;
    loop(y, lm_h) loop(x, lm_w) 
    {
        loopk(3)
        {
            int c = *src, val = 0; 
            const int *m = n>1 ? matrix5x5 : matrix3x3;
            for(int t = -n; t<=n; t++) for(int s = -n; s<=n; s++, m++)
            {
                val += *m * (x+s>=0 && x+s<lm_w && y+t>=0 && y+t<lm_h ? src[t*stride+lmbpp*s] : c);
            }
            *dst++ = val/(n>1 ? matrix5x5sum : matrix3x3sum);
            src++;
        }
        if(lmtype&LM_ALPHA) *dst++ = *src++;
    }
    memcpy(lm, blur, lmbpp*lm_w*lm_h);
}

static inline void generate_alpha(float tolerance, const vec &pos, uchar &alpha)
{
    alpha = lookupblendmap(pos);
    if(lmslot->layermask)
    {
        static const int sdim[] = { 1, 0, 0 }, tdim[] = { 2, 2, 1 };
        int dim = dimension(lmorient);
        float k = 8.0f/lmslot->scale,
              s = (pos[sdim[dim]] * k - lmslot->xoffset) / lmslot->layermaskscale,
              t = (pos[tdim[dim]] * (dim <= 1 ? -k : k) - lmslot->yoffset) / lmslot->layermaskscale;
        if((lmrotate&5)==1) swap(s, t);
        if(lmrotate>=2 && lmrotate<=4) s = -s;
        if((lmrotate>=1 && lmrotate<=2) || lmrotate==5) t = -t;
        const ImageData &mask = *lmslot->layermask;
        int mx = int(floor(s))%mask.w, my = int(floor(t))%mask.h;
        if(mx < 0) mx += mask.w;
        if(my < 0) my += mask.h;
        uchar maskval = mask.data[mask.bpp*(mx + 1) - 1 + mask.pitch*my];
        switch(lmslot->layermaskmode)
        {
            case 2: alpha = min(alpha, maskval); break;
            case 3: alpha = max(alpha, maskval); break;
            case 4: alpha = min(alpha, uchar(0xFF - maskval)); break;
            case 5: alpha = max(alpha, uchar(0xFF - maskval)); break;
            default: alpha = maskval; break;
        }
    }
}
        
VAR(edgetolerance, 1, 4, 8);
VAR(adaptivesample, 0, 1, 1);

enum
{
    NO_SURFACE = 0,
    SURFACE_AMBIENT_BOTTOM,
    SURFACE_AMBIENT_TOP,
    SURFACE_LIGHTMAP_BOTTOM,
    SURFACE_LIGHTMAP_TOP,
    SURFACE_LIGHTMAP_BLEND 
};

#define SURFACE_AMBIENT SURFACE_AMBIENT_BOTTOM
#define SURFACE_LIGHTMAP SURFACE_LIGHTMAP_BOTTOM

int generate_lightmap(float lpu, int y1, int y2, const vec &origin, const lerpvert *lv, int numv, const vec &ustep, const vec &vstep)
{
    static uchar mincolor[4], maxcolor[4];
    static float aacoords[8][2] =
    {
        {0.0f, 0.0f},
        {-0.5f, -0.5f},
        {0.0f, -0.5f},
        {-0.5f, 0.0f},

        {0.3f, -0.6f},
        {0.6f, 0.3f},
        {-0.3f, 0.6f},
        {-0.6f, -0.3f},
    };
    float tolerance = 0.5 / lpu;
    vector<const extentity *> &lights = (y1 == 0 ? lights1 : lights2);
    vec v = origin;
    vec offsets[8];
    loopi(8) loopj(3) offsets[i][j] = aacoords[i][0]*ustep[j] + aacoords[i][1]*vstep[j];

    if(y1 == 0)
    {
        memset(mincolor, 255, sizeof(mincolor));
        memset(maxcolor, 0, sizeof(maxcolor));
        if((lmtype&LM_TYPE) == LM_BUMPMAP0) memset(lm_ray, 0, sizeof(lm_ray));
    }

    static vec samples[4*(LM_MAXW+1)*(LM_MAXH+1)];

    int aasample = min(1 << lmaa, 4);
    int stride = aasample*(lm_w+1);
    vec *sample = &samples[stride*y1];
    uchar *skylight = &lm[lmbpp*lm_w*y1];
    lerpbounds start, end;
    initlerpbounds(lv, numv, start, end);
    for(int y = y1; y < y2; ++y, v.add(vstep)) 
    {
        vec normal, nstep;
        lerpnormal(y, lv, numv, start, end, normal, nstep);
        
        vec u(v);
        for(int x = 0; x < lm_w; ++x, u.add(ustep), normal.add(nstep), skylight += lmbpp) 
        {
            CHECK_PROGRESS(return NO_SURFACE);
            generate_lumel(tolerance, lights, u, vec(normal).normalize(), *sample, x, y);
            if(hasskylight())
            {
                if((lmtype&LM_TYPE)==LM_BUMPMAP0 || !adaptivesample || sample->x<skylightcolor[0] || sample->y<skylightcolor[1] || sample->z<skylightcolor[2])
                    calcskylight(u, normal, tolerance, skylight, lmshadows > 1 ? RAY_ALPHAPOLY : 0);
                else loopk(3) skylight[k] = max(skylightcolor[k], ambientcolor[k]);
            }
            else loopk(3) skylight[k] = ambientcolor[k];
            if(lmtype&LM_ALPHA) generate_alpha(tolerance, u, skylight[3]);
            sample += aasample;
        }
        sample += aasample;
    }
    v = origin;
    sample = &samples[stride*y1];
    initlerpbounds(lv, numv, start, end);
    for(int y = y1; y < y2; ++y, v.add(vstep)) 
    {
        vec normal, nstep;
        lerpnormal(y, lv, numv, start, end, normal, nstep);

        vec u(v);
        for(int x = 0; x < lm_w; ++x, u.add(ustep), normal.add(nstep)) 
        {
            vec &center = *sample++;
            if(adaptivesample && x > 0 && x+1 < lm_w && y > y1 && y+1 < y2 && !lumel_sample(center, aasample, stride))
                loopi(aasample-1) *sample++ = center;
            else
            {
#define EDGE_TOLERANCE(i) \
    ((!x && aacoords[i][0] < 0) \
     || (x+1==lm_w && aacoords[i][0] > 0) \
     || (!y && aacoords[i][1] < 0) \
     || (y+1==lm_h && aacoords[i][1] > 0) \
     ? edgetolerance : 1)
                vec n(normal);
                n.normalize();
                loopi(aasample-1)
                    generate_lumel(EDGE_TOLERANCE(i+1) * tolerance, lights, vec(u).add(offsets[i+1]), n, *sample++, x, y);
                if(lmaa == 3) 
                {
                    loopi(4)
                    {
                        vec s;
                        generate_lumel(EDGE_TOLERANCE(i+4) * tolerance, lights, vec(u).add(offsets[i+4]), n, s, x, y);
                        center.add(s);
                    }
                    center.div(5);
                }
            }
        }
        if(aasample > 1)
        {
            normal.normalize();
            generate_lumel(tolerance, lights, vec(u).add(offsets[1]), normal, sample[1], lm_w-1, y);
            if(aasample > 2)
                generate_lumel(edgetolerance * tolerance, lights, vec(u).add(offsets[3]), normal, sample[3], lm_w-1, y);
        }
        sample += aasample;
    }

    if(y2 == lm_h)
    {
        if(aasample > 1)
        {
            vec normal, nstep;
            lerpnormal(lm_h, lv, numv, start, end, normal, nstep);

            for(int x = 0; x <= lm_w; ++x, v.add(ustep), normal.add(nstep))
            {
                CHECK_PROGRESS(return NO_SURFACE);
                vec n(normal);
                n.normalize();
                generate_lumel(edgetolerance * tolerance, lights, vec(v).add(offsets[1]), n, sample[1], min(x, lm_w-1), lm_h-1);
                if(aasample > 2)
                    generate_lumel(edgetolerance * tolerance, lights, vec(v).add(offsets[2]), n, sample[2], min(x, lm_w-1), lm_h-1);
                sample += aasample;
            } 
        }

        if(hasskylight())
        {
            if(blurskylight && (lm_w>1 || lm_h>1)) blurlightmap(blurskylight);
        }
        sample = samples;
        float weight = 1.0f / (1.0f + 4.0f*lmaa),
              cweight = weight * (lmaa == 3 ? 5.0f : 1.0f);
        uchar *lumel = lm;
        vec *ray = lm_ray;
        bvec minray(255, 255, 255), maxray(0, 0, 0);
        loop(y, lm_h)
        {
            loop(x, lm_w)
            {
                vec l(0, 0, 0);
                const vec &center = *sample++;
                loopi(aasample-1) l.add(*sample++);
                if(aasample > 1)
                {
                    l.add(sample[1]);
                    if(aasample > 2) l.add(sample[3]);
                }
                vec *next = sample + stride - aasample;
                if(aasample > 1)
                {
                    l.add(next[1]);
                    if(aasample > 2) l.add(next[2]);
                    l.add(next[aasample+1]);
                }

                int r = int(center.x*cweight + l.x*weight),
                    g = int(center.y*cweight + l.y*weight),
                    b = int(center.z*cweight + l.z*weight),
                    ar = lumel[0], ag = lumel[1], ab = lumel[2];
                lumel[0] = max(ar, r);
                lumel[1] = max(ag, g);
                lumel[2] = max(ab, b);
                loopk(3)
                {
                    mincolor[k] = min(mincolor[k], lumel[k]);
                    maxcolor[k] = max(maxcolor[k], lumel[k]);
                }
                if(lmtype&LM_ALPHA)
                {
                    mincolor[3] = min(mincolor[3], lumel[3]);
                    maxcolor[3] = max(maxcolor[3], lumel[3]);
                }
                if((lmtype&LM_TYPE) == LM_BUMPMAP0)
                {
                    bvec &n = ((bvec *)lm_ray)[ray-lm_ray];
                    if(ray->iszero()) n = bvec(128, 128, 255);
                    else
                    {
                        // bias the normals towards the amount of ambient/skylight in the lumel 
                        // this is necessary to prevent the light values in shaders from dropping too far below the skylight (to the ambient) if N.L is small 
                        ray->normalize();
                        int l = max(r, max(g, b)), a = max(ar, max(ag, ab));
                        ray->mul(max(l-a, 0));
                        ray->z += a;
                        n = bvec(ray->normalize());
                    }
                    loopk(3)
                    {
                        minray[k] = min(minray[k], n[k]);
                        maxray[k] = max(maxray[k], n[k]);
                    }
                    ray++;
                }
                lumel += lmbpp;
            }
            sample += aasample;
        }
        if(int(maxcolor[0]) - int(mincolor[0]) <= lighterror &&
           int(maxcolor[1]) - int(mincolor[1]) <= lighterror &&
           int(maxcolor[2]) - int(mincolor[2]) <= lighterror &&
           mincolor[3] >= maxcolor[3])
        {
            uchar color[3];
            loopk(3) color[k] = (int(maxcolor[k]) + int(mincolor[k])) / 2;
            if(color[0] <= int(ambientcolor[0]) + lighterror && 
               color[1] <= int(ambientcolor[1]) + lighterror && 
               color[2] <= int(ambientcolor[2]) + lighterror &&
               (maxcolor[3]==0 || mincolor[3]==255))
                return mincolor[3]==255 ? SURFACE_AMBIENT_TOP : SURFACE_AMBIENT_BOTTOM;
            if((lmtype&LM_TYPE) != LM_BUMPMAP0 || 
                (int(maxray.x) - int(minray.x) <= bumperror &&
                 int(maxray.y) - int(minray.z) <= bumperror &&
                 int(maxray.z) - int(minray.z) <= bumperror))

            {
                memcpy(lm, color, 3);
                if(lmtype&LM_ALPHA) lm[3] = mincolor[3];
                if((lmtype&LM_TYPE) == LM_BUMPMAP0) 
                {
                    loopk(3) ((bvec *)lm_ray)[0][k] = uchar((int(maxray[k])+int(minray[k]))/2);
                }
                lm_w = 1;
                lm_h = 1;
            }
        }
        if(blurlms && (lm_w>1 || lm_h>1)) blurlightmap(blurlms);
    }
    if(mincolor[3]==255) return SURFACE_LIGHTMAP_TOP;
    else if(maxcolor[3]==0) return SURFACE_LIGHTMAP_BOTTOM;
    else return SURFACE_LIGHTMAP_BLEND;
}

int preview_lightmap_alpha(float lpu, int y1, int y2, const vec &origin, const vec &ustep, const vec &vstep)
{
    extern int fullbrightlevel;
    float tolerance = 0.5 / lpu;
    uchar *dst = &lm[4*lm_w*y1];
    vec v = origin;
    uchar minalpha = 255, maxalpha = 0;
    for(int y = y1; y < y2; ++y, v.add(vstep))
    {
        vec u(v);
        for(int x = 0; x < lm_w; ++x, u.add(ustep), dst += 4)
        {
            loopk(3) dst[k] = fullbrightlevel;        
            generate_alpha(tolerance, u, dst[3]);
            minalpha = min(minalpha, dst[3]);
            maxalpha = max(maxalpha, dst[3]);
        }
    }
    if(y2 == lm_h)
    {
        if(minalpha==255) return SURFACE_AMBIENT_TOP;
        if(maxalpha==0) return SURFACE_AMBIENT_BOTTOM;
        if(minalpha==maxalpha) lm_w = lm_h = 1;    
        loopi(lm_w*lm_h) ((bvec *)lm_ray)[i] = bvec(128, 128, 255);
    }
    return SURFACE_LIGHTMAP_BLEND;
}        

void clear_lmids(cube *c)
{
    loopi(8)
    {
        if(c[i].ext)
        {
            if(c[i].ext->surfaces) freesurfaces(c[i]);
            if(c[i].ext->normals) freenormals(c[i]);
        }
        if(c[i].children) clear_lmids(c[i].children);
    }
}

#define LIGHTCACHESIZE 1024

static struct lightcacheentry
{
    int x, y;
    vector<int> lights;
} lightcache[1024];

#define LIGHTCACHEHASH(x, y) (((((x)^(y))<<5) + (((x)^(y))>>5)) & (LIGHTCACHESIZE - 1))

VARF(lightcachesize, 4, 6, 12, clearlightcache());

void clearlightcache(int e)
{
    if(e < 0 || !entities::getents()[e]->attr1)
    {
        for(lightcacheentry *lce = lightcache; lce < &lightcache[LIGHTCACHESIZE]; lce++)
        {
            lce->x = -1;
            lce->lights.setsize(0);
        }
    }
    else
    {
        const extentity &light = *entities::getents()[e];
        int radius = light.attr1;
        for(int x = int(max(light.o.x-radius, 0.0f))>>lightcachesize, ex = int(min(light.o.x+radius, worldsize-1.0f))>>lightcachesize; x <= ex; x++)
        for(int y = int(max(light.o.y-radius, 0.0f))>>lightcachesize, ey = int(min(light.o.y+radius, worldsize-1.0f))>>lightcachesize; y <= ey; y++)
        {
            lightcacheentry &lce = lightcache[LIGHTCACHEHASH(x, y)];
            if(lce.x != x || lce.y != y) continue;
            lce.x = -1;
            lce.lights.setsize(0);
        }
    }
}

const vector<int> &checklightcache(int x, int y)
{
    x >>= lightcachesize;
    y >>= lightcachesize; 
    lightcacheentry &lce = lightcache[LIGHTCACHEHASH(x, y)];
    if(lce.x == x && lce.y == y) return lce.lights;

    lce.lights.setsize(0);
    int csize = 1<<lightcachesize, cx = x<<lightcachesize, cy = y<<lightcachesize;
    const vector<extentity *> &ents = entities::getents();
    loopv(ents)
    {
        const extentity &light = *ents[i];
        switch(light.type)
        {
            case ET_LIGHT:
            {
                int radius = light.attr1;
                if(radius > 0)
                {
                    if(light.o.x + radius < cx || light.o.x - radius > cx + csize ||
                       light.o.y + radius < cy || light.o.y - radius > cy + csize)
                        continue;
                }
                break;
            }
            default: continue;
        }
        lce.lights.add(i);
    }

    lce.x = x;
    lce.y = y;
    return lce.lights;
}

static inline void addlight(const extentity &light, int cx, int cy, int cz, int size, const vec *v, const vec *n, const vec *n2)
{
    int radius = light.attr1;
    if(radius > 0)
    {
        if(light.o.x + radius < cx || light.o.x - radius > cx + size ||
           light.o.y + radius < cy || light.o.y - radius > cy + size ||
           light.o.z + radius < cz || light.o.z - radius > cz + size)
            return;
    }

    if(!n2)
    {
        loopi(4)
        {
            vec p(light.o);
            p.sub(v[i]);
            float dist = p.dot(n[i]);
            if(dist >= 0 && (!radius || dist < radius))
            {
                lights1.add(&light);
                return;
            }
        }
        return;
    }

    bool plane1 = false, plane2 = false;
    loopi(4)
    {
        vec p(light.o);
        p.sub(v[i]);
        if(i != 3)
        {
            float dist = p.dot(n[i]);
            if(dist >= 0 && (!radius || dist < radius)) 
            {
                plane1 = true;
                if(plane2) break;
            }
        }
        if(i != 1)
        {
            float dist = p.dot(n2[i > 0 ? i-1 : 0]);
            if(dist >= 0 && (!radius || dist < radius)) 
            {
                plane2 = true;
                if(plane1) break;
            }
        }
    }

    if(plane1) lights1.add(&light);
    if(plane2) lights2.add(&light);
} 

bool find_lights(int cx, int cy, int cz, int size, const vec *v, const vec *n, const vec *n2, const Slot &slot)
{
    lights1.setsize(0);
    lights2.setsize(0);
    const vector<extentity *> &ents = entities::getents();
    if(size <= 1<<lightcachesize)
    {
        const vector<int> &lights = checklightcache(cx, cy);
        loopv(lights)
        {
            const extentity &light = *ents[lights[i]];
            switch(light.type)
            {
                case ET_LIGHT: addlight(light, cx, cy, cz, size, v, n, n2); break;
            }
        }
    }
    else loopv(ents)
    {
        const extentity &light = *ents[i];
        switch(light.type)
        {
            case ET_LIGHT: addlight(light, cx, cy, cz, size, v, n, n2); break;
        }
    }
    if(slot.layer && (setblendmaporigin(ivec(cx, cy, cz), size) || slot.layermask)) return true;
    return lights1.length() || lights2.length() || hasskylight();
}

int setup_surface(plane planes[2], const vec *p, const vec *n, const vec *n2, uchar texcoords[8], bool preview = false)
{
    vec u, v, s, t;
    float umin(0.0f), umax(0.0f),
          vmin(0.0f), vmax(0.0f),
          tmin(0.0f), tmax(0.0f);

    #define COORDMINMAX(u, v, orig, vert) \
    { \
        vec tovert = p[vert]; \
        tovert.sub(p[orig]); \
        float u ## coord = u.dot(tovert), \
              v ## coord = v.dot(tovert); \
        u ## min = min(u ## coord, u ## min); \
        u ## max = max(u ## coord, u ## max); \
        v ## min = min(v ## coord, v ## min); \
        v ## max = max(v ## coord, v ## max);  \
    }

    if(!n2)
    {
        u = (p[0] == p[1] ? p[2] : p[1]);
        u.sub(p[0]);
        u.normalize();
        v.cross(planes[0], u);

        COORDMINMAX(u, v, 0, 1);
        COORDMINMAX(u, v, 0, 2);
        COORDMINMAX(u, v, 0, 3);
    }
    else
    {
        u = p[2];
        u.sub(p[0]);
        u.normalize();
        v.cross(u, planes[0]);
        t.cross(planes[1], u);

        COORDMINMAX(u, v, 0, 1);
        COORDMINMAX(u, v, 0, 2);
        COORDMINMAX(u, t, 0, 2);
        COORDMINMAX(u, t, 0, 3);
    }

    int scale = int(min(umax - umin, vmax - vmin));
    if(n2) scale = min(scale, int(tmax));
    float lpu = 16.0f / float(scale < (1 << lightlod) ? lightprecision / 2 : lightprecision);
    int ul((int)ceil((umax - umin + 1) * lpu)),
        vl((int)ceil((vmax - vmin + 1) * lpu)),
        tl(0);
    vl = max(LM_MINW, vl);
    if(n2)
    {
        tl = (uint)ceil((tmax + 1) * lpu);
        tl = max(LM_MINW, tl);
    }
    lm_w = max(LM_MINW, min(LM_MAXW, ul));
    lm_h = min(LM_MAXH, vl + tl);

    vec origin1(p[0]), origin2, uo(u), vo(v);
    uo.mul(umin);
    if(!n2)
    {
        vo.mul(vmin);
    }
    else
    {
        vo.mul(vmax);
        v.mul(-1);
    }
    origin1.add(uo);
    origin1.add(vo);
    
    vec ustep(u), vstep(v);
    ustep.mul((umax - umin) / (lm_w - 1));
    uint split = vl * lm_h / (vl + tl);
    vstep.mul((vmax - vmin) / (split - 1));
    int surftype = NO_SURFACE;
    if(preview)
    {
        if(!n2) surftype = preview_lightmap_alpha(lpu, 0, lm_h, origin1, ustep, vstep);
        else
        {
            origin2 = p[0];
            origin2.add(uo);
            vec tstep(t);
            tstep.mul(tmax / (lm_h - split - 1));

            surftype = preview_lightmap_alpha(lpu, 0, split, origin1, ustep, vstep);
            if(surftype<SURFACE_LIGHTMAP) return surftype;
            surftype = preview_lightmap_alpha(lpu, split, lm_h, origin2, ustep, tstep);
        }
    }
    else if(!n2)
    {
        lerpvert lv[4];
        int numv = 4;
        calclerpverts(origin1, p, n, ustep, vstep, lv, numv);

        surftype = generate_lightmap(lpu, 0, lm_h, origin1, lv, numv, ustep, vstep);
    }
    else
    {
        origin2 = p[0];
        origin2.add(uo);
        vec tstep(t);
        tstep.mul(tmax / (lm_h - split - 1));

        vec p1[3] = {p[0], p[1], p[2]},
            p2[3] = {p[0], p[2], p[3]};
        lerpvert lv1[3], lv2[3];
        int numv1 = 3, numv2 = 3;
        calclerpverts(origin1, p1, n, ustep, vstep, lv1, numv1);
        calclerpverts(origin2, p2, n2, ustep, tstep, lv2, numv2);

        surftype = generate_lightmap(lpu, 0, split, origin1, lv1, numv1, ustep, vstep);
        if(surftype<SURFACE_LIGHTMAP) return surftype;
        surftype = generate_lightmap(lpu, split, lm_h, origin2, lv2, numv2, ustep, tstep);
    }
    if(surftype<SURFACE_LIGHTMAP) return surftype;

    #define CALCVERT(origin, u, v, offset, vert) \
    { \
        vec tovert = p[vert]; \
        tovert.sub(origin); \
        float u ## coord = u.dot(tovert), \
              v ## coord = v.dot(tovert); \
        texcoords[vert*2] = uchar(u ## coord * u ## scale); \
        texcoords[vert*2+1] = offset + uchar(v ## coord * v ## scale); \
    }

    float uscale = 255.0f / float(umax - umin),
          vscale = 255.0f / float(vmax - vmin) * float(split) / float(lm_h);
    CALCVERT(origin1, u, v, 0, 0)
    CALCVERT(origin1, u, v, 0, 1)
    CALCVERT(origin1, u, v, 0, 2)
    if(!n2)
    {
        CALCVERT(origin1, u, v, 0, 3)
    }
    else
    {
        uchar toffset = uchar(255.0 * float(split) / float(lm_h));
        float tscale = 255.0f / float(tmax - tmin) * float(lm_h - split) / float(lm_h);
        CALCVERT(origin2, u, t, toffset, 3)
    }
    return surftype;
}

void removelmalpha()
{
    if(!(lmtype&LM_ALPHA)) return;
    for(uchar *dst = lm, *src = lm, *end = &src[lm_w*lm_h*4];
        src < end;
        dst += 3, src += 4)
    {
        dst[0] = src[0];
        dst[1] = src[1];
        dst[2] = src[2];
    }
    lmtype &= ~LM_ALPHA;
    lmbpp = 3;
}

void setup_surfaces(cube &c, int cx, int cy, int cz, int size)
{
    if(c.ext && c.ext->surfaces)
    {
        loopi(6) if(c.ext->surfaces[i].lmid >= LMID_RESERVED)
        {
            return;
        }
        freesurfaces(c);
        freenormals(c);
    }
    vvec vvecs[8];
    vec verts[8];
    int vertused = 0, usefaces[6];
    loopi(6) if((usefaces[i] = visibletris(c, i, cx, cy, cz, size))) vertused |= fvmasks[1<<i];
    loopi(8) if(vertused&(1<<i)) 
    {
        calcvert(c, cx, cy, cz, size, vvecs[i], i);
        verts[i] = vvecs[i].tovec(cx, cy, cz);
    }

    int mergeindex = 0;
    surfaceinfo surfaces[12];
    int numsurfs = 0;
    loopi(6) if(usefaces[i])
    {
        CHECK_PROGRESS(return);
        if(c.texture[i] == DEFAULT_SKY) continue;

        plane planes[2];
        vec v[4], n[4], n2[3];
        int numplanes;

        Slot &slot = lookuptexture(c.texture[i], false),
             *layer = slot.layer ? &lookuptexture(slot.layer, false) : NULL;
        Shader *shader = slot.shader;
        int shadertype = shader->type;
        if(layer) shadertype |= layer->shader->type;
        if(c.ext && c.ext->merged&(1<<i))
        {
            if(!(c.ext->mergeorigin&(1<<i))) continue;
            const mergeinfo &m = c.ext->merges[mergeindex++];
            vvec mv[4];
            ivec mo(cx, cy, cz);
            genmergedverts(c, i, mo, size, m, mv, planes);

            numplanes = 1;
            int msz = calcmergedsize(i, mo, size, m, mv);
            mo.mask(~((1<<msz)-1));

            loopj(4)
            {
                v[j] = mv[j].tovec(mo);
                findnormal(mo, mv[j], planes[0], n[j]);
            }

            if(!find_lights(mo.x, mo.y, mo.z, 1<<msz, v, n, NULL, slot))
            {
                if(!(shadertype&(SHADER_NORMALSLMS | SHADER_ENVMAP))) continue;
            }
        }
        else
        {
            numplanes = genclipplane(c, i, verts, planes);
            if(!numplanes) continue;

            vec avg;
            if(numplanes >= 2)
            {
                if(!(usefaces[i]&1)) { planes[0] = planes[1]; numplanes--; }
                else if(!(usefaces[i]&2)) numplanes--;
                else
                {
                    avg = planes[0];
                    avg.add(planes[1]);
                    avg.normalize();
                }
            }

            int order = usefaces[i]&4 || faceconvexity(c, i)<0 ? 1 : 0;
            loopj(4)
            {
                int index = fv[i][(j+order)&3];
                const vvec &vv = vvecs[index];
                v[j] = verts[index];
                if(numplanes < 2 || j == 1) findnormal(ivec(cx, cy, cz), vv, planes[0], n[j]);
                else if(j==3) findnormal(ivec(cx, cy, cz), vv, planes[1], n2[2]);
                else
                {
                    findnormal(ivec(cx, cy, cz), vv, avg, n[j]);
                    if(j) n2[j-1] = n[j];
                    else n2[0] = n[0];
                }
            }

            if(!(usefaces[i]&1)) { v[1] = v[0]; n[1] = n[0]; }
            if(!(usefaces[i]&2)) { v[3] = v[0]; n[3] = n[0]; }

            if(!find_lights(cx, cy, cz, size, v, n, numplanes > 1 ? n2 : NULL, slot))
            {
                if(!(shadertype&(SHADER_NORMALSLMS | SHADER_ENVMAP))) continue;
            }
        }
        if(shadertype&(SHADER_NORMALSLMS | SHADER_ENVMAP))
        {
            newnormals(c);
            surfacenormals *cn = c.ext->normals;
            cn[i].normals[0] = bvec(n[0]);
            cn[i].normals[1] = bvec(n[1]);
            cn[i].normals[2] = bvec(n[2]);
            cn[i].normals[3] = bvec(numplanes < 2 ? n[3] : n2[2]);
        }
        if(lights1.empty() && lights2.empty() && (!layer || (!hasblendmap() && !slot.layermask)) && !hasskylight()) continue;

        uchar texcoords[8];

        lmslot = &slot;
        lmtype = shader->type&SHADER_NORMALSLMS ? LM_BUMPMAP0 : LM_DIFFUSE;
        if(layer) lmtype |= LM_ALPHA;
        lmbpp = lmtype&LM_ALPHA ? 4 : 3;
        lmorient = i;
        lmrotate = slot.rotation;
        int surftype = setup_surface(planes, v, n, numplanes >= 2 ? n2 : NULL, texcoords);
        switch(surftype)
        {
            case SURFACE_LIGHTMAP_BOTTOM:
                if((shader->type^layer->shader->type)&SHADER_NORMALSLMS ||
                   (shader->type&SHADER_NORMALSLMS && slot.rotation!=layer->rotation))
                    break;
                // fall through
            case SURFACE_LIGHTMAP_BLEND:
            case SURFACE_LIGHTMAP_TOP:
            {
                CHECK_PROGRESS(return);
                if(!numsurfs) { numsurfs = 6; memset(surfaces, 0, sizeof(surfaces)); }
                surfaceinfo &surface = surfaces[i];
                surface.w = lm_w;
                surface.h = lm_h;
                if(surftype==SURFACE_LIGHTMAP_BLEND) surface.layer = LAYER_TOP|LAYER_BLEND;
                else
                {
                    if(surftype==SURFACE_LIGHTMAP_BOTTOM) surface.layer = LAYER_BOTTOM;
                    if(lmtype&LM_ALPHA) removelmalpha();
                } 
                memcpy(surface.texcoords, texcoords, 8);
                pack_lightmap(surface);
                if(surftype!=SURFACE_LIGHTMAP_BLEND) continue;
                if((shader->type^layer->shader->type)&SHADER_NORMALSLMS ||
                   (shader->type&SHADER_NORMALSLMS && slot.rotation!=layer->rotation)) 
                    break;
                surfaces[numsurfs] = surface;
                surfaces[numsurfs++].layer = LAYER_BOTTOM;
                continue;
            }

            case SURFACE_AMBIENT_BOTTOM:
                if(layer)
                {
                    if(!numsurfs) { numsurfs = 6; memset(surfaces, 0, sizeof(surfaces)); }
                    surfaces[i].layer = LAYER_BOTTOM;
                }
                continue;

            default: continue;
        }

        lmslot = layer;
        lmtype = layer->shader->type&SHADER_NORMALSLMS ? LM_BUMPMAP0 : LM_DIFFUSE;
        lmbpp = 3;
        lmrotate = layer->rotation;
        switch(setup_surface(planes, v, n, numplanes >= 2 ? n2 : NULL, texcoords))
        {
            case SURFACE_LIGHTMAP_TOP:
            {
                CHECK_PROGRESS(return);
                if(!numsurfs) { numsurfs = 6; memset(surfaces, 0, sizeof(surfaces)); }
                surfaceinfo &surface = surfaces[surftype==SURFACE_LIGHTMAP_BLEND ? numsurfs++ : i];
                surface.w = lm_w;
                surface.h = lm_h;
                surface.layer = LAYER_BOTTOM;
                memcpy(surface.texcoords, texcoords, 8);
                pack_lightmap(surface);
                break;
            }

            case SURFACE_AMBIENT_TOP:
            {
                if(!numsurfs) { numsurfs = 6; memset(surfaces, 0, sizeof(surfaces)); }
                surfaceinfo &surface = surfaces[surftype==SURFACE_LIGHTMAP_BLEND ? numsurfs++ : i];
                memset(&surface, 0, sizeof(surface));
                surface.layer = LAYER_BOTTOM;
                break;
            }
        }
    }
    if(numsurfs) newsurfaces(c, surfaces, numsurfs);
}

void generate_lightmaps(cube *c, int cx, int cy, int cz, int size)
{
    CHECK_PROGRESS(return);

    progress++;

    loopi(8)
    {
        ivec o(i, cx, cy, cz, size);
        if(c[i].children)
            generate_lightmaps(c[i].children, o.x, o.y, o.z, size >> 1);
        if(!c[i].children && !isempty(c[i]))
            setup_surfaces(c[i], o.x, o.y, o.z, size);
    } 
}

bool previewblends(cube &c, const ivec &co, int size)
{
    if(isempty(c)) return false;

    int usefaces[6];
    int vertused = 0;
    loopi(6) if((usefaces[i] = lookuptexture(c.texture[i], false).layer ? visibletris(c, i, co.x, co.y, co.z, size) : 0))
        vertused |= fvmasks[1<<i];
    if(!vertused) return false;

    if(!setblendmaporigin(co, size))
    {
        if(!c.ext || !c.ext->surfaces || c.ext->surfaces==brightsurfaces) return false;
        bool blends = false;
        loopi(6) if(c.ext->surfaces[i].layer&LAYER_BLEND || c.ext->surfaces[i].layer==LAYER_BOTTOM)
        {
            surfaceinfo &surface = c.ext->surfaces[i];
            memset(&surface, 0, sizeof(surfaceinfo));
            surface.lmid = LMID_BRIGHT;
            surface.layer = LAYER_TOP;
            blends = true;
        }
        return blends;
    }

    vec verts[8];
    loopi(8) if(vertused&(1<<i)) 
    {
        vvec vv;
        calcvert(c, co.x, co.y, co.z, size, vv, i);
        verts[i] = vv.tovec(co);
    }

    surfaceinfo surfaces[12], *srcsurfaces = c.ext && c.ext->surfaces && c.ext->surfaces!=brightsurfaces ? c.ext->surfaces : NULL;
    int numsurfs = srcsurfaces ? 6 : 0, numsrcsurfs = srcsurfaces ? 6 : 0;
    if(srcsurfaces) memcpy(surfaces, srcsurfaces, 6*sizeof(surfaceinfo));
    else 
    {
        memset(surfaces, 0, 6*sizeof(surfaceinfo));
        loopi(6) surfaces[i].lmid = LMID_BRIGHT;
    }
    loopi(6)
    {
        if(surfaces[i].layer&LAYER_BLEND) 
        {
            if(!usefaces[i]) 
            {
                surfaces[numsurfs++] = srcsurfaces[numsrcsurfs++];
                continue;
            }
            numsrcsurfs++;
        }
        else if(!usefaces[i]) continue;

        plane planes[2];
        int numplanes = genclipplane(c, i, verts, planes);
        if(!numplanes) continue;

        Slot &slot = lookuptexture(c.texture[i], false),
             &layer = lookuptexture(slot.layer, false);
        Shader *shader = slot.shader;
        int shadertype = shader->type | layer.shader->type;
            
        int order = usefaces[i]&4 || faceconvexity(c, i)<0 ? 1 : 0;
        vec v[4];
        loopk(4) v[k] = verts[fv[i][(k+order)&3]];
        if(!(usefaces[i]&1)) { v[1] = v[0]; if(numplanes>1) { planes[0] = planes[1]; --numplanes; } }
        if(!(usefaces[i]&2)) { v[3] = v[0]; if(numplanes>1) --numplanes; }

        static const vec n[4] = { vec(0, 0, 1), vec(0, 0, 1), vec(0, 0, 1), vec(0, 0, 1) };
        uchar texcoords[8];

        lmslot = &slot;
        lmtype = shadertype&SHADER_NORMALSLMS ? LM_BUMPMAP0|LM_ALPHA : LM_DIFFUSE|LM_ALPHA;
        lmbpp = 4;
        lmorient = i;
        lmrotate = slot.rotation;
        int surftype = setup_surface(planes, v, n, numplanes >= 2 ? n : NULL, texcoords, true);
        switch(surftype)
        {
            case SURFACE_AMBIENT_TOP:
                if(srcsurfaces) 
                {
                    memset(&surfaces[i], 0, sizeof(surfaceinfo));
                    surfaces[i].lmid = LMID_BRIGHT;
                }
                continue;

            case SURFACE_AMBIENT_BOTTOM:
                if(!numsurfs) numsurfs = 6;
                if(srcsurfaces) 
                {
                    memset(&surfaces[i], 0, sizeof(surfaceinfo));
                    surfaces[i].lmid = LMID_BRIGHT;
                }
                surfaces[i].layer = LAYER_BOTTOM;
                continue;

            case SURFACE_LIGHTMAP_BLEND:
            {
                if(!numsurfs) numsurfs = 6;
                surfaceinfo &surface = surfaces[i];
                if(surface.w==lm_w && surface.h==lm_h && 
                   surface.layer==(LAYER_TOP|LAYER_BLEND) && 
                   !memcmp(surface.texcoords, texcoords, 8) &&
                   lightmaps.inrange(surface.lmid-LMID_RESERVED) &&
                   lightmaps[surface.lmid-LMID_RESERVED].type==lmtype)           
                {
                    copy_lightmap(surface);
                    update_lightmap(surface);
                    surfaces[numsurfs] = surface;
                    surfaces[numsurfs++].layer = LAYER_BOTTOM;
                    continue;
                }
                surface.w = lm_w;
                surface.h = lm_h;
                surface.layer = LAYER_TOP|LAYER_BLEND;
                memcpy(surface.texcoords, texcoords, 8);
                if(pack_lightmap(surface)) update_lightmap(surface);
                surfaces[numsurfs] = surface;
                surfaces[numsurfs++].layer = LAYER_BOTTOM;
                continue;
            }
        }
    }
    if(numsurfs>numsrcsurfs) 
    {
        freesurfaces(c);
        newsurfaces(c, surfaces, numsurfs);
        return true;
    }
    else if(numsurfs!=numsrcsurfs || memcmp(srcsurfaces, surfaces, numsurfs*sizeof(surfaceinfo))) 
    {
        if(!numsurfs) brightencube(c);
        else memcpy(srcsurfaces, surfaces, numsurfs*sizeof(surfaceinfo));
        return true;
    }
    else return false;
}

static bool previewblends(cube *c, const ivec &co, int size, const ivec &bo, const ivec &bs)
{
    bool changed = false;
    loopoctabox(co, size, bo, bs)
    {
        ivec o(i, co.x, co.y, co.z, size);
        cubeext *ext = c[i].ext;
        if(ext && ext->va && ext->va->hasmerges)
        {
            destroyva(ext->va);
            ext->va = NULL;
            invalidatemerges(c[i], true);
            changed = true;
        }
        if(c[i].children ? previewblends(c[i].children, o, size/2, bo, bs) : previewblends(c[i], o, size))  
        {
            changed = true;
            if(ext && ext->va)
            {
                int hasmerges = ext->va->hasmerges;
                destroyva(ext->va);
                ext->va = NULL;
                if(hasmerges) invalidatemerges(c[i], true);
            }
        }
    }
    return changed;
}

void previewblends(const ivec &bo, const ivec &bs)
{
    loadlayermasks();
    if(previewblends(worldroot, ivec(0, 0, 0), worldsize/2, bo, bs))
        commitchanges(true);
}
                            
void cleanuplightmaps()
{
    loopv(lightmaps)
    {
        LightMap &lm = lightmaps[i];
        lm.tex = lm.offsetx = lm.offsety = -1;
    }
    loopv(lightmaptexs) glDeleteTextures(1, &lightmaptexs[i].id);
    lightmaptexs.setsize(0);
    if(progresstex) { glDeleteTextures(1, &progresstex); progresstex = 0; }
}

void resetlightmaps()
{
    cleanuplightmaps();
    lightmaps.setsize(0);
    compressed.clear();
}

COMMAND(resetlightmaps, ""); // INTENSITY

static Uint32 calclight_timer(Uint32 interval, void *param)
{
    check_calclight_progress = true;
    return interval;
}

bool setlightmapquality(int quality)
{
    switch(quality)
    {
        case  1: lmshadows = 2; lmaa = 3; break;
        case  0: lmshadows = lmshadows_; lmaa = lmaa_; break;
        case -1: lmshadows = 1; lmaa = 0; break;
        default: return false;
    }
    return true;
}

void calclight(int *quality)
{
    if(!setlightmapquality(*quality))
    {
        conoutf(CON_ERROR, "valid range for calclight quality is -1..1"); 
        return;
    }
    renderbackground("computing lightmaps... (esc to abort)");
    mpremip(true);
    optimizeblendmap();
    loadlayermasks();
    resetlightmaps();
    clear_lmids(worldroot);
    curlumels = 0;
    progress = 0;
    progresstexticks = 0;
    calclight_canceled = false;
    check_calclight_progress = false;
    SDL_TimerID timer = SDL_AddTimer(250, calclight_timer, NULL);
    Uint32 start = SDL_GetTicks();
    calcnormals();
    show_calclight_progress();
    generate_lightmaps(worldroot, 0, 0, 0, worldsize >> 1);
    clearnormals();
    Uint32 end = SDL_GetTicks();
    if(timer) SDL_RemoveTimer(timer);
    uint total = 0, lumels = 0;
    loopv(lightmaps)
    {
        insert_unlit(i);
        if(!editmode) lightmaps[i].finalize();
        total += lightmaps[i].lightmaps;
        lumels += lightmaps[i].lumels;
    }
    if(!editmode) compressed.clear();
    initlights();
    renderbackground("lighting done...");
    allchanged();
    if(calclight_canceled)
        conoutf("calclight aborted");
    else
        conoutf("generated %d lightmaps using %d%% of %d textures (%.1f seconds)",
            total,
            lightmaps.length() ? lumels * 100 / (lightmaps.length() * LM_PACKW * LM_PACKH) : 0,
            lightmaps.length(),
            (end - start) / 1000.0f);
}

COMMAND(calclight, "i");

VAR(patchnormals, 0, 0, 1);

void patchlight(int *quality)
{
    if(noedit(true)) return;
    if(!setlightmapquality(*quality))
    {
        conoutf(CON_ERROR, "valid range for patchlight quality is -1..1"); 
        return;
    }
    renderbackground("patching lightmaps... (esc to abort)");
    loadlayermasks();
    cleanuplightmaps();
    progress = 0;
    progresstexticks = 0;
    int total = 0, lumels = 0;
    loopv(lightmaps)
    {
        total -= lightmaps[i].lightmaps;
        lumels -= lightmaps[i].lumels;
    }
    curlumels = lumels;
    calclight_canceled = false;
    check_calclight_progress = false;
    SDL_TimerID timer = SDL_AddTimer(250, calclight_timer, NULL);
    if(patchnormals) renderprogress(0, "computing normals...");
    Uint32 start = SDL_GetTicks();
    if(patchnormals) calcnormals();
    show_calclight_progress();
    generate_lightmaps(worldroot, 0, 0, 0, worldsize >> 1);
    if(patchnormals) clearnormals();
    Uint32 end = SDL_GetTicks();
    if(timer) SDL_RemoveTimer(timer);
    loopv(lightmaps)
    {
        total += lightmaps[i].lightmaps;
        lumels += lightmaps[i].lumels;
    }
    initlights();
    renderbackground("lighting done...");
    allchanged();
    if(calclight_canceled)
        conoutf("patchlight aborted");
    else
        conoutf("patched %d lightmaps using %d%% of %d textures (%.1f seconds)",
            total,
            lightmaps.length() ? lumels * 100 / (lightmaps.length() * LM_PACKW * LM_PACKH) : 0,
            lightmaps.length(),
            (end - start) / 1000.0f); 
}

COMMAND(patchlight, "i");

void setfullbrightlevel(int fullbrightlevel)
{
    if(lightmaptexs.length() > LMID_BRIGHT)
    {
        uchar bright[3] = { fullbrightlevel, fullbrightlevel, fullbrightlevel };
        createtexture(lightmaptexs[LMID_BRIGHT].id, 1, 1, bright, 0, 1);
    }
    initlights();
}

VARF(fullbright, 0, 0, 1, if(lightmaptexs.length()) initlights());
VARF(fullbrightlevel, 0, 128, 255, setfullbrightlevel(fullbrightlevel));

vector<LightMapTexture> lightmaptexs;

static void rotatenormals(LightMap &lmlv, int x, int y, int w, int h, int rotate)
{
    bool flipx = rotate>=2 && rotate<=4,
         flipy = (rotate>=1 && rotate<=2) || rotate==5,
         swapxy = (rotate&5)==1;
    uchar *lv = lmlv.data + 3*(y*LM_PACKW + x);
    int stride = 3*(LM_PACKW-w);
    loopi(h)
    {
        loopj(w)
        {
            if(flipx) lv[0] = 255 - lv[0];
            if(flipy) lv[1] = 255 - lv[1];
            if(swapxy) swap(lv[0], lv[1]);
            lv += 3;
        }
        lv += stride;
    }
}

static void rotatenormals(cube *c)
{
    loopi(8)
    {
        cube &ch = c[i];
        if(ch.children)
        {
            rotatenormals(ch.children);
            continue;
        }
        else if(!ch.ext || !ch.ext->surfaces) continue;
        loopj(6) if(lightmaps.inrange(ch.ext->surfaces[j].lmid+1-LMID_RESERVED))
        {
            Slot &slot = lookuptexture(ch.texture[j], false);
            if(!slot.rotation) continue;
            surfaceinfo &surface = ch.ext->surfaces[j];
            LightMap &lmlv = lightmaps[surface.lmid+1-LMID_RESERVED];
            if((lmlv.type&LM_TYPE)!=LM_BUMPMAP1) continue;
            rotatenormals(lmlv, surface.x, surface.y, surface.w, surface.h, slot.rotation < 4 ? 4-slot.rotation : slot.rotation);
        }
    }
}

void fixlightmapnormals()
{
    rotatenormals(worldroot);
}

static void convertlightmap(LightMap &lmc, LightMap &lmlv, uchar *dst, size_t stride)
{
    const uchar *c = lmc.data;
    const bvec *lv = (const bvec *)lmlv.data;
    loopi(LM_PACKH)
    {
        uchar *dstrow = dst;
        loopj(LM_PACKW)
        {
            int z = int(lv->z)*2 - 255,
                r = (int(c[0]) * z) / 255,
                g = (int(c[1]) * z) / 255,
                b = (int(c[2]) * z) / 255;
            dstrow[0] = max(r, int(ambientcolor[0]));
            dstrow[1] = max(g, int(ambientcolor[1]));
            dstrow[2] = max(b, int(ambientcolor[2]));
            if(lmc.bpp==4) dstrow[3] = c[3];
            c += lmc.bpp;
            lv++;
            dstrow += lmc.bpp;
        }
        dst += stride;
    }
}

static void copylightmap(LightMap &lm, uchar *dst, size_t stride)
{
    const uchar *c = lm.data;
    loopi(LM_PACKH)
    {
        memcpy(dst, c, lm.bpp*LM_PACKW);
        c += lm.bpp*LM_PACKW;
        dst += stride;
    }
}

VARF(convertlms, 0, 1, 1, { cleanuplightmaps(); initlights(); allchanged(); });

void genreservedlightmaptexs()
{
    while(lightmaptexs.length() < LMID_RESERVED)
    {
        LightMapTexture &tex = lightmaptexs.add();
        tex.type = renderpath != R_FIXEDFUNCTION && lightmaptexs.length()&1 ? LM_DIFFUSE : LM_BUMPMAP1;
        glGenTextures(1, &tex.id);
    }
    uchar unlit[3] = { ambientcolor[0], ambientcolor[1], ambientcolor[2] };
    createtexture(lightmaptexs[LMID_AMBIENT].id, 1, 1, unlit, 0, 1);
    bvec front(128, 128, 255);
    createtexture(lightmaptexs[LMID_AMBIENT1].id, 1, 1, &front, 0, 1);
    uchar bright[3] = { fullbrightlevel, fullbrightlevel, fullbrightlevel };
    createtexture(lightmaptexs[LMID_BRIGHT].id, 1, 1, bright, 0, 1);
    createtexture(lightmaptexs[LMID_BRIGHT1].id, 1, 1, &front, 0, 1);
    uchar dark[3] = { 0, 0, 0 };
    createtexture(lightmaptexs[LMID_DARK].id, 1, 1, dark, 0, 1);
    createtexture(lightmaptexs[LMID_DARK1].id, 1, 1, &front, 0, 1);
}

static void findunlit(int i)
{
    LightMap &lm = lightmaps[i];
    if(lm.unlitx>=0) return;
    else if((lm.type&LM_TYPE)==LM_BUMPMAP0)
    {
        if(i+1>=lightmaps.length() || (lightmaps[i+1].type&LM_TYPE)!=LM_BUMPMAP1) return;
    }
    else if((lm.type&LM_TYPE)!=LM_DIFFUSE) return;
    uchar *data = lm.data;
    loop(y, 2) loop(x, LM_PACKW)
    {
        if(!data[0] && !data[1] && !data[2])
        {
            memcpy(data, ambientcolor.v, 3);
            if((lm.type&LM_TYPE)==LM_BUMPMAP0) ((bvec *)lightmaps[i+1].data)[y*LM_PACKW + x] = bvec(128, 128, 255);
            lm.unlitx = x;
            lm.unlity = y;
            return;
        }
        if(data[0]==ambientcolor[0] && data[1]==ambientcolor[1] && data[2]==ambientcolor[2])
        {
            if((lm.type&LM_TYPE)!=LM_BUMPMAP0 || ((bvec *)lightmaps[i+1].data)[y*LM_PACKW + x] == bvec(128, 128, 255))
            {
                lm.unlitx = x;
                lm.unlity = y;
                return;
            }
        }
        data += lm.bpp;
    }
}

VARF(roundlightmaptex, 0, 4, 16, { cleanuplightmaps(); initlights(); allchanged(); });
VARF(batchlightmaps, 0, 4, 256, { cleanuplightmaps(); initlights(); allchanged(); });

void genlightmaptexs(int flagmask, int flagval)
{
    if(lightmaptexs.length() < LMID_RESERVED) genreservedlightmaptexs();

    int remaining[3] = { 0, 0, 0 }, total = 0; 
    loopv(lightmaps) 
    {
        LightMap &lm = lightmaps[i];
        if(lm.tex >= 0 || (lm.type&flagmask)!=flagval) continue;
        int type = lm.type&LM_TYPE;
        remaining[type]++; 
        total++;
        if(lm.unlitx < 0) findunlit(i);
    }

    if(renderpath==R_FIXEDFUNCTION)
    {
        remaining[LM_DIFFUSE] += remaining[LM_BUMPMAP0];
        remaining[LM_BUMPMAP0] = remaining[LM_BUMPMAP1] = 0;
    }

    int sizelimit = (maxtexsize ? min(maxtexsize, hwtexsize) : hwtexsize)/max(LM_PACKW, LM_PACKH);
    sizelimit = min(batchlightmaps, sizelimit*sizelimit);
    while(total)
    {
        int type = LM_DIFFUSE;
        LightMap *firstlm = NULL;
        loopv(lightmaps)
        {
            LightMap &lm = lightmaps[i];
            if(lm.tex >= 0 || (lm.type&flagmask) != flagval) continue;
            if(renderpath != R_FIXEDFUNCTION) type = lm.type&LM_TYPE;
            else if((lm.type&LM_TYPE) == LM_BUMPMAP1) continue;
            firstlm = &lm; 
            break; 
        }
        if(!firstlm) break;
        int used = 0, uselimit = min(remaining[type], sizelimit);
        do used++; while((1<<used) <= uselimit);
        used--;
        int oldval = remaining[type];
        remaining[type] -= 1<<used;
        if(remaining[type] && (2<<used) <= min(roundlightmaptex, sizelimit))
        {
            remaining[type] -= min(remaining[type], 1<<used);
            used++;
        }
        total -= oldval - remaining[type];
        LightMapTexture &tex = lightmaptexs.add();
        tex.type = firstlm->type;
        tex.w = LM_PACKW<<((used+1)/2);
        tex.h = LM_PACKH<<(used/2);
        int bpp = firstlm->bpp;
        uchar *data = used || (renderpath == R_FIXEDFUNCTION && (firstlm->type&LM_TYPE) == LM_BUMPMAP0 && convertlms) ? 
            new uchar[bpp*tex.w*tex.h] : 
            NULL;
        int offsetx = 0, offsety = 0;
        loopv(lightmaps)
        {
            LightMap &lm = lightmaps[i];
            if(lm.tex >= 0 || (lm.type&flagmask) != flagval || 
               (renderpath==R_FIXEDFUNCTION ? 
                (lm.type&LM_TYPE) == LM_BUMPMAP1 : 
                (lm.type&LM_TYPE) != type))
                continue;

            lm.tex = lightmaptexs.length()-1;
            lm.offsetx = offsetx;
            lm.offsety = offsety;
            if(tex.unlitx < 0 && lm.unlitx >= 0) 
            { 
                tex.unlitx = offsetx + lm.unlitx; 
                tex.unlity = offsety + lm.unlity;
            }

            if(data)
            {
                if(renderpath == R_FIXEDFUNCTION && (lm.type&LM_TYPE) == LM_BUMPMAP0 && convertlms)
                    convertlightmap(lm, lightmaps[i+1], &data[bpp*(offsety*tex.w + offsetx)], bpp*tex.w);
                else copylightmap(lm, &data[bpp*(offsety*tex.w + offsetx)], bpp*tex.w);
            }

            offsetx += LM_PACKW;
            if(offsetx >= tex.w) { offsetx = 0; offsety += LM_PACKH; }
            if(offsety >= tex.h) break;
        }
        
        glGenTextures(1, &tex.id);
        createtexture(tex.id, tex.w, tex.h, data ? data : firstlm->data, 3, 1, bpp==4 ? GL_RGBA : GL_RGB);
        if(data) delete[] data;
    }        
}

bool brightengeom = false;

void clearlights()
{
    clearlightcache();
    const vector<extentity *> &ents = entities::getents();
    loopv(ents)
    {
        extentity &e = *ents[i];
        e.light.color = vec(1, 1, 1);
        e.light.dir = vec(0, 0, 1);
    }
    if(nolights) return;

    genlightmaptexs(LM_ALPHA, 0);
    genlightmaptexs(LM_ALPHA, LM_ALPHA);
    brightengeom = true;
}

void lightent(extentity &e, float height)
{
    if(e.type==ET_LIGHT) return;
    float ambient = 0.0f;
    if(e.type==ET_MAPMODEL)
    {
        LogicEntityPtr entity = LogicSystem::getLogicEntity(e); // INTENSITY
        model *m = entity.get() ? entity->getModel() : NULL; // INTENSITY
        if(m) height = m->above()*0.75f;
    }
    else if(e.type>=ET_GAMESPECIFIC) ambient = 0.4f;
    vec target(e.o.x, e.o.y, e.o.z + height);
    lightreaching(target, e.light.color, e.light.dir, &e, ambient);
}

void updateentlighting()
{
    const vector<extentity *> &ents = entities::getents();
    loopv(ents) lightent(*ents[i]);
}

void initlights()
{
    if(nolights || (fullbright && editmode) || lightmaps.empty())
    {
        clearlights();
        return;
    }

    clearlightcache();
    updateentlighting();
    genlightmaptexs(LM_ALPHA, 0);
    genlightmaptexs(LM_ALPHA, LM_ALPHA);
    brightengeom = false;
}

void lightreaching(const vec &target, vec &color, vec &dir, extentity *t, float ambient)
{
    if(nolights || (fullbright && editmode) || lightmaps.empty())
    {
        color = vec(1, 1, 1);
        dir = vec(0, 0, 1);
        return;
    }

    color = dir = vec(0, 0, 0);
    const vector<extentity *> &ents = entities::getents();
    const vector<int> &lights = checklightcache(int(target.x), int(target.y));
    loopv(lights)
    {
        extentity &e = *ents[lights[i]];
        if(e.type != ET_LIGHT)
            continue;
    
        vec ray(target);
        ray.sub(e.o);
        float mag = ray.magnitude();
        if(e.attr1 && mag >= float(e.attr1))
            continue;
    
        ray.div(mag);
        if(shadowray(e.o, ray, mag, RAY_SHADOW | RAY_POLY, t) < mag)
            continue;
        float intensity = 1;
        if(e.attr1)
            intensity -= mag / float(e.attr1);
        if(e.attached && e.attached->type==ET_SPOTLIGHT)
        {
            vec spot(vec(e.attached->o).sub(e.o).normalize());
            float maxatten = 1-cosf(max(1, min(90, int(e.attached->attr1)))*RAD);
            float spotatten = 1-(1-ray.dot(spot))/maxatten;
            if(spotatten<=0) continue;
            intensity *= spotatten;
        }

        //if(target==player->o)
        //{
        //    conoutf(CON_DEBUG, "%d - %f %f", i, intensity, mag);
        //}
 
        color.add(vec(e.attr2, e.attr3, e.attr4).div(255).mul(intensity));

        intensity *= e.attr2*e.attr3*e.attr4;

        if(fabs(mag)<1e-3) dir.add(vec(0, 0, 1));
        else dir.add(vec(e.o).sub(target).mul(intensity/mag));
    }

    if(t && hasskylight())
    {
        uchar skylight[3];
        calcskylight(target, vec(0, 0, 0), 0.5f, skylight, RAY_POLY, t);
        loopk(3) color[k] = min(1.5f, max(max(skylight[k]/255.0f, ambient), color[k]));
    }
    else loopk(3)
    {
        float skylight = 0.75f*max(max(skylightcolor[k], ambientcolor[k])/255.0f, ambient) + 0.25f*max(ambientcolor[k]/255.0f, ambient);
        color[k] = min(1.5f, max(skylight, color[k]));
    }
    if(dir.iszero()) dir = vec(0, 0, 1);
    else dir.normalize();
}

entity *brightestlight(const vec &target, const vec &dir)
{
    const vector<extentity *> &ents = entities::getents();
    const vector<int> &lights = checklightcache(int(target.x), int(target.y));
    extentity *brightest = NULL;
    float bintensity = 0;
    loopv(lights)
    {
        extentity &e = *ents[lights[i]];
        if(e.type != ET_LIGHT || vec(e.o).sub(target).dot(dir)<0)
            continue;

        vec ray(target);
        ray.sub(e.o);
        float mag = ray.magnitude();
        if(e.attr1 && mag >= float(e.attr1))
             continue;

        ray.div(mag);
        if(shadowray(e.o, ray, mag, RAY_SHADOW | RAY_POLY) < mag)
            continue;
        float intensity = 1;
        if(e.attr1)
            intensity -= mag / float(e.attr1);
        if(e.attached && e.attached->type==ET_SPOTLIGHT)
        {
            vec spot(vec(e.attached->o).sub(e.o).normalize());
            float maxatten = 1-cosf(max(1, min(90, int(e.attached->attr1)))*RAD);
            float spotatten = 1-(1-ray.dot(spot))/maxatten;
            if(spotatten<=0) continue;
            intensity *= spotatten;
        }

        if(!brightest || intensity > bintensity)
        {
            brightest = &e;
            bintensity = intensity;
        }
    }
    return brightest;
}

void brightencube(cube &c)
{
    if(c.ext && c.ext->surfaces)
    {
        if(c.ext->surfaces==brightsurfaces) return;
        freesurfaces(c);
    }
    ext(c).surfaces = brightsurfaces;
}
        
void newsurfaces(cube &c, const surfaceinfo *surfs, int numsurfs)
{
    if(!c.ext) newcubeext(c);
    if(!c.ext->surfaces || c.ext->surfaces==brightsurfaces)
    {
        c.ext->surfaces = new surfaceinfo[numsurfs];
        memcpy(c.ext->surfaces, surfs, numsurfs*sizeof(surfaceinfo));
    }
}

void freesurfaces(cube &c)
{
    if(c.ext)
    {
        if(c.ext->surfaces==brightsurfaces) c.ext->surfaces = NULL;
        else DELETEA(c.ext->surfaces);
    }
}

void dumplms()
{
    loopv(lightmaps)
    {
        ImageData temp(LM_PACKW, LM_PACKH, lightmaps[i].bpp, lightmaps[i].data);
        const char *map = game::getclientmap(), *name = strrchr(map, '/');
        defformatstring(buf)("lightmap_%s_%d.png", name ? name+1 : map, i);
        savepng(buf, temp, true);
    }
}

COMMAND(dumplms, "");

