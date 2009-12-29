// renderva.cpp: handles the occlusion and rendering of vertex arrays

#include "engine.h"

static inline void drawtris(GLsizei numindices, const GLvoid *indices, ushort minvert, ushort maxvert)
{
    if(hasDRE) glDrawRangeElements_(GL_TRIANGLES, minvert, maxvert, numindices, GL_UNSIGNED_SHORT, indices);
    else glDrawElements(GL_TRIANGLES, numindices, GL_UNSIGNED_SHORT, indices);
    glde++;
}

static inline void drawvatris(vtxarray *va, GLsizei numindices, const GLvoid *indices)
{
    drawtris(numindices, indices, va->minvert, va->maxvert);
}

///////// view frustrum culling ///////////////////////

plane vfcP[5];  // perpindictular vectors to view frustrum bounding planes
float vfcDfog;  // far plane culling distance (fog limit).
float vfcDnear[5], vfcDfar[5];
float vfcfov, vfcfovy;

vtxarray *visibleva;

int isvisiblesphere(float rad, const vec &cv)
{
    int v = VFC_FULL_VISIBLE;
    float dist;

    loopi(5)
    {
        dist = vfcP[i].dist(cv);
        if(dist < -rad) return VFC_NOT_VISIBLE;
        if(dist < rad) v = VFC_PART_VISIBLE;
    }

    dist -= vfcDfog;
    if(dist > rad) return VFC_FOGGED;  //VFC_NOT_VISIBLE;    // culling when fog is closer than size of world results in HOM
    if(dist > -rad) v = VFC_PART_VISIBLE;

    return v;
}

int isvisiblecube(const ivec &o, int size)
{
    int v = VFC_FULL_VISIBLE;
    float dist;

    loopi(5)
    {
        dist = o.dist(vfcP[i]);
        if(dist < -vfcDfar[i]*size) return VFC_NOT_VISIBLE;
        if(dist < -vfcDnear[i]*size) v = VFC_PART_VISIBLE;
    }

    dist -= vfcDfog;
    if(dist > -vfcDnear[4]*size) return VFC_FOGGED;
    if(dist > -vfcDfar[4]*size) v = VFC_PART_VISIBLE;

    return v;
}

float vadist(vtxarray *va, const vec &p)
{
    return p.dist_to_bb(va->bbmin, va->bbmax);
}

#define VASORTSIZE 64

static vtxarray *vasort[VASORTSIZE];

void addvisibleva(vtxarray *va)
{
    float dist = vadist(va, camera1->o);
    va->distance = int(dist); /*cv.dist(camera1->o) - va->size*SQRT3/2*/

    int hash = min(int(dist*VASORTSIZE/worldsize), VASORTSIZE-1);
    vtxarray **prev = &vasort[hash], *cur = vasort[hash];

    while(cur && va->distance >= cur->distance)
    {
        prev = &cur->next;
        cur = cur->next;
    }

    va->next = *prev;
    *prev = va;
}

void sortvisiblevas()
{
    visibleva = NULL; 
    vtxarray **last = &visibleva;
    loopi(VASORTSIZE) if(vasort[i])
    {
        vtxarray *va = vasort[i];
        *last = va;
        while(va->next) va = va->next;
        last = &va->next;
    }
}

void findvisiblevas(vector<vtxarray *> &vas, bool resetocclude = false)
{
    loopv(vas)
    {
        vtxarray &v = *vas[i];
        int prevvfc = resetocclude ? VFC_NOT_VISIBLE : v.curvfc;
        v.curvfc = isvisiblecube(v.o, v.size);
        if(v.curvfc!=VFC_NOT_VISIBLE) 
        {
            if(pvsoccluded(v.o, v.size))
            {
                v.curvfc += PVS_FULL_VISIBLE - VFC_FULL_VISIBLE;
                continue;
            }
            addvisibleva(&v);
            if(v.children.length()) findvisiblevas(v.children, prevvfc>=VFC_NOT_VISIBLE);
            if(prevvfc>=VFC_NOT_VISIBLE)
            {
                v.occluded = !v.texs || pvsoccluded(v.geommin, v.geommax) ? OCCLUDE_GEOM : OCCLUDE_NOTHING;
                v.query = NULL;
            }
        }
    }
}

void calcvfcD()
{
    loopi(5)
    {
        plane &p = vfcP[i];
        vfcDnear[i] = vfcDfar[i] = 0;
        loopk(3) if(p[k] > 0) vfcDfar[i] += p[k];
        else vfcDnear[i] += p[k];
    }
} 

void setvfcP(float yaw, float pitch, const vec &camera, float minyaw = -M_PI, float maxyaw = M_PI, float minpitch = -M_PI, float maxpitch = M_PI)
{
    yaw *= RAD;
    pitch *= RAD;
    vfcP[0].toplane(vec(yaw + M_PI/2 - min(vfcfov, -minyaw), pitch), camera);   // left plane
    vfcP[1].toplane(vec(yaw - M_PI/2 + min(vfcfov,  maxyaw), pitch), camera);   // right plane
    vfcP[2].toplane(vec(yaw, pitch + M_PI/2 - min(vfcfovy, -minpitch)), camera); // top plane
    vfcP[3].toplane(vec(yaw, pitch - M_PI/2 + min(vfcfovy,  maxpitch)), camera); // bottom plane
    vfcP[4].toplane(vec(yaw, pitch), camera);          // near/far planes
    extern int fog;
    vfcDfog = fog;
    calcvfcD();
}

plane oldvfcP[5];

void reflectvfcP(float z, float minyaw, float maxyaw, float minpitch, float maxpitch)
{
    memcpy(oldvfcP, vfcP, sizeof(vfcP));

    if(z < 0) setvfcP(camera1->yaw, camera1->pitch, camera1->o, minyaw, maxyaw, minpitch, maxpitch);
    else
    {
        vec o(camera1->o);
        o.z = z-(camera1->o.z-z);
        setvfcP(camera1->yaw, -camera1->pitch, o, minyaw, maxyaw, -maxpitch, -minpitch);
    }
}

void restorevfcP()
{
    memcpy(vfcP, oldvfcP, sizeof(vfcP));
    calcvfcD();
}

extern vector<vtxarray *> varoot;

void visiblecubes(float fov, float fovy)
{
    memset(vasort, 0, sizeof(vasort));

    vfcfov = fov*0.5f*RAD;
    vfcfovy = fovy*0.5f*RAD;

    // Calculate view frustrum: Only changes if resize, but...
    setvfcP(camera1->yaw, camera1->pitch, camera1->o);

    findvisiblevas(varoot);
    sortvisiblevas();
}

static inline bool insideva(const vtxarray *va, const vec &v, int margin = 1)
{
    int size = va->size + margin;
    return v.x>=va->o.x-margin && v.y>=va->o.y-margin && v.z>=va->o.z-margin && 
           v.x<=va->o.x+size && v.y<=va->o.y+size && v.z<=va->o.z+size;
}

static ivec vaorigin;

static void resetorigin()
{
    vaorigin = ivec(-1, -1, -1);
}

static bool setorigin(vtxarray *va, bool shadowmatrix = false)
{
    ivec o = floatvtx ? ivec(0, 0, 0) : ivec(va->o).mask(~VVEC_INT_MASK).add(0x8000>>VVEC_FRAC);
    if(o != vaorigin)
    {
        vaorigin = o;
        glPopMatrix();
        glPushMatrix();
        glTranslatef(o.x, o.y, o.z);
        static const float scale = 1.0f / (1<<VVEC_FRAC);
        glScalef(scale, scale, scale);

        if(shadowmatrix) adjustshadowmatrix(o, scale);
        return true;
    }
    return false;
}

///////// occlusion queries /////////////

#define MAXQUERY 2048

struct queryframe
{
    int cur, max;
    occludequery queries[MAXQUERY];
};

static queryframe queryframes[2] = {{0, 0}, {0, 0}};
static uint flipquery = 0;

int getnumqueries()
{
    return queryframes[flipquery].cur;
}

void flipqueries()
{
    flipquery = (flipquery + 1) % 2;
    queryframe &qf = queryframes[flipquery];
    loopi(qf.cur) qf.queries[i].owner = NULL;
    qf.cur = 0;
}

occludequery *newquery(void *owner)
{
    queryframe &qf = queryframes[flipquery];
    if(qf.cur >= qf.max)
    {
        if(qf.max >= MAXQUERY) return NULL;
        glGenQueries_(1, &qf.queries[qf.max++].id);
    }
    occludequery *query = &qf.queries[qf.cur++];
    query->owner = owner;
    query->fragments = -1;
    return query;
}

void resetqueries()
{
    loopi(2) loopj(queryframes[i].max) queryframes[i].queries[j].owner = NULL;
}

void clearqueries()
{
    loopi(2)
    {
        queryframe &qf = queryframes[i];
        loopj(qf.max) 
        {
            glDeleteQueries_(1, &qf.queries[j].id);
            qf.queries[j].owner = NULL;
        }
        qf.cur = qf.max = 0;
    }
}

VAR(oqfrags, 0, 8, 64);
VAR(oqreflect, 0, 4, 64);
VAR(oqwait, 0, 1, 1);

bool checkquery(occludequery *query, bool nowait)
{
    GLuint fragments;
    if(query->fragments >= 0) fragments = query->fragments;
    else
    {
        if(nowait || !oqwait)
        {
            GLint avail;
            glGetQueryObjectiv_(query->id, GL_QUERY_RESULT_AVAILABLE, &avail);
            if(!avail) return false;
        }
        glGetQueryObjectuiv_(query->id, GL_QUERY_RESULT_ARB, &fragments);
        query->fragments = fragments;
    }
    return fragments < (uint)(reflecting || refracting ? oqreflect : oqfrags);
}

void drawbb(const ivec &bo, const ivec &br, const vec &camera, int scale, const ivec &origin)
{
    glBegin(GL_QUADS);

    loopi(6)
    {
        int dim = dimension(i), coord = dimcoord(i);

        if(coord)
        {
            if(camera[dim] < bo[dim] + br[dim]) continue;
        }
        else if(camera[dim] > bo[dim]) continue;

        loopj(4)
        {
            const ivec &cc = cubecoords[fv[i][j]];
            glVertex3f(((cc.x ? bo.x+br.x : bo.x) - origin.x) << scale,
                       ((cc.y ? bo.y+br.y : bo.y) - origin.y) << scale,
                       ((cc.z ? bo.z+br.z : bo.z) - origin.z) << scale);
        }

        xtraverts += 4;
    }

    glEnd();
}

extern int octaentsize;

static octaentities *visiblemms, **lastvisiblemms;

void findvisiblemms(const vector<extentity *> &ents)
{
    for(vtxarray *va = visibleva; va; va = va->next)
    {
        if(!va->mapmodels || va->curvfc >= VFC_FOGGED || va->occluded >= OCCLUDE_BB) continue;
        loopv(*va->mapmodels)
        {
            octaentities *oe = (*va->mapmodels)[i];
            if(isvisiblecube(oe->o, oe->size) >= VFC_FOGGED || pvsoccluded(oe->bbmin, ivec(oe->bbmax).sub(oe->bbmin))) continue;

            bool occluded = oe->query && oe->query->owner == oe && checkquery(oe->query);
            if(occluded)
            {
                oe->distance = -1;

                oe->next = NULL;
                *lastvisiblemms = oe;
                lastvisiblemms = &oe->next;
            }
            else
            {
                int visible = 0;
                loopv(oe->mapmodels)
                {
                    extentity &e = *ents[oe->mapmodels[i]];
                    if(e.attr3 && e.triggerstate == TRIGGER_DISAPPEARED) continue;
                    e.visible = true;
                    ++visible;
                }
                if(!visible) continue;

                oe->distance = int(camera1->o.dist_to_bb(oe->o, oe->size));

                octaentities **prev = &visiblemms, *cur = visiblemms;
                while(cur && cur->distance >= 0 && oe->distance > cur->distance)
                {
                    prev = &cur->next;
                    cur = cur->next;
                }

                if(*prev == NULL) lastvisiblemms = &oe->next;
                oe->next = *prev;
                *prev = oe;
            }
        }
    }
}

VAR(oqmm, 0, 4, 8);

extern bool getentboundingbox(extentity &e, ivec &o, ivec &r);

void rendermapmodel(extentity &e)
{
#if 0 // INTENSITY: Use new systems
    int anim = ANIM_MAPMODEL|ANIM_LOOP, basetime = 0;
    if(e.attr3) switch(e.triggerstate)
    {
        case TRIGGER_RESET: anim = ANIM_TRIGGER|ANIM_START; break;
        case TRIGGERING: anim = ANIM_TRIGGER; basetime = e.lasttrigger; break;
        case TRIGGERED: anim = ANIM_TRIGGER|ANIM_END; break;
        case TRIGGER_RESETTING: anim = ANIM_TRIGGER|ANIM_REVERSE; basetime = e.lasttrigger; break;
    }
    mapmodelinfo &mmi = getmminfo(e.attr2);
    if(&mmi) rendermodel(&e.light, mmi.name, anim, e.o, (float)((e.attr1+7)-(e.attr1+7)%15), 0, MDL_CULL_VFC | MDL_CULL_DIST | MDL_DYNLIGHT, NULL, NULL, basetime);
#else
    LogicEntityPtr entity = LogicSystem::getLogicEntity(e);
    if (!entity.get() || entity->isNone())
    {
        Logging::log(Logging::ERROR, "Trying to show a missing mapmodel\r\n");
        Logging::log(Logging::ERROR, "                                  %d\r\n", LogicSystem::getUniqueId(&e));
        assert(0);
    }
    int anim     = entity.get()->getAnimation(); // ANIM_MAPMODEL|ANIM_LOOP
    int basetime = entity.get()->getStartTime();

    model* theModel = LogicSystem::getLogicEntity(e).get()->getModel();

    // Kripken: MDL_SHADOW is necessary for getting shadows for a mapmodel. Note however the notes in fpsrender.h, that isn't enough.
    if(theModel)
        rendermodel(&e.light, theModel->name(), anim, e.o, entity, (float)((e.attr1+7)-(e.attr1+7)%15), 0, 0, MDL_CULL_VFC | MDL_CULL_DIST | MDL_DYNLIGHT, NULL, NULL, basetime); // INTENSITY: Added roll = 0
#endif
}

extern int reflectdist;

vtxarray *reflectedva;

void renderreflectedmapmodels()
{
    const vector<extentity *> &ents = entities::getents();

    octaentities *mms = visiblemms;
    if(reflecting)
    {
        octaentities **lastmms = &mms;
        for(vtxarray *va = reflectedva; va; va = va->rnext)
        {
            if(!va->mapmodels || va->distance > reflectdist) continue;
            loopv(*va->mapmodels) 
            {
                octaentities *oe = (*va->mapmodels)[i];
                *lastmms = oe;
                lastmms = &oe->rnext;
            }
        }
        *lastmms = NULL;
    }
    for(octaentities *oe = mms; oe; oe = reflecting ? oe->rnext : oe->next)
    {
        if(reflecting || refracting>0 ? oe->bbmax.z <= reflectz : oe->bbmin.z >= reflectz) continue;
        if(isvisiblecube(oe->o, oe->size) >= VFC_FOGGED) continue;
        loopv(oe->mapmodels)
        {
           extentity &e = *ents[oe->mapmodels[i]];
           if(e.visible || (e.attr3 && e.triggerstate == TRIGGER_DISAPPEARED)) continue;
           e.visible = true;
        }
    }
    if(mms)
    {
        startmodelbatches();
        for(octaentities *oe = mms; oe; oe = reflecting ? oe->rnext : oe->next)
        {
            loopv(oe->mapmodels)
            {
                extentity &e = *ents[oe->mapmodels[i]];
                if(!e.visible) continue;
                rendermapmodel(e);
                e.visible = false;
            }
        }
        endmodelbatches();
    }
}

void rendermapmodels()
{
    const vector<extentity *> &ents = entities::getents();

    visiblemms = NULL;
    lastvisiblemms = &visiblemms;
    findvisiblemms(ents);

    static int skipoq = 0;
    bool doquery = hasOQ && oqfrags && oqmm;

    startmodelbatches();
    for(octaentities *oe = visiblemms; oe; oe = oe->next) if(oe->distance>=0)
    {
        bool rendered = false;
        loopv(oe->mapmodels)
        {
            extentity &e = *ents[oe->mapmodels[i]];
            if(!e.visible) continue;
            if(!rendered)
            {
                rendered = true;
                oe->query = doquery && oe->distance>0 && !(++skipoq%oqmm) ? newquery(oe) : NULL;
                if(oe->query) startmodelquery(oe->query);
            }        
            rendermapmodel(e);
            e.visible = false;
        }
        if(rendered && oe->query) endmodelquery();
    }
    endmodelbatches();

    bool colormask = true;
    for(octaentities *oe = visiblemms; oe; oe = oe->next) if(oe->distance<0)
    {
        oe->query = doquery ? newquery(oe) : NULL;
        if(!oe->query) continue;
        if(colormask)
        {
            glDepthMask(GL_FALSE);
            glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
            nocolorshader->set();
            colormask = false;
        }
        startquery(oe->query);
        drawbb(oe->bbmin, ivec(oe->bbmax).sub(oe->bbmin));
        endquery(oe->query);
    }
    if(!colormask)
    {
        glDepthMask(GL_TRUE);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, fading ? GL_FALSE : GL_TRUE);
    }
}

static inline bool bbinsideva(const ivec &bo, const ivec &br, vtxarray *va)
{
    return bo.x >= va->bbmin.x && bo.y >= va->bbmin.y && va->o.z >= va->bbmin.z &&
        bo.x + br.x <= va->bbmax.x && bo.y + br.y <= va->bbmax.y && bo.z + br.z <= va->bbmax.z; 
}

static inline bool bboccluded(const ivec &bo, const ivec &br, cube *c, const ivec &o, int size)
{
    loopoctabox(o, size, bo, br)
    {
        ivec co(i, o.x, o.y, o.z, size);
        if(c[i].ext && c[i].ext->va)
        {
            vtxarray *va = c[i].ext->va;
            if(va->curvfc >= VFC_FOGGED || (va->occluded >= OCCLUDE_BB && bbinsideva(bo, br, va))) continue;
        }
        if(c[i].children && bboccluded(bo, br, c[i].children, co, size>>1)) continue;
        return false;
    }
    return true;
}

bool bboccluded(const ivec &bo, const ivec &br)
{
    int diff = (bo.x^(bo.x+br.x)) | (bo.y^(bo.y+br.y)) | (bo.z^(bo.z+br.z));
    if(diff&~((1<<worldscale)-1)) return false;
    int scale = worldscale-1;
    if(diff&(1<<scale)) return bboccluded(bo, br, worldroot, ivec(0, 0, 0), 1<<scale);
    cube *c = &worldroot[octastep(bo.x, bo.y, bo.z, scale)];
    if(c->ext && c->ext->va)
    {
        vtxarray *va = c->ext->va;
        if(va->curvfc >= VFC_FOGGED || (va->occluded >= OCCLUDE_BB && bbinsideva(bo, br, va))) return true;
    }
    scale--;
    while(c->children && !(diff&(1<<scale)))
    {
        c = &c->children[octastep(bo.x, bo.y, bo.z, scale)];
        if(c->ext && c->ext->va)
        {
            vtxarray *va = c->ext->va;
            if(va->curvfc >= VFC_FOGGED || (va->occluded >= OCCLUDE_BB && bbinsideva(bo, br, va))) return true;
        }
        scale--;
    }
    if(c->children) return bboccluded(bo, br, c->children, ivec(bo).mask(~((2<<scale)-1)), 1<<scale);
    return false;
}

extern int ati_texgen_bug;

static void setuptexgen(int dims = 2)
{
    glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
    glEnable(GL_TEXTURE_GEN_S);
    if(dims>=2)
    {
        glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
        glEnable(GL_TEXTURE_GEN_T);
        if(ati_texgen_bug) glEnable(GL_TEXTURE_GEN_R);     // should not be needed, but apparently makes some ATI drivers happy
    }
}

static void disabletexgen(int dims = 2)
{
    glDisable(GL_TEXTURE_GEN_S);
    if(dims>=2)
    {
        glDisable(GL_TEXTURE_GEN_T);
        if(ati_texgen_bug) glDisable(GL_TEXTURE_GEN_R);
    }
}

HVAR(outline, 0, 0, 0xFFFFFF);
VAR(dtoutline, 0, 1, 1);

void renderoutline()
{
    notextureshader->set();

    glDisable(GL_TEXTURE_2D);
    glEnableClientState(GL_VERTEX_ARRAY);

    glPushMatrix();

    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glColor3ub((outline>>16)&0xFF, (outline>>8)&0xFF, outline&0xFF);

    enablepolygonoffset(GL_POLYGON_OFFSET_LINE);

    if(!dtoutline) glDisable(GL_DEPTH_TEST);

    resetorigin();    
    vtxarray *prev = NULL;
    for(vtxarray *va = visibleva; va; va = va->next)
    {
        if(!va->texs || va->occluded >= OCCLUDE_GEOM) continue;

        if(!prev || va->vbuf != prev->vbuf)
        {
            setorigin(va);
            if(hasVBO)
            {
                glBindBuffer_(GL_ARRAY_BUFFER_ARB, va->vbuf);
                glBindBuffer_(GL_ELEMENT_ARRAY_BUFFER_ARB, va->ebuf);
            }
            glVertexPointer(3, floatvtx ? GL_FLOAT : GL_SHORT, VTXSIZE, &va->vdata[0].x);
        }

        drawvatris(va, 3*va->tris, va->edata);
        xtravertsva += va->verts;
        
        prev = va;
    }

    if(!dtoutline) glEnable(GL_DEPTH_TEST);

    disablepolygonoffset(GL_POLYGON_OFFSET_LINE);

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    glPopMatrix();

    if(hasVBO)
    {
        glBindBuffer_(GL_ARRAY_BUFFER_ARB, 0);
        glBindBuffer_(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
    }
    glDisableClientState(GL_VERTEX_ARRAY);
    glEnable(GL_TEXTURE_2D);

    defaultshader->set();
}

HVAR(blendbrushcolor, 0, 0x0000C0, 0xFFFFFF);

void renderblendbrush(GLuint tex, float x, float y, float w, float h)
{
    static Shader *blendbrushshader = NULL;
    if(!blendbrushshader) blendbrushshader = lookupshaderbyname("blendbrush");
    blendbrushshader->set();

    glEnableClientState(GL_VERTEX_ARRAY);

    glPushMatrix();

    glDepthFunc(GL_LEQUAL);

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_TEXTURE_2D); 
    glBindTexture(GL_TEXTURE_2D, tex);
    glColor4ub((blendbrushcolor>>16)&0xFF, (blendbrushcolor>>8)&0xFF, blendbrushcolor&0xFF, 0x40);

    GLfloat s[4] = { 0, 0, 0, 0 }, t[4] = { 0, 0, 0, 0 };
    if(renderpath==R_FIXEDFUNCTION) setuptexgen();

    resetorigin();
    vtxarray *prev = NULL;
    for(vtxarray *va = visibleva; va; va = va->next)
    {
        if(!va->texs || va->occluded >= OCCLUDE_GEOM) continue;
        if(va->o.x + va->size <= x || va->o.y + va->size <= y || va->o.x >= x + w || va->o.y >= y + h) continue;

        if(!prev || va->vbuf != prev->vbuf)
        {
            if(setorigin(va))
            {
                s[0] = 1.0f / (w*(1<<VVEC_FRAC));
                s[3] = (vaorigin.x - x) / w;
                t[1] = 1.0f / (h*(1<<VVEC_FRAC));
                t[3] = (vaorigin.y - y) / h;
                if(renderpath==R_FIXEDFUNCTION)
                {
                    glTexGenfv(GL_S, GL_OBJECT_PLANE, s);
                    glTexGenfv(GL_T, GL_OBJECT_PLANE, t);
                }
                else
                {
                    setlocalparamfv("texgenS", SHPARAM_VERTEX, 0, s);
                    setlocalparamfv("texgenT", SHPARAM_VERTEX, 1, t);
                }
            }
        
            if(hasVBO)
            {
                glBindBuffer_(GL_ARRAY_BUFFER_ARB, va->vbuf);
                glBindBuffer_(GL_ELEMENT_ARRAY_BUFFER_ARB, va->ebuf);
            }
            glVertexPointer(3, floatvtx ? GL_FLOAT : GL_SHORT, VTXSIZE, &va->vdata[0].x);
        }

        drawvatris(va, 3*va->tris, va->edata);
        xtravertsva += va->verts;

        prev = va;
    }

    if(renderpath==R_FIXEDFUNCTION) disabletexgen();

    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);

    glDepthFunc(GL_LESS);

    glPopMatrix();

    if(hasVBO)
    {
        glBindBuffer_(GL_ARRAY_BUFFER_ARB, 0);
        glBindBuffer_(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
    }
    glDisableClientState(GL_VERTEX_ARRAY);

    notextureshader->set();
}
 
void rendershadowmapreceivers()
{
    if(!hasBE) return;

    static Shader *shadowmapshader = NULL;
    if(!shadowmapshader) shadowmapshader = lookupshaderbyname("shadowmapreceiver");
    shadowmapshader->set();

    glDisable(GL_TEXTURE_2D);
    glEnableClientState(GL_VERTEX_ARRAY);

    glCullFace(GL_BACK);
    glDepthMask(GL_FALSE);
    glDepthFunc(GL_GREATER);

    extern int ati_minmax_bug;
    if(!ati_minmax_bug) glColorMask(GL_FALSE, GL_FALSE, GL_TRUE, GL_FALSE);

    glEnable(GL_BLEND);
    glBlendEquation_(GL_MAX_EXT);
    glBlendFunc(GL_ONE, GL_ONE);
 
    glPushMatrix();

    resetorigin();
    vtxarray *prev = NULL;
    for(vtxarray *va = visibleva; va; va = va->next)
    {
        if(!va->texs || va->curvfc >= VFC_FOGGED || !isshadowmapreceiver(va)) continue;

        if(!prev || va->vbuf != prev->vbuf)
        {
            setorigin(va);
            if(hasVBO)
            {
                glBindBuffer_(GL_ARRAY_BUFFER_ARB, va->vbuf);
                glBindBuffer_(GL_ELEMENT_ARRAY_BUFFER_ARB, va->ebuf);
            }
            glVertexPointer(3, floatvtx ? GL_FLOAT : GL_SHORT, VTXSIZE, &va->vdata[0].x);
        }

        drawvatris(va, 3*va->tris, va->edata);
        xtravertsva += va->verts;

        prev = va;
    }

    glPopMatrix();

    glDisable(GL_BLEND);
    glBlendEquation_(GL_FUNC_ADD_EXT);

    glCullFace(GL_FRONT);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);
    
    if(!ati_minmax_bug) glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    if(hasVBO)
    {
        glBindBuffer_(GL_ARRAY_BUFFER_ARB, 0);
        glBindBuffer_(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
    }
    glDisableClientState(GL_VERTEX_ARRAY);
    glEnable(GL_TEXTURE_2D);
}

void renderdepthobstacles(const vec &bbmin, const vec &bbmax, float scale, float *ranges, int numranges)
{
    float scales[4] = { 0, 0, 0, 0 }, offsets[4] = { 0, 0, 0, 0 };
    if(numranges < 0)
    {
        SETSHADER(depthfxsplitworld);

        loopi(-numranges)
        {
            if(!i) scales[i] = 1.0f/scale;
            else scales[i] = scales[i-1]*256;
        }
    }
    else
    {
        SETSHADER(depthfxworld);

        if(!numranges) loopi(4) scales[i] = 1.0f/scale;
        else loopi(numranges) 
        {
            scales[i] = 1.0f/scale;
            offsets[i] = -ranges[i]/scale;
        }
    }
    setlocalparamfv("depthscale", SHPARAM_VERTEX, 0, scales);
    setlocalparamfv("depthoffsets", SHPARAM_VERTEX, 1, offsets);

    glDisable(GL_TEXTURE_2D);
    glEnableClientState(GL_VERTEX_ARRAY);

    glPushMatrix();

    resetorigin();
    vtxarray *prev = NULL;
    for(vtxarray *va = visibleva; va; va = va->next)
    {
        if(!va->texs || va->occluded >= OCCLUDE_GEOM || 
           va->o.x > bbmax.x || va->o.y > bbmax.y || va->o.z > bbmax.z ||
           va->o.x + va->size < bbmin.x || va->o.y + va->size < bbmin.y || va->o.z + va->size < bbmin.z)
           continue;

        if(!prev || va->vbuf != prev->vbuf)
        {
            setorigin(va);
            if(hasVBO)
            {
                glBindBuffer_(GL_ARRAY_BUFFER_ARB, va->vbuf);
                glBindBuffer_(GL_ELEMENT_ARRAY_BUFFER_ARB, va->ebuf);
            }
            glVertexPointer(3, floatvtx ? GL_FLOAT : GL_SHORT, VTXSIZE, &va->vdata[0].x);
        }

        drawvatris(va, 3*va->tris, va->edata);
        xtravertsva += va->verts;

        prev = va;
    }

    glPopMatrix();

    if(hasVBO)
    {
        glBindBuffer_(GL_ARRAY_BUFFER_ARB, 0);
        glBindBuffer_(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
    }
    glDisableClientState(GL_VERTEX_ARRAY);
    glEnable(GL_TEXTURE_2D);

    defaultshader->set();
}

// [rotation][dimension] = vec4
float orientation_tangent [6][3][4] =
{
    { { 0,  1,  0, 0 }, {  1, 0,  0, 0 }, {  1,  0, 0, 0 } },
    { { 0,  0, -1, 0 }, {  0, 0, -1, 0 }, {  0,  1, 0, 0 } },
    { { 0, -1,  0, 0 }, { -1, 0,  0, 0 }, { -1,  0, 0, 0 } },
    { { 0,  0,  1, 0 }, {  0, 0,  1, 0 }, {  0, -1, 0, 0 } },
    { { 0, -1,  0, 0 }, { -1, 0,  0, 0 }, { -1,  0, 0, 0 } },
    { { 0,  1,  0, 0 }, {  1, 0,  0, 0 }, {  1,  0, 0, 0 } },
};
float orientation_binormal[6][3][4] =
{
    { { 0,  0, -1, 0 }, {  0, 0, -1, 0 }, {  0,  1, 0, 0 } },
    { { 0, -1,  0, 0 }, { -1, 0,  0, 0 }, { -1,  0, 0, 0 } },
    { { 0,  0,  1, 0 }, {  0, 0,  1, 0 }, {  0, -1, 0, 0 } },
    { { 0,  1,  0, 0 }, {  1, 0,  0, 0 }, {  1,  0, 0, 0 } },
    { { 0,  0, -1, 0 }, {  0, 0, -1, 0 }, {  0,  1, 0, 0 } },
    { { 0,  0,  1, 0 }, {  0, 0,  1, 0 }, {  0, -1, 0, 0 } },
};

struct renderstate
{
    bool colormask, depthmask, blending, mtglow, skippedglow;
    GLuint vbuf;
    float fogplane;
    int diffusetmu, lightmaptmu, glowtmu, fogtmu, causticstmu;
    GLfloat color[4];
    vec glowcolor;
    GLuint textures[8];
    Slot *slot;
    float texgenSk, texgenSoff, texgenTk, texgenToff;
    int texgendim;
    bool mttexgen;
    int visibledynlights;
    uint dynlightmask;
    vec dynlightpos;
    float dynlightradius;

    renderstate() : colormask(true), depthmask(true), blending(false), mtglow(false), skippedglow(false), vbuf(0), fogplane(-1), diffusetmu(0), lightmaptmu(1), glowtmu(-1), fogtmu(-1), causticstmu(-1), glowcolor(1, 1, 1), slot(NULL), texgendim(-1), mttexgen(false), visibledynlights(0), dynlightmask(0)
    {
        loopk(4) color[k] = 1;
        loopk(8) textures[k] = 0;
    }
};

void renderquery(renderstate &cur, occludequery *query, vtxarray *va)
{
    nocolorshader->set();
    if(cur.colormask) { cur.colormask = false; glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE); }
    if(cur.depthmask) { cur.depthmask = false; glDepthMask(GL_FALSE); }

    vec camera(camera1->o);
    if(reflecting) camera.z = reflectz;

    startquery(query);

    drawbb(va->bbmin, ivec(va->bbmax).sub(va->bbmin), camera, vaorigin.x >= 0 ? VVEC_FRAC : 0, vaorigin.x >= 0 ? vaorigin : ivec(0, 0, 0));

    endquery(query);
}

enum
{
    RENDERPASS_LIGHTMAP = 0,
    RENDERPASS_COLOR,
    RENDERPASS_Z,
    RENDERPASS_GLOW,
    RENDERPASS_CAUSTICS,
    RENDERPASS_FOG,
    RENDERPASS_SHADOWMAP,
    RENDERPASS_DYNLIGHT,
    RENDERPASS_LIGHTMAP_BLEND
};

struct geombatch
{
    const elementset &es;
    Slot &slot;
    ushort *edata;
    vtxarray *va;
    int next, batch;

    geombatch(const elementset &es, ushort *edata, vtxarray *va)
      : es(es), slot(lookuptexture(es.texture)), edata(edata), va(va),
        next(-1), batch(-1)
    {}

    int compare(const geombatch &b) const
    {
        if(va->vbuf < b.va->vbuf) return -1;
        if(va->vbuf > b.va->vbuf) return 1;
        if(renderpath!=R_FIXEDFUNCTION)
        {
            if(va->dynlightmask < b.va->dynlightmask) return -1;
            if(va->dynlightmask > b.va->dynlightmask) return 1;
            if(slot.shader < b.slot.shader) return -1;
            if(slot.shader > b.slot.shader) return 1;
            if(slot.params.length() < b.slot.params.length()) return -1;
            if(slot.params.length() > b.slot.params.length()) return 1;
        }
        if(es.texture < b.es.texture) return -1;
        if(es.texture > b.es.texture) return 1;
        if(es.lmid < b.es.lmid) return -1;
        if(es.lmid > b.es.lmid) return 1;
        if(es.envmap < b.es.envmap) return -1;
        if(es.envmap > b.es.envmap) return 1;
        return 0;
    }
};

static vector<geombatch> geombatches;
static int firstbatch = -1, numbatches = 0;

static void mergetexs(vtxarray *va, elementset *texs = NULL, int numtexs = 0, ushort *edata = NULL)
{
    if(!texs) 
    { 
        texs = va->eslist; 
        numtexs = va->texs; 
        edata = va->edata;
    }

    if(firstbatch < 0)
    {
        firstbatch = geombatches.length();
        numbatches = numtexs;
        loopi(numtexs-1) 
        {
            geombatches.add(geombatch(texs[i], edata, va)).next = i+1;
            edata += texs[i].length[5];
        }
        geombatches.add(geombatch(texs[numtexs-1], edata, va));
        return;
    }
    
    int prevbatch = -1, curbatch = firstbatch, curtex = 0;
    do
    {
        geombatch &b = geombatches.add(geombatch(texs[curtex], edata, va));
        edata += texs[curtex].length[5];
        int dir = -1;
        while(curbatch >= 0)
        {
            dir = b.compare(geombatches[curbatch]);
            if(dir <= 0) break;
            prevbatch = curbatch;
            curbatch = geombatches[curbatch].next;
        }
        if(!dir)
        {
            int last = curbatch, next;
            for(;;)
            {
                next = geombatches[last].batch;
                if(next < 0) break;
                last = next;
            }
            if(last==curbatch)
            {
                b.batch = curbatch;
                b.next = geombatches[curbatch].next;
                if(prevbatch < 0) firstbatch = geombatches.length()-1;
                else geombatches[prevbatch].next = geombatches.length()-1;
                curbatch = geombatches.length()-1;
            }
            else
            {
                b.batch = next;
                geombatches[last].batch = geombatches.length()-1;
            }    
        }
        else 
        {
            numbatches++;
            b.next = curbatch;
            if(prevbatch < 0) firstbatch = geombatches.length()-1;
            else geombatches[prevbatch].next = geombatches.length()-1;
            prevbatch = geombatches.length()-1;
        }
    }
    while(++curtex < numtexs);
}

static void mergeglowtexs(vtxarray *va)
{
    int start = -1;
    ushort *edata = va->edata, *startdata = NULL;
    loopi(va->texs)
    {
        elementset &es = va->eslist[i];
        Slot &slot = lookuptexture(es.texture, false);
        if(slot.texmask&(1<<TEX_GLOW) && !slot.mtglowed)
        {
            if(start<0) { start = i; startdata = edata; }
        }
        else if(start>=0)
        {
            mergetexs(va, &va->eslist[start], i-start, startdata);
            start = -1;
        }
        edata += es.length[5];
    }
    if(start>=0) mergetexs(va, &va->eslist[start], va->texs-start, startdata);
}

static void changefogplane(renderstate &cur, int pass, vtxarray *va)
{
    if(renderpath!=R_FIXEDFUNCTION)
    {
        if((fading && !cur.blending) || fogging)
        {
            float fogplane = reflectz - vaorigin.z;
            if(cur.fogplane!=fogplane)
            {
                cur.fogplane = fogplane;
                if(fogging) setfogplane(1.0f/(1<<VVEC_FRAC), fogplane, false, -0.25f/(1<<VVEC_FRAC), 0.5f + 0.25f*fogplane);
                else setfogplane(0, 0, false, 0.25f/(1<<VVEC_FRAC), 0.5f - 0.25f*fogplane);
            }
        }
    }
    else if(pass==RENDERPASS_FOG || (cur.fogtmu>=0 && (pass==RENDERPASS_LIGHTMAP || pass==RENDERPASS_GLOW || pass==RENDERPASS_SHADOWMAP)))
    {
        if(pass==RENDERPASS_LIGHTMAP) glActiveTexture_(GL_TEXTURE0_ARB+cur.fogtmu);
        else if(pass==RENDERPASS_GLOW || pass==RENDERPASS_SHADOWMAP) glActiveTexture_(GL_TEXTURE1_ARB);
        GLfloat s[4] = { 0, 0, -1.0f/(waterfog<<VVEC_FRAC), (reflectz - vaorigin.z)/waterfog };
        glTexGenfv(GL_S, GL_OBJECT_PLANE, s);
        if(pass==RENDERPASS_LIGHTMAP) glActiveTexture_(GL_TEXTURE0_ARB+cur.diffusetmu);
        else if(pass==RENDERPASS_GLOW || pass==RENDERPASS_SHADOWMAP) glActiveTexture_(GL_TEXTURE0_ARB);
    }
}

static void changedynlightpos(renderstate &cur, vtxarray *va)
{
    GLfloat tx[4] = { 0.5f/(cur.dynlightradius * (1<<VVEC_FRAC)), 0, 0, 0.5f + 0.5f*(vaorigin.x - cur.dynlightpos.x)/cur.dynlightradius },
            ty[4] = { 0, 0.5f/(cur.dynlightradius * (1<<VVEC_FRAC)), 0, 0.5f + 0.5f*(vaorigin.y - cur.dynlightpos.y)/cur.dynlightradius },
            tz[4] = { 0, 0, 0.5f/(cur.dynlightradius * (1<<VVEC_FRAC)), 0.5f + 0.5f*(vaorigin.z - cur.dynlightpos.z)/cur.dynlightradius };
    glActiveTexture_(GL_TEXTURE0_ARB);
    glTexGenfv(GL_S, GL_OBJECT_PLANE, tx);
    glTexGenfv(GL_T, GL_OBJECT_PLANE, ty);
    glActiveTexture_(GL_TEXTURE1_ARB);
    glTexGenfv(GL_S, GL_OBJECT_PLANE, tz);
    glActiveTexture_(GL_TEXTURE2_ARB);
}
    
static void changevbuf(renderstate &cur, int pass, vtxarray *va)
{
    if(setorigin(va, renderpath!=R_FIXEDFUNCTION ? pass==RENDERPASS_LIGHTMAP && !envmapping && !glaring : pass==RENDERPASS_SHADOWMAP))
    {
        cur.visibledynlights = 0;
        cur.dynlightmask = 0;
    }
    if(hasVBO)
    {
        glBindBuffer_(GL_ARRAY_BUFFER_ARB, va->vbuf);
        glBindBuffer_(GL_ELEMENT_ARRAY_BUFFER_ARB, va->ebuf);
    }
    cur.vbuf = va->vbuf;

    glVertexPointer(3, floatvtx ? GL_FLOAT : GL_SHORT, VTXSIZE, &va->vdata[0].x);

    if(pass==RENDERPASS_LIGHTMAP)
    {
        glClientActiveTexture_(GL_TEXTURE0_ARB+cur.lightmaptmu);
        glTexCoordPointer(2, GL_SHORT, VTXSIZE, floatvtx ? &((fvertex *)va->vdata)[0].u : &va->vdata[0].u);
        glClientActiveTexture_(GL_TEXTURE0_ARB+cur.diffusetmu);

        if(renderpath!=R_FIXEDFUNCTION)
        {
            glColorPointer(3, GL_UNSIGNED_BYTE, VTXSIZE, floatvtx ? &((fvertex *)va->vdata)[0].n : &va->vdata[0].n);
            setenvparamf("camera", SHPARAM_VERTEX, 4,
                (camera1->o.x - vaorigin.x)*(1<<VVEC_FRAC),
                (camera1->o.y - vaorigin.y)*(1<<VVEC_FRAC),
                (camera1->o.z - vaorigin.z)*(1<<VVEC_FRAC),
                1);
        }
    }
}

static void changebatchtmus(renderstate &cur, int pass, geombatch &b)
{
    bool changed = false;
    extern bool brightengeom;
    extern int fullbright;
    int lmid = brightengeom && (b.es.lmid < LMID_RESERVED || (fullbright && editmode)) ? LMID_BRIGHT : b.es.lmid; 
    if(cur.textures[cur.lightmaptmu]!=lightmaptexs[lmid].id)
    {
        glActiveTexture_(GL_TEXTURE0_ARB+cur.lightmaptmu);
        glBindTexture(GL_TEXTURE_2D, cur.textures[cur.lightmaptmu] = lightmaptexs[lmid].id);
        changed = true;
    }
    if(renderpath!=R_FIXEDFUNCTION)
    {
        int tmu = cur.lightmaptmu+1;
        if(b.slot.shader->type&SHADER_NORMALSLMS)
        {
            if(lmid+1 >= 0 && lmid+1 < lightmaptexs.length() && cur.textures[tmu]!=lightmaptexs[lmid+1].id) // INTENSITY: Checks
            {
                glActiveTexture_(GL_TEXTURE0_ARB+tmu);
                glBindTexture(GL_TEXTURE_2D, cur.textures[tmu] = lightmaptexs[lmid+1].id);
                changed = true;
            }
            tmu++;
        }
        if(b.slot.shader->type&SHADER_ENVMAP && b.es.envmap!=EMID_CUSTOM)
        {
            GLuint emtex = lookupenvmap(b.es.envmap);
            if(cur.textures[tmu]!=emtex)
            {
                glActiveTexture_(GL_TEXTURE0_ARB+tmu);
                glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, cur.textures[tmu] = emtex);
                changed = true;
            }
        }
    }
    if(changed) glActiveTexture_(GL_TEXTURE0_ARB+cur.diffusetmu);
}

static void changeglow(renderstate &cur, int pass, Slot &slot)
{
    vec color = slot.glowcolor;
    if(slot.pulseglowspeed)
    {
        float k = lastmillis*slot.pulseglowspeed;
        k -= floor(k);
        k = fabs(k*2 - 1);
        color.lerp(color, slot.pulseglowcolor, k);
    }
    if(pass==RENDERPASS_GLOW)
    {
        if(cur.glowcolor!=color) glColor3fv(color.v);
    }
    else 
    {
        if(cur.glowcolor!=color)
        {
            if(color==vec(1, 1, 1)) 
            {
                glActiveTexture_(GL_TEXTURE0_ARB+cur.glowtmu);
                setuptmu(cur.glowtmu, "P + T", "= Pa");
            }
            else if(hasTE3 || hasTE4)
            {
                glActiveTexture_(GL_TEXTURE0_ARB+cur.glowtmu);
                if(cur.glowcolor==vec(1, 1, 1))
                {
                    if(hasTE3) setuptmu(cur.glowtmu, "TPK3", "= Pa");
                    else if(hasTE4) setuptmu(cur.glowtmu, "TKP14", "= Pa");
                }
                colortmu(cur.glowtmu, color.x, color.y, color.z);
            }
            else
            {
                slot.mtglowed = false;
                cur.skippedglow = true;
                return;
            }
        }
        else glActiveTexture_(GL_TEXTURE0_ARB+cur.glowtmu);
        if(!cur.mtglow) { glEnable(GL_TEXTURE_2D); cur.mtglow = true; }
        slot.mtglowed = true;
    }
    loopvj(slot.sts)
    {
        Slot::Tex &t = slot.sts[j];
        if(t.type==TEX_GLOW && t.combined<0)
        {
            if(cur.textures[cur.glowtmu]!=t.t->id)
                glBindTexture(GL_TEXTURE_2D, cur.textures[cur.glowtmu] = t.t->id);
            break;
        }
    }
    cur.glowcolor = color;
}

static void changeslottmus(renderstate &cur, int pass, Slot &slot)
{
    if(pass==RENDERPASS_LIGHTMAP || pass==RENDERPASS_COLOR || pass==RENDERPASS_DYNLIGHT) 
    {
        GLuint diffusetex = slot.sts.empty() ? notexture->id : slot.sts[0].t->id;
        if(cur.textures[cur.diffusetmu]!=diffusetex)
            glBindTexture(GL_TEXTURE_2D, cur.textures[cur.diffusetmu] = diffusetex);
    }

    if(renderpath==R_FIXEDFUNCTION)
    {
        if(slot.texmask&(1<<TEX_GLOW))
        {
            if(pass==RENDERPASS_LIGHTMAP || pass==RENDERPASS_COLOR)
            {
                if(cur.glowtmu<0) { slot.mtglowed = false; cur.skippedglow = true; }
                else changeglow(cur, pass, slot);
            }
            else if(pass==RENDERPASS_GLOW && !slot.mtglowed) changeglow(cur, pass, slot);
        }
        if(cur.mtglow)
        {
            if(!(slot.texmask&(1<<TEX_GLOW)) || !slot.mtglowed) 
            { 
                glActiveTexture_(GL_TEXTURE0_ARB+cur.glowtmu); 
                glDisable(GL_TEXTURE_2D);
                cur.mtglow = false;
            }
            glActiveTexture_(GL_TEXTURE0_ARB+cur.diffusetmu);
        }
    }
    else
    {
        int tmu = cur.lightmaptmu+1, envmaptmu = -1;
        if(slot.shader->type&SHADER_NORMALSLMS) tmu++;
        if(slot.shader->type&SHADER_ENVMAP) envmaptmu = tmu++;
        loopvj(slot.sts)
        {
            Slot::Tex &t = slot.sts[j];
            if(t.type==TEX_DIFFUSE || t.combined>=0) continue;
            if(t.type==TEX_ENVMAP)
            {
                if(envmaptmu>=0 && cur.textures[envmaptmu]!=t.t->id)
                {
                    glActiveTexture_(GL_TEXTURE0_ARB+envmaptmu);
                    glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, cur.textures[envmaptmu] = t.t->id);
                }
                continue;
            }
            else if(cur.textures[tmu]!=t.t->id)
            {  
                glActiveTexture_(GL_TEXTURE0_ARB+tmu);
                glBindTexture(GL_TEXTURE_2D, cur.textures[tmu] = t.t->id);
            }
            tmu++;
        }
        glActiveTexture_(GL_TEXTURE0_ARB+cur.diffusetmu);
    } 

    Texture *curtex = !cur.slot || cur.slot->sts.empty() ? notexture : cur.slot->sts[0].t,
            *tex = slot.sts.empty() ? notexture : slot.sts[0].t;
    if(!cur.slot || slot.sts.empty() ||
        (curtex->xs != tex->xs || curtex->ys != tex->ys || 
         cur.slot->rotation != slot.rotation || cur.slot->scale != slot.scale || 
         cur.slot->xoffset != slot.xoffset || cur.slot->yoffset != slot.yoffset ||
         cur.slot->scrollS != slot.scrollS || cur.slot->scrollT != slot.scrollT))
    {
        float k = 8.0f/slot.scale/(1<<VVEC_FRAC),
              xs = slot.rotation>=2 && slot.rotation<=4 ? -tex->xs : tex->xs, 
              ys = (slot.rotation>=1 && slot.rotation<=2) || slot.rotation==5 ? -tex->ys : tex->ys;
        if((slot.rotation&5)==1)
        {
            cur.texgenSk = k/xs; cur.texgenSoff = (slot.scrollT*lastmillis*tex->xs - slot.yoffset)/xs;
            cur.texgenTk = k/ys; cur.texgenToff = (slot.scrollS*lastmillis*tex->ys - slot.xoffset)/ys;
        }
        else
        {
            cur.texgenSk = k/xs; cur.texgenSoff = (slot.scrollS*lastmillis*tex->xs - slot.xoffset)/xs;
            cur.texgenTk = k/ys; cur.texgenToff = (slot.scrollT*lastmillis*tex->ys - slot.yoffset)/ys;
        }
        cur.texgendim = -1;
    }

    cur.slot = &slot;
}

static void changeshader(renderstate &cur, Shader *s, Slot &slot, bool shadowed)
{
    if(glaring)
    {
        static Shader *noglareshader = NULL, *noglareblendshader = NULL;
        if(!noglareshader) noglareshader = lookupshaderbyname("noglareworld");
        if(!noglareblendshader) noglareblendshader = lookupshaderbyname("noglareblendworld");
        if(s->hasoption(4)) s->setvariant(cur.visibledynlights, 4, &slot, cur.blending ? noglareblendshader : noglareshader);
        else s->setvariant(cur.blending ? 1 : 0, 4, &slot, cur.blending ? noglareblendshader : noglareshader);
    }
    else if(fading && !cur.blending)
    {
        if(shadowed) s->setvariant(cur.visibledynlights, 3, &slot);
        else s->setvariant(cur.visibledynlights, 2, &slot);
    }
    else if(shadowed) s->setvariant(cur.visibledynlights, 1, &slot);
    else if(!cur.visibledynlights) s->set(&slot);
    else s->setvariant(cur.visibledynlights-1, 0, &slot);
    if(s->type&SHADER_GLSLANG) cur.texgendim = -1;
}

static void changetexgen(renderstate &cur, Slot &slot, int dim)
{
    static const int si[] = { 1, 0, 0 };
    static const int ti[] = { 2, 2, 1 };

    GLfloat sgen[4] = { 0.0f, 0.0f, 0.0f, cur.texgenSoff },
            tgen[4] = { 0.0f, 0.0f, 0.0f, cur.texgenToff };
    int sdim = si[dim], tdim = ti[dim];
    if((slot.rotation&5)==1)
    {
        sgen[tdim] = (dim <= 1 ? -cur.texgenSk : cur.texgenSk);
        sgen[3] += (vaorigin[tdim]<<VVEC_FRAC)*sgen[tdim];
        tgen[sdim] = cur.texgenTk;
        tgen[3] += (vaorigin[sdim]<<VVEC_FRAC)*tgen[sdim];
    }
    else
    {
        sgen[sdim] = cur.texgenSk;
        sgen[3] += (vaorigin[sdim]<<VVEC_FRAC)*sgen[sdim];
        tgen[tdim] = (dim <= 1 ? -cur.texgenTk : cur.texgenTk);
        tgen[3] += (vaorigin[tdim]<<VVEC_FRAC)*tgen[tdim];
    }

    if(renderpath==R_FIXEDFUNCTION)
    {
        if(cur.texgendim!=dim)
        {
            glTexGenfv(GL_S, GL_OBJECT_PLANE, sgen);
            glTexGenfv(GL_T, GL_OBJECT_PLANE, tgen);
            // KLUGE: workaround for buggy nvidia drivers
            // object planes are somehow invalid unless texgen is toggled
            extern int nvidia_texgen_bug;
            if(nvidia_texgen_bug)
            {
                glDisable(GL_TEXTURE_GEN_S);
                glDisable(GL_TEXTURE_GEN_T);
                glEnable(GL_TEXTURE_GEN_S);
                glEnable(GL_TEXTURE_GEN_T);
            }
        }

        if(cur.mtglow)
        {
            glActiveTexture_(GL_TEXTURE0_ARB+cur.glowtmu);
            glTexGenfv(GL_S, GL_OBJECT_PLANE, sgen);
            glTexGenfv(GL_T, GL_OBJECT_PLANE, tgen);
            glActiveTexture_(GL_TEXTURE0_ARB+cur.diffusetmu);
        }
        cur.mttexgen = cur.mtglow;
    }
    else
    {
        // have to pass in env, otherwise same problem as fixed function
        setlocalparamfv("texgenS", SHPARAM_VERTEX, 0, sgen);
        setlocalparamfv("texgenT", SHPARAM_VERTEX, 1, tgen);
        setlocalparamfv("orienttangent", SHPARAM_VERTEX, 2, orientation_tangent[slot.rotation][dim]);
        setlocalparamfv("orientbinormal", SHPARAM_VERTEX, 3, orientation_binormal[slot.rotation][dim]);
    }

    cur.texgendim = dim;
}

struct batchdrawinfo
{
    ushort *edata;
    ushort len, minvert, maxvert;

    batchdrawinfo(geombatch &b, int dim, ushort offset, ushort len)
      : edata(b.edata + offset), len(len), 
        minvert(b.va->shadowed ? b.es.minvert[dim] : min(b.es.minvert[dim], b.es.minvert[dim+1])), 
        maxvert(b.va->shadowed ? b.es.maxvert[dim] : max(b.es.maxvert[dim], b.es.maxvert[dim+1]))
    {}
};

static void renderbatch(renderstate &cur, int pass, geombatch &b)
{
    static vector<batchdrawinfo> draws[6];
    for(geombatch *curbatch = &b;; curbatch = &geombatches[curbatch->batch])
    {
        int dim = 0;
        ushort offset = 0, len = 0;
        loopi(3)
        {
            offset += len;
            len = curbatch->es.length[dim + (curbatch->va->shadowed ? 0 : 1)] - offset;
            if(len) draws[dim].add(batchdrawinfo(*curbatch, dim, offset, len));
            dim++;
   
            if(curbatch->va->shadowed)
            {    
                offset += len;
                len = curbatch->es.length[dim] - offset;
                if(len) draws[dim].add(batchdrawinfo(*curbatch, dim, offset, len));
            }
            dim++;
        }
        if(curbatch->batch < 0) break;
    }
    loop(shadowed, 2) 
    {
        bool rendered = false;
        loop(dim, 3)
        {
            vector<batchdrawinfo> &draw = draws[2*dim + shadowed];
            if(draw.empty()) continue;

            if(!rendered)
            {
                if(renderpath!=R_FIXEDFUNCTION) changeshader(cur, b.slot.shader, b.slot, shadowed!=0);
                rendered = true;
            }
            if(cur.texgendim!=dim || cur.mtglow>cur.mttexgen)
                changetexgen(cur, b.slot, dim);

            gbatches++;
            loopv(draw)
            {
                batchdrawinfo &info = draw[i];
                drawtris(info.len, info.edata, info.minvert, info.maxvert);
                vtris += info.len/3;
            }
            draw.setsizenodelete(0);
        }
    }
}

static void resetbatches()
{
    geombatches.setsizenodelete(0);
    firstbatch = -1;
    numbatches = 0;
}

static void renderbatches(renderstate &cur, int pass)
{
    cur.slot = NULL;
    int curbatch = firstbatch;
    if(curbatch >= 0)
    {
        if(!cur.depthmask) { cur.depthmask = true; glDepthMask(GL_TRUE); }
        if(!cur.colormask) { cur.colormask = true; glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE); }
    }        
    while(curbatch >= 0)
    {
        geombatch &b = geombatches[curbatch];
        curbatch = b.next;

        if(cur.vbuf != b.va->vbuf) 
        {
            changevbuf(cur, pass, b.va);
            if(pass == RENDERPASS_DYNLIGHT) changedynlightpos(cur, b.va);
            else changefogplane(cur, pass, b.va);
        }
        if(pass == RENDERPASS_LIGHTMAP) 
        {
            changebatchtmus(cur, pass, b);
            if(cur.dynlightmask != b.va->dynlightmask)
            {
                cur.visibledynlights = setdynlights(b.va, vaorigin);
                cur.dynlightmask = b.va->dynlightmask;
            }
        }
        if(cur.slot != &b.slot) changeslottmus(cur, pass, b.slot);   

        renderbatch(cur, pass, b);
    }

    if(pass == RENDERPASS_LIGHTMAP && cur.mtglow)
    {
        glActiveTexture_(GL_TEXTURE0_ARB+cur.glowtmu);
        glDisable(GL_TEXTURE_2D);
        glActiveTexture_(GL_TEXTURE0_ARB+cur.diffusetmu);
        cur.mtglow = false; 
    }

    resetbatches();
}

void renderzpass(renderstate &cur, vtxarray *va)
{
    if(cur.vbuf!=va->vbuf) changevbuf(cur, RENDERPASS_Z, va);
    if(!cur.depthmask) { cur.depthmask = true; glDepthMask(GL_TRUE); }
    if(cur.colormask) { cur.colormask = false; glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE); }

    extern int apple_glsldepth_bug;
    if(renderpath!=R_GLSLANG || !apple_glsldepth_bug)
    {
        nocolorshader->set();
        drawvatris(va, 3*va->tris, va->edata);
    }
    else
    {
        static Shader *nocolorglslshader = NULL;
        if(!nocolorglslshader) nocolorglslshader = lookupshaderbyname("nocolorglsl");
        Slot *lastslot = NULL;
        int lastdraw = 0, offset = 0;
        loopi(va->texs)
        {
            Slot &slot = lookuptexture(va->eslist[i].texture);
            if(lastslot && (slot.shader->type&SHADER_GLSLANG) != (lastslot->shader->type&SHADER_GLSLANG) && offset > lastdraw)
            {
                (lastslot->shader->type&SHADER_GLSLANG ? nocolorglslshader : nocolorshader)->set();
                drawvatris(va, offset-lastdraw, va->edata+lastdraw);
                lastdraw = offset;
            }
            lastslot = &slot;
            offset += va->eslist[i].length[5];
        }
        if(offset > lastdraw)
        {
            (lastslot->shader->type&SHADER_GLSLANG ? nocolorglslshader : nocolorshader)->set();
            drawvatris(va, offset-lastdraw, va->edata+lastdraw);
        }
    }
    xtravertsva += va->verts;
}

vector<vtxarray *> foggedvas;

#define startvaquery(va, flush) \
    do { \
        if(!refracting) \
        { \
            occludequery *query = reflecting ? va->rquery : va->query; \
            if(query) \
            { \
                flush; \
                startquery(query); \
            } \
        } \
    } while(0)


#define endvaquery(va, flush) \
    do { \
        if(!refracting) \
        { \
            occludequery *query = reflecting ? va->rquery : va->query; \
            if(query) \
            { \
                flush; \
                endquery(query); \
            } \
        } \
    } while(0)

void renderfoggedvas(renderstate &cur, bool doquery = false)
{
    static Shader *fogshader = NULL;
    if(!fogshader) fogshader = lookupshaderbyname("fogworld");
    fogshader->set();

    glDisable(GL_TEXTURE_2D);
        
    glColor3ubv(watercolor.v);

    loopv(foggedvas)
    {
        vtxarray *va = foggedvas[i];
        if(cur.vbuf!=va->vbuf) changevbuf(cur, RENDERPASS_FOG, va);

        if(doquery) startvaquery(va, );
        drawvatris(va, 3*va->tris, va->edata);
        vtris += va->tris;
        if(doquery) endvaquery(va, );
    }

    glEnable(GL_TEXTURE_2D);

    foggedvas.setsizenodelete(0);
}

void rendershadowmappass(renderstate &cur, vtxarray *va)
{
    if(cur.vbuf!=va->vbuf) 
    {
        changevbuf(cur, RENDERPASS_SHADOWMAP, va);
        changefogplane(cur, RENDERPASS_SHADOWMAP, va);
    }

    elementset *texs = va->eslist;
    ushort *edata = va->edata;
    loopi(va->texs)
    {
        elementset &es = texs[i];
        int len = es.length[1] - es.length[0];
        if(len > 0) 
        {
            drawtris(len, &edata[es.length[0]], es.minvert[1], es.maxvert[1]);
            vtris += len/3;
        }
        len = es.length[3] - es.length[2];
        if(len > 0) 
        {
            drawtris(len, &edata[es.length[2]], es.minvert[3], es.maxvert[3]);
            vtris += len/3;
        }
        len = es.length[5] - es.length[4];
        if(len > 0) 
        {
            drawtris(len, &edata[es.length[4]], es.minvert[5], es.maxvert[5]);
            vtris += len/3;
        }
        edata += es.length[5];
    }
}

VAR(batchgeom, 0, 1, 1);

void renderva(renderstate &cur, vtxarray *va, int pass = RENDERPASS_LIGHTMAP, bool fogpass = false, bool doquery = false)
{
    switch(pass)
    {
        case RENDERPASS_GLOW:
            if(!(va->texmask&(1<<TEX_GLOW))) return;
            mergeglowtexs(va);
            if(!batchgeom && geombatches.length()) renderbatches(cur, pass);
            break;

        case RENDERPASS_COLOR:
        case RENDERPASS_LIGHTMAP:
            vverts += va->verts;
            va->shadowed = false;
            va->dynlightmask = 0;
            if(fogpass ? va->geommax.z<=reflectz-waterfog : va->curvfc==VFC_FOGGED)
            {
                foggedvas.add(va);
                break;
            }
            if(renderpath!=R_FIXEDFUNCTION && !envmapping && !glaring)
            {
                va->shadowed = isshadowmapreceiver(va);
                calcdynlightmask(va);
            }
            if(doquery) startvaquery(va, { if(geombatches.length()) renderbatches(cur, pass); });
            mergetexs(va);
            if(doquery) endvaquery(va, { if(geombatches.length()) renderbatches(cur, pass); });
            else if(!batchgeom && geombatches.length()) renderbatches(cur, pass);
            break;

        case RENDERPASS_LIGHTMAP_BLEND:
        {
            if(doquery) startvaquery(va, { if(geombatches.length()) renderbatches(cur, RENDERPASS_LIGHTMAP); });
            ushort *edata = va->edata;
            loopi(va->texs) edata += va->eslist[i].length[5];
            mergetexs(va, &va->eslist[va->texs], va->blends, edata);
            if(doquery) endvaquery(va, { if(geombatches.length()) renderbatches(cur, RENDERPASS_LIGHTMAP); });
            else if(!batchgeom && geombatches.length()) renderbatches(cur, RENDERPASS_LIGHTMAP);
            break;
        }

        case RENDERPASS_DYNLIGHT:
            if(cur.dynlightpos.dist_to_bb(va->geommin, va->geommax) >= cur.dynlightradius) break;
            vverts += va->verts;
            mergetexs(va);
            if(!batchgeom && geombatches.length()) renderbatches(cur, pass);
            break;

        case RENDERPASS_FOG:
            if(cur.vbuf!=va->vbuf)
            {
                changevbuf(cur, pass, va);
                changefogplane(cur, pass, va);
            }
            drawvatris(va, 3*va->tris, va->edata);
            xtravertsva += va->verts;
            break;

        case RENDERPASS_SHADOWMAP:
            if(isshadowmapreceiver(va)) rendershadowmappass(cur, va);
            break;

        case RENDERPASS_CAUSTICS:
            if(cur.vbuf!=va->vbuf) changevbuf(cur, pass, va);
            drawvatris(va, 3*va->tris, va->edata);
            xtravertsva += va->verts;
            break;
 
        case RENDERPASS_Z:
            if(doquery) startvaquery(va, );
            renderzpass(cur, va);
            if(doquery) endvaquery(va, );
            break;
    }
}

VAR(oqdist, 0, 256, 1024);
VAR(zpass, 0, 1, 1);
VAR(glowpass, 0, 1, 1);

GLuint fogtex = 0;

void createfogtex()
{
    extern int bilinear;
    uchar buf[2*256] = { 255, 0, 255, 255 };
    if(!bilinear) loopi(256) { buf[2*i] = 255; buf[2*i+1] = i; }
    glGenTextures(1, &fogtex);
    createtexture(fogtex, bilinear ? 2 : 256, 1, buf, 3, 1, GL_LUMINANCE_ALPHA, GL_TEXTURE_1D);
}

GLuint attenxytex = 0, attenztex = 0;

static GLuint createattenxytex(int size)
{
    uchar *data = new uchar[size*size], *dst = data;
    loop(y, size) loop(x, size)
    {
        float dx = 2*float(x)/(size-1) - 1, dy = 2*float(y)/(size-1) - 1;
        float atten = max(0.0f, 1.0f - dx*dx - dy*dy);
        *dst++ = uchar(atten*255);
    }
    GLuint tex = 0;
    glGenTextures(1, &tex);
    createtexture(tex, size, size, data, 3, 1, GL_ALPHA);
    delete[] data;
    return tex;
}

static GLuint createattenztex(int size)
{
    uchar *data = new uchar[size], *dst = data;
    loop(z, size) 
    {
        float dz = 2*float(z)/(size-1) - 1;
        float atten = dz*dz;
        *dst++ = uchar(atten*255);
    }
    GLuint tex = 0;
    glGenTextures(1, &tex);
    createtexture(tex, size, 1, data, 3, 1, GL_ALPHA, GL_TEXTURE_1D);
    delete[] data;
    return tex;
}

#define NUMCAUSTICS 32

static Texture *caustictex[NUMCAUSTICS] = { NULL };

void loadcaustics(bool force)
{
    static bool needcaustics = false;
    if(force) needcaustics = true;
    if(!caustics || !needcaustics) return;
    useshaderbyname("caustic");
    if(caustictex[0]) return;
    loopi(NUMCAUSTICS)
    {
        defformatstring(name)(
            renderpath==R_FIXEDFUNCTION ? 
                "<mad:0.6,0.4>packages/caustics/caust%.2d.png" :
                "<mad:-0.6,0.6>packages/caustics/caust%.2d.png",
            i);
        caustictex[i] = textureload(name);
    }
}

void cleanupva()
{
    clearvas(worldroot);
    clearqueries();
    if(fogtex) { glDeleteTextures(1, &fogtex); fogtex = 0; }
    if(attenxytex) { glDeleteTextures(1, &attenxytex); attenxytex = 0; }
    if(attenztex) { glDeleteTextures(1, &attenztex); attenztex = 0; }
    loopi(NUMCAUSTICS) caustictex[i] = NULL;
}

VARR(causticscale, 0, 100, 10000);
VARR(causticmillis, 0, 75, 1000);
VARFP(caustics, 0, 1, 1, loadcaustics());

void setupcaustics(int tmu, float blend, GLfloat *color = NULL)
{
    GLfloat s[4] = { 0.011f, 0, 0.0066f, 0 };
    GLfloat t[4] = { 0, 0.011f, 0.0066f, 0 };
    loopk(3)
    {
        s[k] *= 100.0f/(causticscale<<VVEC_FRAC);
        t[k] *= 100.0f/(causticscale<<VVEC_FRAC);
    }
    int tex = (lastmillis/causticmillis)%NUMCAUSTICS;
    float frac = float(lastmillis%causticmillis)/causticmillis;
    if(color) color[3] = frac;
    else glColor4f(1, 1, 1, frac);
    loopi(2)
    {
        glActiveTexture_(GL_TEXTURE0_ARB+tmu+i);
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, caustictex[(tex+i)%NUMCAUSTICS]->id);
        if(renderpath==R_FIXEDFUNCTION)
        {
            setuptexgen();
            setuptmu(tmu+i, !i ? "= T" : "T , P @ Ca");
            glTexGenfv(GL_S, GL_OBJECT_PLANE, s);
            glTexGenfv(GL_T, GL_OBJECT_PLANE, t);
        }
    }
    if(renderpath!=R_FIXEDFUNCTION)
    {
        static Shader *causticshader = NULL;
        if(!causticshader) causticshader = lookupshaderbyname("caustic");
        causticshader->set();
        setlocalparamfv("texgenS", SHPARAM_VERTEX, 0, s);
        setlocalparamfv("texgenT", SHPARAM_VERTEX, 1, t);
        setlocalparamf("frameoffset", SHPARAM_PIXEL, 0, blend*(1-frac), blend*frac, blend);
    }
}

void setupTMUs(renderstate &cur, float causticspass, bool fogpass)
{
    if(!reflecting && !refracting && !envmapping && shadowmap && hasFBO)
    {
        glDisableClientState(GL_VERTEX_ARRAY);

        if(hasVBO)
        {
            glBindBuffer_(GL_ARRAY_BUFFER_ARB, 0);
            glBindBuffer_(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
        }

        rendershadowmap();

        glEnableClientState(GL_VERTEX_ARRAY);
    }

    if(renderpath==R_FIXEDFUNCTION)
    {
        if(nolights) cur.lightmaptmu = -1;
        else if(maxtmus>=3)
        {
            if(maxtmus>=4 && causticspass>=1)
            {
                cur.causticstmu = 0;
                cur.diffusetmu = 2;
                cur.lightmaptmu = 3;
                if(maxtmus>=5)
                {
                    if(fogpass) 
                    {
                        if(glowpass && maxtmus>=6) 
                        { 
                            cur.fogtmu = 5;
                            cur.glowtmu = 4;
                        }
                        else cur.fogtmu = 4;
                    } 
                    else if(glowpass) cur.glowtmu = 4;
                }
            }
            else if(fogpass && causticspass<1) cur.fogtmu = 2;
            else if(glowpass) cur.glowtmu = 2;
        }
        if(cur.glowtmu>=0)
        {
            glActiveTexture_(GL_TEXTURE0_ARB+cur.glowtmu);
            setuptexgen();
            setuptmu(cur.glowtmu, "P + T", "= Pa");
        }
        if(cur.fogtmu>=0)
        {
            glActiveTexture_(GL_TEXTURE0_ARB+cur.fogtmu);
            glEnable(GL_TEXTURE_1D);
            setuptexgen(1);
            setuptmu(cur.fogtmu, "C , P @ Ta", "= Pa");
            if(!fogtex) createfogtex();
            glBindTexture(GL_TEXTURE_1D, fogtex);
            loopk(3) cur.color[k] = watercolor[k]/255.0f;
        }
        if(cur.causticstmu>=0) setupcaustics(cur.causticstmu, causticspass, cur.color);
    }
    else
    {
        // need to invalidate vertex params in case they were used somewhere else for streaming params
        invalidateenvparams(SHPARAM_VERTEX, 10, RESERVEDSHADERPARAMS + MAXSHADERPARAMS - 10);
        glEnableClientState(GL_COLOR_ARRAY);
        loopi(8-2) { glActiveTexture_(GL_TEXTURE2_ARB+i); glEnable(GL_TEXTURE_2D); }
        glActiveTexture_(GL_TEXTURE0_ARB);
        setenvparamf("ambient", SHPARAM_PIXEL, 5, ambientcolor[0]/255.0f, ambientcolor[1]/255.0f, ambientcolor[2]/255.0f);
        setenvparamf("millis", SHPARAM_VERTEX, 6, lastmillis/1000.0f, lastmillis/1000.0f, lastmillis/1000.0f);
    }
 
    glColor4fv(cur.color);

    if(cur.lightmaptmu>=0)
    {
        glActiveTexture_(GL_TEXTURE0_ARB+cur.lightmaptmu);
        glClientActiveTexture_(GL_TEXTURE0_ARB+cur.lightmaptmu);

        setuptmu(cur.lightmaptmu, "P * T x 2", "= Ta");
        glEnable(GL_TEXTURE_2D);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glMatrixMode(GL_TEXTURE);
        glLoadIdentity();
        glScalef(1.0f/SHRT_MAX, 1.0f/SHRT_MAX, 1.0f);
        glMatrixMode(GL_MODELVIEW);

        glActiveTexture_(GL_TEXTURE0_ARB+cur.diffusetmu);
        glClientActiveTexture_(GL_TEXTURE0_ARB+cur.diffusetmu);
        glEnable(GL_TEXTURE_2D); 
        setuptmu(cur.diffusetmu, cur.diffusetmu>0 ? "P * T" : "= T");
    }

    if(renderpath==R_FIXEDFUNCTION) setuptexgen();
}

void cleanupTMUs(renderstate &cur)
{
    if(cur.lightmaptmu>=0)
    {
        glActiveTexture_(GL_TEXTURE0_ARB+cur.lightmaptmu);
        glClientActiveTexture_(GL_TEXTURE0_ARB+cur.lightmaptmu);

        resettmu(cur.lightmaptmu);
        glDisable(GL_TEXTURE_2D);
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        glMatrixMode(GL_TEXTURE);
        glLoadIdentity();
        glMatrixMode(GL_MODELVIEW);
    }
    if(cur.glowtmu>=0)
    {
        glActiveTexture_(GL_TEXTURE0_ARB+cur.glowtmu);
        resettmu(cur.glowtmu);
        disabletexgen();
        glDisable(GL_TEXTURE_2D);
    }
    if(cur.fogtmu>=0)
    {
        glActiveTexture_(GL_TEXTURE0_ARB+cur.fogtmu);
        resettmu(cur.fogtmu);
        disabletexgen(1);
        glDisable(GL_TEXTURE_1D);
    }
    if(cur.causticstmu>=0) loopi(2)
    {
        glActiveTexture_(GL_TEXTURE0_ARB+cur.causticstmu+i);
        resettmu(cur.causticstmu+i);
        disabletexgen();
        glDisable(GL_TEXTURE_2D);
    }
        
    if(cur.lightmaptmu>=0)
    {
        glActiveTexture_(GL_TEXTURE0_ARB+cur.diffusetmu);
        resettmu(cur.diffusetmu);
        glDisable(GL_TEXTURE_2D);
    }

    if(renderpath==R_FIXEDFUNCTION) disabletexgen();
    else
    {
        glDisableClientState(GL_COLOR_ARRAY);
        loopi(8-2) { glActiveTexture_(GL_TEXTURE2_ARB+i); glDisable(GL_TEXTURE_2D); }
    }

    if(cur.lightmaptmu>=0)
    {
        glActiveTexture_(GL_TEXTURE0_ARB);
        glClientActiveTexture_(GL_TEXTURE0_ARB);
        glEnable(GL_TEXTURE_2D);
    }
}

#define FIRSTVA (reflecting ? reflectedva : visibleva)
#define NEXTVA (reflecting ? va->rnext : va->next)

static void rendergeommultipass(renderstate &cur, int pass, bool fogpass)
{
    cur.vbuf = 0;
    for(vtxarray *va = FIRSTVA; va; va = NEXTVA)
    {
        if(!va->texs || va->occluded >= OCCLUDE_GEOM) continue;
        if(refracting)
        {    
            if(refracting < 0 ? va->geommin.z > reflectz : va->geommax.z <= reflectz) continue;
            if(isvisiblecube(va->o, va->size) >= VFC_NOT_VISIBLE) continue;
            if((!hasOQ || !oqfrags) && va->distance > reflectdist) break;
        }
        else if(reflecting)
        {
            if(va->geommax.z <= reflectz || (va->rquery && checkquery(va->rquery))) continue;
        }
        if(fogpass ? va->geommax.z <= reflectz-waterfog : va->curvfc==VFC_FOGGED) continue;
        renderva(cur, va, pass, fogpass);
    }
    if(geombatches.length()) renderbatches(cur, pass);
}

VAR(oqgeom, 0, 1, 1);
VAR(oqbatch, 0, 1, 1);

VAR(dbgffsm, 0, 0, 1);
VAR(dbgffdl, 0, 0, 1);
VAR(ffdlscissor, 0, 1, 1);

void rendergeom(float causticspass, bool fogpass)
{
    renderstate cur;

    if(causticspass && ((renderpath==R_FIXEDFUNCTION && maxtmus<2) || !causticscale || !causticmillis)) causticspass = 0;

    glEnableClientState(GL_VERTEX_ARRAY);

    if(!reflecting && !refracting)
    {
        flipqueries();
        vtris = vverts = 0;
    }

    bool doOQ = reflecting ? hasOQ && oqfrags && oqreflect : !refracting && zpass!=0;
    if(!doOQ) 
    {
        setupTMUs(cur, causticspass, fogpass);
        if(shadowmap && !envmapping && !glaring && renderpath!=R_FIXEDFUNCTION) pushshadowmap();
    }

    int hasdynlights = finddynlights();

    glPushMatrix();

    resetorigin();

    resetbatches();

    int blends = 0;
    for(vtxarray *va = FIRSTVA; va; va = NEXTVA)
    {
        if(!va->texs) continue;
        if(refracting)
        {
            if((refracting < 0 ? va->geommin.z > reflectz : va->geommax.z <= reflectz) || va->occluded >= OCCLUDE_GEOM) continue;
            if(isvisiblecube(va->o, va->size) >= VFC_NOT_VISIBLE) continue;
            if((!hasOQ || !oqfrags) && va->distance > reflectdist) break;
        }
        else if(reflecting)
        {
            if(va->geommax.z <= reflectz) continue;
            if(doOQ)
            {
                va->rquery = newquery(&va->rquery);
                if(!va->rquery) continue;
                if(va->occluded >= OCCLUDE_BB || va->curvfc >= VFC_NOT_VISIBLE)
                {
                    renderquery(cur, va->rquery, va);
                    continue;
                }
            }
        }
        else if(hasOQ && oqfrags && (zpass || va->distance > oqdist) && !insideva(va, camera1->o) && oqgeom)
        {
            if(!zpass && va->query && va->query->owner == va) 
            {
                if(checkquery(va->query)) va->occluded = min(va->occluded+1, int(OCCLUDE_BB));
                else va->occluded = pvsoccluded(va->geommin, va->geommax) ? OCCLUDE_GEOM : OCCLUDE_NOTHING;
            }
            if(zpass && oqbatch)
            {
                if(va->parent && va->parent->occluded >= OCCLUDE_BB)
                {
                    va->query = NULL;
                    va->occluded = OCCLUDE_PARENT;
                    continue;
                }
                bool succeeded = false;
                if(va->query && va->query->owner == va && checkquery(va->query))
                {
                    va->occluded = min(va->occluded+1, int(OCCLUDE_BB));
                    succeeded = true;
                }
                va->query = newquery(va);
                if(!va->query || !succeeded) 
                    va->occluded = pvsoccluded(va->geommin, va->geommax) ? OCCLUDE_GEOM : OCCLUDE_NOTHING;
                if(va->occluded >= OCCLUDE_GEOM)
                {
                    if(va->query) renderquery(cur, va->query, va);
                    continue;
                }
            }
            else if(zpass && va->parent && 
               (va->parent->occluded == OCCLUDE_PARENT || 
                (va->parent->occluded >= OCCLUDE_BB && 
                 va->parent->query && va->parent->query->owner == va->parent && va->parent->query->fragments < 0)))
            {
                va->query = NULL;
                if(va->occluded >= OCCLUDE_GEOM || pvsoccluded(va->geommin, va->geommax))
                {
                    va->occluded = OCCLUDE_PARENT;
                    continue;
                }
            }
            else if(va->occluded >= OCCLUDE_GEOM)
            {
                va->query = newquery(va);
                if(va->query) renderquery(cur, va->query, va);
                continue;
            }
            else va->query = newquery(va);
        }
        else
        {
            va->query = NULL;
            va->occluded = pvsoccluded(va->geommin, va->geommax) ? OCCLUDE_GEOM : OCCLUDE_NOTHING;
            if(va->occluded >= OCCLUDE_GEOM) continue;
        }

        if(!doOQ) blends += va->blends;
        renderva(cur, va, doOQ ? RENDERPASS_Z : (nolights ? RENDERPASS_COLOR : RENDERPASS_LIGHTMAP), fogpass, true);
    }

    if(geombatches.length()) renderbatches(cur, nolights ? RENDERPASS_COLOR : RENDERPASS_LIGHTMAP);

    if(!cur.colormask) { cur.colormask = true; glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE); }
    if(!cur.depthmask) { cur.depthmask = true; glDepthMask(GL_TRUE); }
   
    if(doOQ)
    {
        setupTMUs(cur, causticspass, fogpass);
        if(shadowmap && !envmapping && !glaring && renderpath!=R_FIXEDFUNCTION) 
        {
            glPopMatrix();
            glPushMatrix();
            pushshadowmap();
            resetorigin();
        }
        glDepthFunc(GL_LEQUAL);
        cur.vbuf = 0;

        for(vtxarray **prevva = &FIRSTVA, *va = FIRSTVA; va; prevva = &NEXTVA, va = NEXTVA)
        {
            if(!va->texs) continue;
            if(reflecting)
            {
                if(va->geommax.z <= reflectz) continue;
                if(va->rquery && checkquery(va->rquery))
                {
                    if(va->occluded >= OCCLUDE_BB || va->curvfc >= VFC_NOT_VISIBLE) *prevva = va->rnext;
                    continue;
                }
            }
            else if(oqbatch)
            {
                if(va->occluded >= OCCLUDE_GEOM) continue;
            }
            else if(va->parent && va->parent->occluded >= OCCLUDE_BB && (!va->parent->query || va->parent->query->fragments >= 0))
            {
                va->query = NULL;
                va->occluded = OCCLUDE_BB;
                continue;
            }
            else
            {
                if(va->query && checkquery(va->query)) va->occluded = min(va->occluded+1, int(OCCLUDE_BB));
                else va->occluded = pvsoccluded(va->geommin, va->geommax) ? OCCLUDE_GEOM : OCCLUDE_NOTHING;
                if(va->occluded >= OCCLUDE_GEOM) continue;
            }
            
            blends += va->blends;
            renderva(cur, va, nolights ? RENDERPASS_COLOR : RENDERPASS_LIGHTMAP, fogpass);
        }
        if(geombatches.length()) renderbatches(cur, nolights ? RENDERPASS_COLOR : RENDERPASS_LIGHTMAP);
        if(oqbatch && !reflecting) for(vtxarray **prevva = &FIRSTVA, *va = FIRSTVA; va; prevva = &NEXTVA, va = NEXTVA)
        {
            if(!va->texs || va->occluded < OCCLUDE_GEOM) continue;
            else if(va->query && checkquery(va->query)) continue;
            else if(va->parent && (va->parent->occluded >= OCCLUDE_BB ||
                    (va->parent->occluded >= OCCLUDE_GEOM && va->parent->query && checkquery(va->parent->query))))
            {
                va->occluded = OCCLUDE_BB;
                continue;
            }
            else
            {
                va->occluded = pvsoccluded(va->geommin, va->geommax) ? OCCLUDE_GEOM : OCCLUDE_NOTHING;
                if(va->occluded >= OCCLUDE_GEOM) continue;
            }

            blends += va->blends;
            renderva(cur, va, nolights ? RENDERPASS_COLOR : RENDERPASS_LIGHTMAP, fogpass);
        }
        if(geombatches.length()) renderbatches(cur, nolights ? RENDERPASS_COLOR : RENDERPASS_LIGHTMAP);
   
        if(foggedvas.empty()) glDepthFunc(GL_LESS);
    }

    if(blends && (renderpath!=R_FIXEDFUNCTION || !nolights))
    {
        if(shadowmap && !envmapping && !glaring && renderpath!=R_FIXEDFUNCTION)
        {
            glPopMatrix();
            glPushMatrix();
            resetorigin();
        }
        if(foggedvas.empty()) glDepthFunc(GL_LEQUAL);
        glDepthMask(GL_FALSE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);

        cur.vbuf = 0;
        cur.blending = true;
        for(vtxarray **prevva = &FIRSTVA, *va = FIRSTVA; va; prevva = &NEXTVA, va = NEXTVA)
        {
            if(!va->blends || va->occluded >= OCCLUDE_GEOM) continue;
            if(refracting)
            {
                if(refracting < 0 ? va->geommin.z > reflectz : va->geommax.z <= reflectz) continue;
                if(isvisiblecube(va->o, va->size) >= VFC_NOT_VISIBLE) continue;
                if((!hasOQ || !oqfrags) && va->distance > reflectdist) break;
            }
            else if(reflecting)
            {
                if(va->geommax.z <= reflectz || (va->rquery && checkquery(va->rquery))) continue;
            }
            if(fogpass ? va->geommax.z <= reflectz-waterfog : va->curvfc==VFC_FOGGED) continue;
            renderva(cur, va, RENDERPASS_LIGHTMAP_BLEND, fogpass);
        }
        if(geombatches.length()) renderbatches(cur, RENDERPASS_LIGHTMAP);
        cur.blending = false;

        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glDisable(GL_BLEND);
        glDepthMask(GL_TRUE);
        if(foggedvas.empty()) glDepthFunc(GL_LESS);
    }

    if(shadowmap && !envmapping && !glaring && renderpath!=R_FIXEDFUNCTION) popshadowmap();

    cleanupTMUs(cur);

    if(foggedvas.length()) 
    {
        renderfoggedvas(cur, !doOQ);
        if(doOQ) glDepthFunc(GL_LESS);
    }

    if(renderpath==R_FIXEDFUNCTION ? (glowpass && cur.skippedglow) || (causticspass>=1 && cur.causticstmu<0) || (fogpass && cur.fogtmu<0) || (shadowmap && shadowmapcasters) || hasdynlights : causticspass)
    {
        glDepthFunc(GL_LEQUAL);
        glDepthMask(GL_FALSE);
        glEnable(GL_BLEND);
        static GLfloat zerofog[4] = { 0, 0, 0, 1 }, onefog[4] = { 1, 1, 1, 1 }; 
        GLfloat oldfogc[4];
        glGetFloatv(GL_FOG_COLOR, oldfogc);

        if(renderpath==R_FIXEDFUNCTION && glowpass && cur.skippedglow)
        {
            glBlendFunc(GL_ONE, GL_ONE);
            glFogfv(GL_FOG_COLOR, zerofog);
            setuptexgen();
            if(cur.fogtmu>=0)
            {
                setuptmu(0, "C * T");
                glActiveTexture_(GL_TEXTURE1_ARB);
                glEnable(GL_TEXTURE_1D);
                setuptexgen(1);
                setuptmu(1, "P * T~a");
                if(!fogtex) createfogtex();
                glBindTexture(GL_TEXTURE_1D, fogtex);    
                glActiveTexture_(GL_TEXTURE0_ARB);
            } 
            cur.glowcolor = vec(-1, -1, -1);
            cur.glowtmu = 0;
            rendergeommultipass(cur, RENDERPASS_GLOW, fogpass);
            disabletexgen();
            if(cur.fogtmu>=0)
            {
                resettmu(0);
                glActiveTexture_(GL_TEXTURE1_ARB);
                resettmu(1);
                disabletexgen(1);
                glDisable(GL_TEXTURE_1D);
                glActiveTexture_(GL_TEXTURE0_ARB);
            } 
        }

        if(renderpath==R_FIXEDFUNCTION ? causticspass>=1 && cur.causticstmu<0 : causticspass)
        {
            setupcaustics(0, causticspass);
            glBlendFunc(GL_ZERO, renderpath==R_FIXEDFUNCTION ? GL_SRC_COLOR : GL_ONE_MINUS_SRC_COLOR);
            glFogfv(GL_FOG_COLOR, renderpath==R_FIXEDFUNCTION ? onefog : zerofog);
            if(fading) glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);
            rendergeommultipass(cur, RENDERPASS_CAUSTICS, fogpass);
            if(fading) glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
            loopi(2)
            {
                glActiveTexture_(GL_TEXTURE0_ARB+i);
                resettmu(i);
                if(renderpath==R_FIXEDFUNCTION || !i) 
                {
                    resettmu(i);
                    disabletexgen();
                }
                if(i) glDisable(GL_TEXTURE_2D);
            }
            glActiveTexture_(GL_TEXTURE0_ARB);
        }

        if(renderpath==R_FIXEDFUNCTION && shadowmap && shadowmapcasters)
        {
            glBlendFunc(GL_ZERO, GL_ONE_MINUS_SRC_COLOR);
            glFogfv(GL_FOG_COLOR, zerofog);
            glPopMatrix();
            glPushMatrix();
            pushshadowmap();
            resetorigin();
            if(cur.fogtmu>=0)
            {
                setuptmu(0, "C * T");
                glActiveTexture_(GL_TEXTURE1_ARB);
                glEnable(GL_TEXTURE_1D);
                setuptexgen(1);
                setuptmu(1, "P * T~a");
                if(!fogtex) createfogtex();
                glBindTexture(GL_TEXTURE_1D, fogtex);
                glActiveTexture_(GL_TEXTURE0_ARB);
            }
            if(dbgffsm) { glDisable(GL_BLEND); glDisable(GL_TEXTURE_2D); glColor3f(1, 0, 1); }
            rendergeommultipass(cur, RENDERPASS_SHADOWMAP, fogpass);
            if(dbgffsm) { glEnable(GL_BLEND); glEnable(GL_TEXTURE_2D); }
            popshadowmap();
            if(cur.fogtmu>=0)
            {
                resettmu(0);
                glActiveTexture_(GL_TEXTURE1_ARB);
                resettmu(1);
                disabletexgen();
                glDisable(GL_TEXTURE_1D);
                glActiveTexture_(GL_TEXTURE0_ARB);
            }
        }
   
        if(renderpath==R_FIXEDFUNCTION && hasdynlights)
        {
            glBlendFunc(GL_SRC_ALPHA, dbgffdl ? GL_ZERO : GL_ONE);
            glFogfv(GL_FOG_COLOR, zerofog);

            if(!attenxytex) attenxytex = createattenxytex(64);
            glBindTexture(GL_TEXTURE_2D, attenxytex);

            setuptmu(0, "= C", "= Ta");
            setuptexgen();

            glActiveTexture_(GL_TEXTURE1_ARB);
            setuptmu(1, "= P", "Pa - Ta");
            setuptexgen(1);
            if(!attenztex) attenztex = createattenztex(64);
            glBindTexture(GL_TEXTURE_1D, attenztex);
            glEnable(GL_TEXTURE_1D);
 
            glActiveTexture_(GL_TEXTURE2_ARB);
            cur.diffusetmu = 2;
            setuptmu(2, "P * T x 4", "= Pa");
            setuptexgen();
            glEnable(GL_TEXTURE_2D);

            vec lightcolor;
            for(int n = 0; getdynlight(n, cur.dynlightpos, cur.dynlightradius, lightcolor); n++)
            {
                lightcolor.mul(0.5f);
                if(fogpass && cur.fogtmu>=0)
                {
                    float fog = (reflectz - cur.dynlightpos.z)/waterfog;
                    if(fog >= 1.0f) continue;
                    lightcolor.mul(1.0f - max(fog, 0.0f));
                }
                glColor3f(lightcolor.x, lightcolor.y, lightcolor.z);
                if(ffdlscissor)
                {
                    float sx1, sy1, sx2, sy2;
                    calcspherescissor(cur.dynlightpos, cur.dynlightradius, sx1, sy1, sx2, sy2);
                    pushscissor(sx1, sy1, sx2, sy2);
                }
                resetorigin();
                rendergeommultipass(cur, RENDERPASS_DYNLIGHT, fogpass);
                if(ffdlscissor) popscissor();
            }

            glDisable(GL_TEXTURE_2D);
            disabletexgen();
            resettmu(2);

            glActiveTexture_(GL_TEXTURE1_ARB);
            glDisable(GL_TEXTURE_1D);
            resettmu(1);
            disabletexgen(1);
            
            glActiveTexture_(GL_TEXTURE0_ARB);
            resettmu(0);
            disabletexgen();
        }

        if(renderpath==R_FIXEDFUNCTION && fogpass && cur.fogtmu<0)
        {
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glDisable(GL_TEXTURE_2D);
            glEnable(GL_TEXTURE_1D);
            setuptexgen(1);
            if(!fogtex) createfogtex();
            glBindTexture(GL_TEXTURE_1D, fogtex);
            setuptexgen(1);
            glColor3ubv(watercolor.v);
            rendergeommultipass(cur, RENDERPASS_FOG, fogpass);
            disabletexgen(1);
            glDisable(GL_TEXTURE_1D);
            glEnable(GL_TEXTURE_2D);
        }

        glFogfv(GL_FOG_COLOR, oldfogc);
        glDisable(GL_BLEND);
        glDepthFunc(GL_LESS);
        glDepthMask(GL_TRUE);
    }

    glPopMatrix();

    if(hasVBO)
    {
        glBindBuffer_(GL_ARRAY_BUFFER_ARB, 0);
        glBindBuffer_(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
    }
    glDisableClientState(GL_VERTEX_ARRAY);
}

void findreflectedvas(vector<vtxarray *> &vas, int prevvfc = VFC_PART_VISIBLE)
{
    bool doOQ = hasOQ && oqfrags && oqreflect;
    loopv(vas)
    {
        vtxarray *va = vas[i];
        if(prevvfc >= VFC_NOT_VISIBLE) va->curvfc = prevvfc;
        if(va->curvfc == VFC_FOGGED || va->curvfc == PVS_FOGGED || va->o.z+va->size <= reflectz || isvisiblecube(va->o, va->size) >= VFC_FOGGED) continue;
        bool render = true;
        if(va->curvfc == VFC_FULL_VISIBLE)
        {
            if(va->occluded >= OCCLUDE_BB) continue;
            if(va->occluded >= OCCLUDE_GEOM) render = false;
        }
        else if(va->curvfc == PVS_FULL_VISIBLE) continue;
        if(render)
        {
            if(va->curvfc >= VFC_NOT_VISIBLE) va->distance = (int)vadist(va, camera1->o);
            if(!doOQ && va->distance > reflectdist) continue;
            va->rquery = NULL;
            vtxarray **vprev = &reflectedva, *vcur = reflectedva;
            while(vcur && va->distance > vcur->distance)
            {
                vprev = &vcur->rnext;
                vcur = vcur->rnext;
            }
            va->rnext = *vprev;
            *vprev = va;
        }
        if(va->children.length()) findreflectedvas(va->children, va->curvfc);
    }
}

void renderreflectedgeom(bool causticspass, bool fogpass)
{
    if(reflecting)
    {
        reflectedva = NULL;
        findreflectedvas(varoot);
        rendergeom(causticspass ? 1 : 0, fogpass);
    }
    else rendergeom(causticspass ? 1 : 0, fogpass);
}                

static vtxarray *prevskyva = NULL;

void renderskyva(vtxarray *va, bool explicitonly = false)
{
    if(!prevskyva || va->vbuf != prevskyva->vbuf)
    {
        if(!prevskyva)
        {
            glEnableClientState(GL_VERTEX_ARRAY);
            glPushMatrix();
            resetorigin();
        }

        setorigin(va);
        if(hasVBO)
        {
            glBindBuffer_(GL_ARRAY_BUFFER_ARB, va->vbuf);
            glBindBuffer_(GL_ELEMENT_ARRAY_BUFFER_ARB, va->skybuf);
        }
        glVertexPointer(3, floatvtx ? GL_FLOAT : GL_SHORT, VTXSIZE, &va->vdata[0].x);
    }

    drawvatris(va, explicitonly ? va->explicitsky : va->sky+va->explicitsky, explicitonly ? va->skydata+va->sky : va->skydata);

    if(!explicitonly) xtraverts += va->sky/3;
    xtraverts += va->explicitsky/3;

    prevskyva = va;
}

int renderedsky = 0, renderedexplicitsky = 0, renderedskyfaces = 0, renderedskyclip = INT_MAX;

static inline void updateskystats(vtxarray *va)
{
    renderedsky += va->sky;
    renderedexplicitsky += va->explicitsky;
    renderedskyfaces |= va->skyfaces&0x3F;
    if(!(va->skyfaces&0x1F) || camera1->o.z < va->skyclip) renderedskyclip = min(renderedskyclip, va->skyclip);
    else renderedskyclip = 0;
}

void renderreflectedskyvas(vector<vtxarray *> &vas, int prevvfc = VFC_PART_VISIBLE)
{
    loopv(vas)
    {
        vtxarray *va = vas[i];
        if(prevvfc >= VFC_NOT_VISIBLE) va->curvfc = prevvfc;
        if((va->curvfc == VFC_FULL_VISIBLE && va->occluded >= OCCLUDE_BB) || va->curvfc==PVS_FULL_VISIBLE) continue;
        if(va->o.z+va->size <= reflectz || isvisiblecube(va->o, va->size) == VFC_NOT_VISIBLE) continue;
        if(va->sky+va->explicitsky) 
        {
            updateskystats(va);
            renderskyva(va);
        }
        if(va->children.length()) renderreflectedskyvas(va->children, va->curvfc);
    }
}

bool rendersky(bool explicitonly)
{
    prevskyva = NULL;
    renderedsky = renderedexplicitsky = renderedskyfaces = 0;
    renderedskyclip = INT_MAX;

    if(reflecting)
    {
        renderreflectedskyvas(varoot);
    }
    else for(vtxarray *va = visibleva; va; va = va->next)
    {
        if((va->occluded >= OCCLUDE_BB && va->skyfaces&0x80) || !(va->sky+va->explicitsky)) continue;

        // count possibly visible sky even if not actually rendered
        updateskystats(va);
        if(explicitonly && !va->explicitsky) continue;
        renderskyva(va, explicitonly);
    }

    if(prevskyva)
    {
        glPopMatrix();
        glDisableClientState(GL_VERTEX_ARRAY);
        if(hasVBO) 
        {
            glBindBuffer_(GL_ARRAY_BUFFER_ARB, 0);
            glBindBuffer_(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
        }
    }

    return renderedsky+renderedexplicitsky > 0;
}

