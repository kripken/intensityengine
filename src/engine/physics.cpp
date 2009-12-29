// physics.cpp: no physics books were hurt nor consulted in the construction of this code.
// All physics computations and constants were invented on the fly and simply tweaked until
// they "felt right", and have no basis in reality. Collision detection is simplistic but
// very robust (uses discrete steps at fixed fps).

#include "engine.h"
#include "game.h" // INTENSITY: for fpsent

#include "world_system.h" // INTENSITY
#include "targeting.h" // INTENSITY
#include "script_engine_manager.h"
#ifdef CLIENT
    #include "client_engine_additions.h" // INTENSITY
#endif


const int MAXCLIPPLANES = 1024;

clipplanes clipcache[MAXCLIPPLANES], *nextclip = clipcache;

static inline void setcubeclip(cube &c, int x, int y, int z, int size)
{
    if(!c.ext || !c.ext->clip || c.ext->clip->owner!=&c)
    {
        if(nextclip >= &clipcache[MAXCLIPPLANES]) nextclip = clipcache;
        ext(c).clip = nextclip;
        nextclip->owner = &c;
        genclipplanes(c, x, y, z, size, *nextclip);
        nextclip++;
    }
}
    
void freeclipplanes(cube &c)
{
    if(!c.ext || !c.ext->clip) return;
    if(c.ext->clip->owner==&c) c.ext->clip->owner = NULL;
    c.ext->clip = NULL;
}

/////////////////////////  ray - cube collision ///////////////////////////////////////////////

static inline void pushvec(vec &o, const vec &ray, float dist)
{
    vec d(ray);
    d.mul(dist);
    o.add(d);
}

static inline bool pointinbox(const vec &v, const vec &bo, const vec &br)
{
    return v.x <= bo.x+br.x &&
           v.x >= bo.x-br.x &&
           v.y <= bo.y+br.y &&
           v.y >= bo.y-br.y &&
           v.z <= bo.z+br.z &&
           v.z >= bo.z-br.z;
}

bool pointincube(const clipplanes &p, const vec &v)
{
    if(!pointinbox(v, p.o, p.r)) return false;
    loopi(p.size) if(p.p[i].dist(v)>1e-3f) return false;
    return true;
}

#define INTERSECTPLANES(setentry) \
    clipplanes &p = *c.ext->clip; \
    float enterdist = -1e16f, exitdist = 1e16f; \
    loopi(p.size) \
    { \
        float pdist = p.p[i].dist(o), facing = ray.dot(p.p[i]); \
        if(facing < 0) \
        { \
            pdist /= -facing; \
            if(pdist > enterdist) \
            { \
                if(pdist > exitdist) return false; \
                enterdist = pdist; \
                setentry; \
            } \
        } \
        else if(facing > 0) \
        { \
            pdist /= -facing; \
            if(pdist < exitdist) \
            { \
                if(pdist < enterdist) return false; \
                exitdist = pdist; \
            } \
        } \
        else if(pdist > 0) return false; \
    }

// optimized shadow version
bool shadowcubeintersect(const cube &c, const vec &o, const vec &ray, float &dist)
{
    INTERSECTPLANES({});
    if(exitdist < 0) return false;
    enterdist += 0.1f;
    if(enterdist < 0) enterdist = 0;
    if(!pointinbox(vec(ray).mul(enterdist).add(o), p.o, p.r)) return false;
    dist = enterdist;
    return true;
}
 
vec hitsurface;

bool raycubeintersect(const cube &c, const vec &o, const vec &ray, float &dist)
{
    int entry = -1, bbentry = -1;
    INTERSECTPLANES(entry = i);
    loop(i, 3)
    {
        if(ray[i])
        {
            float pdist = ((ray[i] > 0 ? p.o[i]-p.r[i] : p.o[i]+p.r[i]) - o[i]) / ray[i];
            if(pdist > enterdist)
            {
                if(pdist > exitdist) return false;
                enterdist = pdist;
                bbentry = i;
            }
            pdist += 2*p.r[i]/fabs(ray[i]);
            if(pdist < exitdist)
            {
                if(pdist < enterdist) return false;
                exitdist = pdist;
            }
         }
         else if(o[i] < p.o[i]-p.r[i] || o[i] > p.o[i]+p.r[i]) return false;
    }
    if(exitdist < 0) return false;
    dist = max(enterdist+0.1f, 0.0f);
    if(bbentry>=0) { hitsurface = vec(0, 0, 0); hitsurface[bbentry] = ray[bbentry]>0 ? -1 : 1; }
    else hitsurface = p.p[entry]; 
    return true;
}

extern void entselectionbox(const entity &e, vec &eo, vec &es);
extern int entselradius;
float hitentdist;
int hitent, hitorient;

static float disttoent(octaentities *oc, octaentities *last, const vec &o, const vec &ray, float radius, int mode, extentity *t)
{
    vec eo, es;
    int orient;
    float dist = 1e16f, f = 0.0f;
    if(oc == last || oc == NULL) return dist;
    const vector<extentity *> &ents = entities::getents();

    #define entintersect(mask, type, func) {\
        if((mode&(mask))==(mask)) \
        { \
            loopv(oc->type) \
                if(!last || last->type.find(oc->type[i])<0) \
                { \
                    extentity &e = *ents[oc->type[i]]; \
                    if(!e.inoctanode || &e==t) continue; \
                    func; \
                    if(f<dist && f>0) \
                    { \
                        hitentdist = dist = f; \
                        hitent = oc->type[i]; \
                        hitorient = orient; \
                    } \
                } \
        } \
    }

    entintersect(RAY_POLY, mapmodels,
        if(e.attr3 && (e.triggerstate == TRIGGER_DISAPPEARED || !checktriggertype(e.attr3, TRIG_COLLIDE) || e.triggerstate == TRIGGERED) && (mode&RAY_ENTS)!=RAY_ENTS) continue;
        orient = 0; // FIXME, not set
        if(!mmintersect(e, o, ray, radius, mode, f)) continue;
    );

    entintersect(RAY_ENTS, other,
        entselectionbox(e, eo, es);
        if(!rayrectintersect(eo, es, o, ray, f, orient)) continue;
    );

    entintersect(RAY_ENTS, mapmodels,
        entselectionbox(e, eo, es);
        if(!rayrectintersect(eo, es, o, ray, f, orient)) continue;
    );

    return dist;
}

static float disttooutsideent(const vec &o, const vec &ray, float radius, int mode, extentity *t)
{
    vec eo, es;
    int orient;
    float dist = 1e16f, f = 0.0f;
    const vector<extentity *> &ents = entities::getents();
    loopv(outsideents)
    {
        extentity &e = *ents[outsideents[i]];
        if(!e.inoctanode || &e == t) continue;
        entselectionbox(e, eo, es);
        if(!rayrectintersect(eo, es, o, ray, f, orient)) continue;
        if(f<dist && f>0) 
        {
            hitentdist = dist = f;
            hitent = outsideents[i];
            hitorient = orient;
        }
    }
    return dist;
}
         
// optimized shadow version
static float shadowent(octaentities *oc, octaentities *last, const vec &o, const vec &ray, float radius, int mode, extentity *t)
{
    float dist = 1e16f, f = 0.0f;
    if(oc == last || oc == NULL) return dist;
    const vector<extentity *> &ents = entities::getents();
    loopv(oc->mapmodels) if(!last || last->mapmodels.find(oc->mapmodels[i])<0)
    {
        extentity &e = *ents[oc->mapmodels[i]];
        if(!e.inoctanode || &e==t) continue;
        if(e.attr3 && (e.triggerstate == TRIGGER_DISAPPEARED || !checktriggertype(e.attr3, TRIG_COLLIDE) || e.triggerstate == TRIGGERED)) continue;
        if(!mmintersect(e, o, ray, radius, mode, f)) continue;
        if(f>0 && f<dist) dist = f;
    } 
    return dist;
}

#define INITRAYCUBE \
    octaentities *oclast = NULL; \
    float dist = 0, dent = mode&RAY_BB ? 1e16f : 1e14f; \
    vec v(o), invray(ray.x ? 1/ray.x : 1e16f, ray.y ? 1/ray.y : 1e16f, ray.z ? 1/ray.z : 1e16f); \
    static cube *levels[32]; \
    levels[worldscale] = worldroot; \
    int lshift = worldscale; \
    ivec lsizemask(invray.x>0 ? 1 : 0, invray.y>0 ? 1 : 0, invray.z>0 ? 1 : 0); \

#define CHECKINSIDEWORLD \
    if(!insideworld(o)) \
    { \
        float disttoworld = 0, exitworld = 1e16f; \
        loopi(3) \
        { \
            float c = v[i]; \
            if(c<0 || c>=worldsize) \
            { \
                float d = ((invray[i]>0?0:worldsize)-c)*invray[i]; \
                if(d<0) return (radius>0?radius:-1); \
                disttoworld = max(disttoworld, 0.1f + d); \
            } \
            float e = ((invray[i]>0?worldsize:0)-c)*invray[i]; \
            exitworld = min(exitworld, e); \
        } \
        if(disttoworld > exitworld) return (radius>0?radius:-1); \
        pushvec(v, ray, disttoworld); \
        dist += disttoworld; \
    }

#define DOWNOCTREE(disttoent, earlyexit) \
        cube *lc = levels[lshift]; \
        for(;;) \
        { \
            lshift--; \
            lc += octastep(x, y, z, lshift); \
            if(lc->ext && lc->ext->ents && dent > 1e15f) \
            { \
                dent = disttoent(lc->ext->ents, oclast, o, ray, radius, mode, t); \
                if(dent < 1e15f earlyexit) return min(dent, dist); \
                oclast = lc->ext->ents; \
            } \
            if(lc->children==NULL) break; \
            lc = lc->children; \
            levels[lshift] = lc; \
        }
        
#define FINDCLOSEST(xclosest, yclosest, zclosest) \
        float dx = (lo.x+(lsizemask.x<<lshift)-v.x)*invray.x, \
              dy = (lo.y+(lsizemask.y<<lshift)-v.y)*invray.y, \
              dz = (lo.z+(lsizemask.z<<lshift)-v.z)*invray.z; \
        float disttonext = dx; \
        xclosest; \
        if(dy < disttonext) { disttonext = dy; yclosest; } \
        if(dz < disttonext) { disttonext = dz; zclosest; } \
        disttonext += 0.1f; \
        pushvec(v, ray, disttonext); \
        dist += disttonext;

#define UPOCTREE(exitworld) \
        x = int(v.x); \
        y = int(v.y); \
        z = int(v.z); \
        uint diff = uint(lo.x^x)|uint(lo.y^y)|uint(lo.z^z); \
        if(diff >= uint(worldsize)) exitworld; \
        diff >>= lshift; \
        if(!diff) exitworld; \
        do \
        { \
            lshift++; \
            diff >>= 1; \
        } while(diff);

float raycube(const vec &o, const vec &ray, float radius, int mode, int size, extentity *t)
{
    if(ray.iszero()) return 0;

    INITRAYCUBE;
    CHECKINSIDEWORLD;

    int closest = -1, x = int(v.x), y = int(v.y), z = int(v.z);
    for(;;)
    {
        DOWNOCTREE(disttoent, && (mode&RAY_SHADOW));

        int lsize = 1<<lshift;

        cube &c = *lc;
        if((dist>0 || !(mode&RAY_SKIPFIRST)) &&
           (((mode&RAY_CLIPMAT) && c.ext && isclipped(c.ext->material&MATF_VOLUME)) ||
            ((mode&RAY_EDITMAT) && c.ext && c.ext->material != MAT_AIR) ||
            (!(mode&RAY_PASS) && lsize==size && !isempty(c)) ||
            isentirelysolid(c) ||
            dent < dist))
        {
            if(closest >= 0) { hitsurface = vec(0, 0, 0); hitsurface[closest] = ray[closest]>0 ? -1 : 1; }
            return min(dent, dist);
        }

        ivec lo(x&(~0<<lshift), y&(~0<<lshift), z&(~0<<lshift));

        if(!isempty(c))
        {
            float f = 0;
            setcubeclip(c, lo.x, lo.y, lo.z, lsize);
            if(raycubeintersect(c, v, ray, f) && (dist+f>0 || !(mode&RAY_SKIPFIRST)))
                return min(dent, dist+f);
        }

        FINDCLOSEST(closest = 0, closest = 1, closest = 2);

        if(radius>0 && dist>=radius) return min(dent, dist);

        UPOCTREE(return min(dent, radius>0 ? radius : dist));
    }
}

// optimized version for lightmap shadowing... every cycle here counts!!!
float shadowray(const vec &o, const vec &ray, float radius, int mode, extentity *t)
{
    INITRAYCUBE;
    CHECKINSIDEWORLD;

    int x = int(v.x), y = int(v.y), z = int(v.z);
    for(;;)
    {
        DOWNOCTREE(shadowent, );

        cube &c = *lc;
        if(isentirelysolid(c)) return dist;

        ivec lo(x&(~0<<lshift), y&(~0<<lshift), z&(~0<<lshift));

        if(!isempty(c))
        {
            float f = 0;
            setcubeclip(c, lo.x, lo.y, lo.z, 1<<lshift);
            if(shadowcubeintersect(c, v, ray, f)) return dist+f;
        }

        FINDCLOSEST( , , );

        if(dist>=radius) return dist;

        UPOCTREE(return radius);
    }
}

float rayent(const vec &o, const vec &ray, float radius, int mode, int size, int &orient, int &ent)
{
    hitent = -1;
    hitentdist = radius;
    float dist = raycube(o, ray, radius, mode, size);
    if((mode&RAY_ENTS) == RAY_ENTS)
    {
        float dent = disttooutsideent(o, ray, dist < 0 ? 1e16f : dist, mode, NULL);
        if(dent < 1e15f && (dist < 0 || dent < dist)) dist = dent; 
    }
    orient = hitorient;
    ent = hitentdist == dist ? hitent : -1;
    return dist;
}

float raycubepos(const vec &o, const vec &ray, vec &hitpos, float radius, int mode, int size)
{
    hitpos = ray;
    float dist = raycube(o, ray, radius, mode, size);
    hitpos.mul(dist).add(o); 
    return dist; 
}

bool raycubelos(const vec &o, const vec &dest, vec &hitpos)
{
    vec ray(dest);
    ray.sub(o);
    float mag = ray.magnitude();
    ray.mul(1/mag);
    float distance = raycubepos(o, ray, hitpos, mag, RAY_CLIPMAT|RAY_POLY);
    return distance >= mag;
}

float rayfloor(const vec &o, vec &floor, int mode, float radius)
{
    if(o.z<=0) return -1;
    hitsurface = vec(0, 0, 1);
    float dist = raycube(o, vec(0, 0, -1), radius, mode);
    if(dist<0 || (radius>0 && dist>=radius)) return dist;
    floor = hitsurface;
    return dist;
}

/////////////////////////  entity collision  ///////////////////////////////////////////////

// info about collisions
bool inside; // whether an internal collision happened
physent *hitplayer; // whether the collection hit a player
vec wall; // just the normal vectors.
float walldistance;
const float STAIRHEIGHT = 4.1f;
const float FLOORZ = 0.867f;
const float SLOPEZ = 0.5f;
const float WALLZ = 0.2f;
#define JUMPVEL (1.25f*(100+pl->maxspeed)/2) /* INTENSITY: Scale with maxspeed, so that maxspeed = 100 gives the old value here */
float GRAVITY; // INTENSITY: Removed 'const' (and 'external') and the value itself

bool ellipserectcollide(physent *d, const vec &dir, const vec &o, const vec &center, float yaw, float xr, float yr, float hi, float lo)
{
    float below = (o.z+center.z-lo) - (d->o.z+d->aboveeye),
          above = (d->o.z-d->eyeheight) - (o.z+center.z+hi);
    if(below>=0 || above>=0) return true;

    vec yo(d->o);
    yo.sub(o);
    yo.rotate_around_z(-yaw*RAD);
    yo.sub(center);
 
    float dx = clamp(yo.x, -xr, xr) - yo.x, dy = clamp(yo.y, -yr, yr) - yo.y,
          dist = sqrtf(dx*dx + dy*dy) - d->radius;
    if(dist < 0)
    {
        int sx = yo.x <= -xr ? -1 : (yo.x >= xr ? 1 : 0),
            sy = yo.y <= -yr ? -1 : (yo.y >= yr ? 1 : 0);
        if(dist > (yo.z < 0 ? below : above) && (sx || sy))
        {
            vec ydir(dir);
            ydir.rotate_around_z(-yaw*RAD);
            if(sx*yo.x - xr > sy*yo.y - yr)
            {
                if(dir.iszero() || sx*ydir.x < -1e-6f)
                {
                    wall = vec(sx, 0, 0);
                    wall.rotate_around_z(yaw*RAD);
                    return false;
                }
            }
            else if(dir.iszero() || sy*ydir.y < -1e-6f)
            { 
                wall = vec(0, sy, 0);
                wall.rotate_around_z(yaw*RAD);
                return false;
            }
        }
        if(yo.z < 0)
        {
            if(dir.iszero() || (dir.z > 0 && (d->type>=ENT_INANIMATE || below >= d->zmargin-(d->eyeheight+d->aboveeye)/4.0f)))
            {
                wall = vec(0, 0, -1);
                return false;
            }
        }
        else if(dir.iszero() || (dir.z < 0 && (d->type>=ENT_INANIMATE || above >= d->zmargin-(d->eyeheight+d->aboveeye)/3.0f)))
        {
            wall = vec(0, 0, 1);
            return false;
        }
        inside = true;
    }
    return true;
}

bool ellipsecollide(physent *d, const vec &dir, const vec &o, const vec &center, float yaw, float xr, float yr, float hi, float lo)
{
    float below = (o.z+center.z-lo) - (d->o.z+d->aboveeye),
          above = (d->o.z-d->eyeheight) - (o.z+center.z+hi);
    if(below>=0 || above>=0) return true;
    vec yo(center);
    yo.rotate_around_z(yaw*RAD);
    yo.add(o);
    float x = yo.x - d->o.x, y = yo.y - d->o.y;
    float angle = atan2f(y, x), dangle = angle-(d->yaw+90)*RAD, eangle = angle-(yaw+90)*RAD;
    float dx = d->xradius*cosf(dangle), dy = d->yradius*sinf(dangle);
    float ex = xr*cosf(eangle), ey = yr*sinf(eangle);
    float dist = sqrtf(x*x + y*y) - sqrtf(dx*dx + dy*dy) - sqrtf(ex*ex + ey*ey);
    if(dist < 0)
    {
        if(dist > (d->o.z < yo.z ? below : above) && (dir.iszero() || x*dir.x + y*dir.y > 0))
        {
            wall = vec(-x, -y, 0);
            if(!wall.iszero()) wall.normalize();
            return false;
        }
        if(d->o.z < yo.z)
        {
            if(dir.iszero() || (dir.z > 0 && (d->type>=ENT_INANIMATE || below >= d->zmargin-(d->eyeheight+d->aboveeye)/4.0f)))
            {
                wall = vec(0, 0, -1);
                return false;
            }
        }
        else if(dir.iszero() || (dir.z < 0 && (d->type>=ENT_INANIMATE || above >= d->zmargin-(d->eyeheight+d->aboveeye)/3.0f)))
        {
            wall = vec(0, 0, 1);
            return false;
        }
        inside = true;
    }
    return true;
}

bool rectcollide(physent *d, const vec &dir, const vec &o, float xr, float yr,  float hi, float lo, uchar visible = 0xFF, bool collideonly = true, float cutoff = 0)
{
    vec s(d->o);
    s.sub(o);
    float dxr = d->collidetype==COLLIDE_ELLIPSE ? d->radius : d->xradius, dyr = d->collidetype==COLLIDE_ELLIPSE ? d->radius : d->yradius;
    xr += dxr;
    yr += dyr;
    walldistance = -1e10f;
    float zr = s.z>0 ? d->eyeheight+hi : d->aboveeye+lo;
    float ax = fabs(s.x)-xr;
    float ay = fabs(s.y)-yr;
    float az = fabs(s.z)-zr;
    if(ax>0 || ay>0 || az>0) return true;
    wall.x = wall.y = wall.z = 0;
#define TRYCOLLIDE(dim, ON, OP, N, P) \
    { \
        if(s.dim<0) { if(visible&(1<<ON) && (dir.iszero() || (dir.dim>0 && (d->type>=ENT_INANIMATE || (N))))) { walldistance = a ## dim; wall.dim = -1; return false; } } \
        else if(visible&(1<<OP) && (dir.iszero() || (dir.dim<0 && (d->type>=ENT_INANIMATE || (P))))) { walldistance = a ## dim; wall.dim = 1; return false; } \
    }
    if(ax>ay && ax>az) TRYCOLLIDE(x, O_LEFT, O_RIGHT, ax > -dxr, ax > -dxr);
    if(ay>az) TRYCOLLIDE(y, O_BACK, O_FRONT, ay > -dyr, ay > -dyr);
    TRYCOLLIDE(z, O_BOTTOM, O_TOP,
         az >= d->zmargin-(d->eyeheight+d->aboveeye)/4.0f,
         az >= d->zmargin-(d->eyeheight+d->aboveeye)/3.0f);
    if(collideonly) inside = true;
    return collideonly;
}

#define DYNENTCACHESIZE 1024

static uint dynentframe = 0;

static struct dynentcacheentry
{
    int x, y;
    uint frame;
    vector<physent *> dynents;
} dynentcache[DYNENTCACHESIZE];

void cleardynentcache()
{
    dynentframe++;
    if(!dynentframe || dynentframe == 1) loopi(DYNENTCACHESIZE) dynentcache[i].frame = 0;
    if(!dynentframe) dynentframe = 1;
}

VARF(dynentsize, 4, 7, 12, cleardynentcache());

#define DYNENTHASH(x, y) (((((x)^(y))<<5) + (((x)^(y))>>5)) & (DYNENTCACHESIZE - 1))

const vector<physent *> &checkdynentcache(int x, int y)
{
    dynentcacheentry &dec = dynentcache[DYNENTHASH(x, y)];
    if(dec.x == x && dec.y == y && dec.frame == dynentframe) return dec.dynents;
    dec.x = x;
    dec.y = y;
    dec.frame = dynentframe;
    dec.dynents.setsize(0);
    int numdyns = game::numdynents(), dsize = 1<<dynentsize, dx = x<<dynentsize, dy = y<<dynentsize;
    loopi(numdyns)
    {
        dynent *d = game::iterdynents(i);
        if(d->state != CS_ALIVE ||
           d->o.x+d->radius <= dx || d->o.x-d->radius >= dx+dsize ||
           d->o.y+d->radius <= dy || d->o.y-d->radius >= dy+dsize)
            continue;
        dec.dynents.add(d);
    }
    return dec.dynents;
}

#define loopdynentcache(curx, cury, o, radius) \
    for(int curx = max(int(o.x-radius), 0)>>dynentsize, endx = min(int(o.x+radius), worldsize-1)>>dynentsize; curx <= endx; curx++) \
    for(int cury = max(int(o.y-radius), 0)>>dynentsize, endy = min(int(o.y+radius), worldsize-1)>>dynentsize; cury <= endy; cury++)

void updatedynentcache(physent *d)
{
    loopdynentcache(x, y, d->o, d->radius)
    {
        dynentcacheentry &dec = dynentcache[DYNENTHASH(x, y)];
        if(dec.x != x || dec.y != y || dec.frame != dynentframe || dec.dynents.find(d) >= 0) continue;
        dec.dynents.add(d);
    }
}

bool overlapsdynent(const vec &o, float radius)
{
    loopdynentcache(x, y, o, radius)
    {
        const vector<physent *> &dynents = checkdynentcache(x, y);
        loopv(dynents)
        {
            physent *d = dynents[i];
            if(o.dist(d->o)-d->radius < radius) return true;
        }
    }
    return false;
}

bool plcollide(physent *d, const vec &dir)    // collide with player or monster
{
    if(d->type==ENT_CAMERA || d->state!=CS_ALIVE) return true;
    loopdynentcache(x, y, d->o, d->radius)
    {
        const vector<physent *> &dynents = checkdynentcache(x, y);
        loopv(dynents)
        {
            physent *o = dynents[i];
            if(o==d || d->o.reject(o->o, d->radius+o->radius)) continue;
            if(d->collidetype!=COLLIDE_ELLIPSE || o->collidetype!=COLLIDE_ELLIPSE)
            { 
                if(!rectcollide(d, dir, o->o, o->collidetype==COLLIDE_ELLIPSE ? o->radius : o->xradius, o->collidetype==COLLIDE_ELLIPSE ? o->radius : o->yradius, o->aboveeye, o->eyeheight))
                {
                    hitplayer = o;
                    if((d->type==ENT_AI || d->type==ENT_INANIMATE) && wall.z>0) d->onplayer = o;
                    return false;
                }
            }
            else if(!ellipsecollide(d, dir, o->o, vec(0, 0, 0), o->yaw, o->xradius, o->yradius, o->aboveeye, o->eyeheight))
            {
                hitplayer = o;
                if((d->type==ENT_AI || d->type==ENT_INANIMATE) && wall.z>0) d->onplayer = o;
                return false;
            }
        }
    }
    return true;
}

void rotatebb(vec &center, vec &radius, int yaw)
{
    yaw = (yaw+7)-(yaw+7)%15;
    yaw += 180;
    if(yaw < 0) yaw = 360 + yaw%360;
    else if(yaw >= 360) yaw %= 360;
    switch(yaw)
    {
        case 0: break;
        case 180:
            center.x = -center.x;
            center.y = -center.y;
            break;
        case 90:
            swap(radius.x, radius.y);
            swap(center.x, center.y);
            center.x = -center.x;
            break;
        case 270:
            swap(radius.x, radius.y);
            swap(center.x, center.y);
            center.y = -center.y;
            break;
        default:
            radius.x = radius.y = max(radius.x, radius.y) + max(fabs(center.x), fabs(center.y));
            center.x = center.y = 0.0f;
            break;
    }
}

bool mmcollide(physent *d, const vec &dir, octaentities &oc)               // collide with a mapmodel
{   
    const vector<extentity *> &ents = entities::getents();
    loopv(oc.mapmodels)
    {
        extentity &e = *ents[oc.mapmodels[i]];
#if 0
        if(e.attr3 && e.attr3!=15 && (e.triggerstate == TRIGGER_DISAPPEARED || !checktriggertype(e.attr3, TRIG_COLLIDE) || e.triggerstate == TRIGGERED || (e.triggerstate == TRIGGERING && lastmillis-e.lasttrigger >= 500))) continue;
        model *m = loadmodel(NULL, e.attr2);
#else // INTENSITY: Use entity info to get the model
        LogicEntityPtr entity = LogicSystem::getLogicEntity(e);

        model *m = entity.get()->getModel(); //loadmodel(NULL, e.attr2);
        if(!m) continue;
        if ( (m->collisionsonlyfortriggering && !WorldSystem::triggeringCollisions) ||
             (!m->collisionsonlyfortriggering && WorldSystem::triggeringCollisions) )
            continue; // If mapmodel type not relevant for current physics test, ignore

        #define HANDLE_TRIGGERING_COLLISIONS \
                if (WorldSystem::triggeringCollisions) \
                { \
                    WorldSystem::triggerCollide(entity, d, false); \
                    continue; /* This is not a 'real' collision, we just checked for trigger events, let other mapmodels */ \
                              /* also trigger XXX  FIXME: Might this same mapmodel be tested twice? */ \
                }
#endif

        if(!m || !m->collide) continue;
        vec center, radius;
        m->collisionbox(0, center, radius, entity.get()); // INTENSITY: entity

        if(d->collidetype==COLLIDE_ELLIPSE)
        {
            float yaw = 180 + float((e.attr1+7)-(e.attr1+7)%15);
            if(m->ellipsecollide)
            {
                if(!ellipsecollide(d, !WorldSystem::triggeringCollisions ? dir : vec(0,0,0), e.o, center, yaw, radius.x, radius.y, radius.z, radius.z)) // INTENSITY: If just for triggering events (AreaTriggers), then we want all intersections regardless of direction
                {
                    HANDLE_TRIGGERING_COLLISIONS // INTENSITY: Fire trigger, and handle collisiononlyfortriggering case

                    return false;
                }
            }
            else if(!ellipserectcollide(d, !WorldSystem::triggeringCollisions ? dir : vec(0,0,0), e.o, center, yaw, radius.x, radius.y, radius.z, radius.z)) // INTENSITY: If just for triggering events (AreaTriggers), then we want all intersections regardless of direction
            {
                HANDLE_TRIGGERING_COLLISIONS // INTENSITY: Fire trigger, and handle collisiononlyfortriggering case

                return false;
            }
        } 
        else
        {
            rotatebb(center, radius, e.attr1);
            if(!rectcollide(d, !WorldSystem::triggeringCollisions ? dir : vec(0,0,0), center.add(e.o), radius.x, radius.y, radius.z, radius.z)) // INTENSITY: If just for triggering events (AreaTriggers), then we want all intersections regardless of direction
            {
                HANDLE_TRIGGERING_COLLISIONS // INTENSITY: Fire trigger, and handle collisiononlyfortriggering case

                return false;
            }
        }
    }
    return true;
}

bool cubecollide(physent *d, const vec &dir, float cutoff, cube &c, int x, int y, int z, int size, bool solid) // collide with cube geometry
{
    if(solid || isentirelysolid(c))
    {
        int s2 = size>>1;
        vec o = vec(x+s2, y+s2, z+s2);
        vec r = vec(s2, s2, s2);
        return rectcollide(d, dir, o, r.x, r.y, r.z, r.z, isentirelysolid(c) ? (c.ext ? c.ext->visible : 0) : 0xFF, true, cutoff);
    }

    setcubeclip(c, x, y, z, size);
    clipplanes &p = *c.ext->clip;

    float r = d->radius,
          zr = (d->aboveeye+d->eyeheight)/2;
    vec o(d->o), *w = &wall;
    o.z += zr - d->eyeheight;

    if(rectcollide(d, dir, p.o, p.r.x, p.r.y, p.r.z, p.r.z, c.ext->visible, !p.size, cutoff)) return true;

    if(p.size)
    {
        if(!wall.iszero())
        {
            vec wo(o), wrad(r, r, zr);
            loopi(3) if(wall[i]) { wo[i] = p.o[i]+wall[i]*p.r[i]; wrad[i] = 0; break; }
            loopi(p.size)
            {
                plane &f = p.p[i];
                if(!wall.dot(f)) continue;
                if(f.dist(wo) >= vec(f.x*wrad.x, f.y*wrad.y, f.z*wrad.z).magnitude()) 
                { 
                    wall = vec(0, 0, 0);
                    walldistance = -1e10f;
                    break;
                }
            }
        }
        float m = walldistance;
        loopi(p.size)
        {
            plane &f = p.p[i];
            float dist = f.dist(o) - vec(f.x*r, f.y*r, f.z*zr).magnitude();
            if(dist > 0) return true;
            if(dist > m)
            {
                if(!dir.iszero())
                {
                    if(f.dot(dir) >= -cutoff*dir.magnitude()) continue;
                    if(d->type<ENT_CAMERA && 
                        dist < (dir.z*f.z < 0 ? 
                            d->zmargin-(d->eyeheight+d->aboveeye)/(dir.z < 0 ? 3.0f : 4.0f) : 
                            ((dir.x*f.x < 0 || dir.y*f.y < 0) ? -r : 0))) 
                        continue;
                }
                if(f.x && (f.x>0 ? o.x-p.o.x : p.o.x-o.x) + p.r.x - r < dist && 
                    f.dist(vec(p.o.x + (f.x>0 ? -p.r.x : p.r.x), o.y, o.z)) >= vec(0, f.y*r, f.z*zr).magnitude()) 
                    continue;
                if(f.y && (f.y>0 ? o.y-p.o.y : p.o.y-o.y) + p.r.y - r < dist && 
                    f.dist(vec(o.x, p.o.y + (f.y>0 ? -p.r.y : p.r.y), o.z)) >= vec(f.x*r, 0, f.z*zr).magnitude()) 
                    continue;
                if(f.z && (f.z>0 ? o.z-p.o.z : p.o.z-o.z) + p.r.z - zr < dist && 
                    f.dist(vec(o.x, o.y, p.o.z + (f.z>0 ? -p.r.z : p.r.z))) >= vec(f.x*r, f.y*r, 0).magnitude()) 
                    continue;
                w = &f;
                m = dist;
            }
        }
        wall = *w;
        if(wall.iszero())
        {
            inside = true;
            return true;
        }
    }
    return false;
}

static inline bool octacollide(physent *d, const vec &dir, float cutoff, const ivec &bo, const ivec &bs, cube *c, const ivec &cor, int size) // collide with octants
{
    loopoctabox(cor, size, bo, bs)
    {
        if(c[i].ext && c[i].ext->ents) if(!mmcollide(d, dir, *c[i].ext->ents)) return false;
        ivec o(i, cor.x, cor.y, cor.z, size);
        if(c[i].children)
        {
            if(!octacollide(d, dir, cutoff, bo, bs, c[i].children, o, size>>1)) return false;
        }
        else
        {
            bool solid = false;
            if(c[i].ext) switch(c[i].ext->material&MATF_CLIP)
            {
                case MAT_NOCLIP: continue;
                case MAT_GAMECLIP: if(d->type==ENT_AI) solid = true; break;
                case MAT_CLIP: if(isclipped(c[i].ext->material&MATF_VOLUME) || d->type<ENT_CAMERA) solid = true; break;
            }
            if(!solid && isempty(c[i])) continue;
            if(!cubecollide(d, dir, cutoff, c[i], o.x, o.y, o.z, size, solid)) return false;
        }
    }
    return true;
}

static inline bool octacollide(physent *d, const vec &dir, float cutoff, const ivec &bo, const ivec &bs)
{
    int diff = (bo.x^(bo.x+bs.x)) | (bo.y^(bo.y+bs.y)) | (bo.z^(bo.z+bs.z)),
        scale = worldscale-1;
    if(diff&~((1<<scale)-1) || uint(bo.x|bo.y|bo.z|(bo.x+bs.x)|(bo.y+bs.y)|(bo.z+bs.z)) >= uint(worldsize))
       return octacollide(d, dir, cutoff, bo, bs, worldroot, ivec(0, 0, 0), worldsize>>1);
    cube *c = &worldroot[octastep(bo.x, bo.y, bo.z, scale)];
    if(c->ext && c->ext->ents && !mmcollide(d, dir, *c->ext->ents)) return false;
    scale--;
    while(c->children && !(diff&(1<<scale)))
    {
        c = &c->children[octastep(bo.x, bo.y, bo.z, scale)];
        if(c->ext && c->ext->ents && !mmcollide(d, dir, *c->ext->ents)) return false;
        scale--;
    }
    if(c->children) return octacollide(d, dir, cutoff, bo, bs, c->children, ivec(bo).mask(~((2<<scale)-1)), 1<<scale);
    bool solid = false;
    if(c->ext) switch(c->ext->material&MATF_CLIP)
    {
        case MAT_NOCLIP: return true;
        case MAT_GAMECLIP: if(d->type==ENT_AI) solid = true; break;
        case MAT_CLIP: if(isclipped(c->ext->material&MATF_VOLUME) || d->type<ENT_CAMERA) solid = true; break;
    }
    if(!solid && isempty(*c)) return true;
    int csize = 2<<scale, cmask = ~(csize-1);
    return cubecollide(d, dir, cutoff, *c, bo.x&cmask, bo.y&cmask, bo.z&cmask, csize, solid);
}

// all collision happens here
bool collide(physent *d, const vec &dir, float cutoff, bool playercol)
{
    inside = false;
    hitplayer = NULL;
    wall.x = wall.y = wall.z = 0;
    ivec bo(int(d->o.x-d->radius), int(d->o.y-d->radius), int(d->o.z-d->eyeheight)),
         bs(int(d->radius)*2, int(d->radius)*2, int(d->eyeheight+d->aboveeye));
    bs.add(2);  // guard space for rounding errors
    if(!octacollide(d, dir, cutoff, bo, bs)) return false;//, worldroot, ivec(0, 0, 0), worldsize>>1)) return false; // collide with world
    return !playercol || plcollide(d, dir);
}

void recalcdir(physent *d, const vec &oldvel, vec &dir)
{
    float speed = oldvel.magnitude();
    if(speed > 1e-6f)
    {
        float step = dir.magnitude();
        dir = d->vel;
        dir.add(d->falling);
        dir.mul(step/speed);
    }
}

void slideagainst(physent *d, vec &dir, const vec &obstacle, bool foundfloor, bool slidecollide)
{
    vec wall(obstacle);
    if(foundfloor ? wall.z > 0 : slidecollide)
    {
        wall.z = 0;
        if(!wall.iszero()) wall.normalize();
    }
    vec oldvel(d->vel);
    oldvel.add(d->falling);
    d->vel.project(wall);
    d->falling.project(wall);
    recalcdir(d, oldvel, dir);
}

void switchfloor(physent *d, vec &dir, const vec &floor)
{
    if(floor.z >= FLOORZ) d->falling = vec(0, 0, 0);

    vec oldvel(d->vel);
    oldvel.add(d->falling);
    if(dir.dot(floor) >= 0)
    {
        if(d->physstate < PHYS_SLIDE || fabs(dir.dot(d->floor)) > 0.01f*dir.magnitude()) return;
        d->vel.projectxy(floor, 0.0f);
    }
    else d->vel.projectxy(floor);
    d->falling.project(floor);
    recalcdir(d, oldvel, dir);
}

bool trystepup(physent *d, vec &dir, const vec &obstacle, float maxstep, const vec &floor)
{
    vec old(d->o), stairdir = (obstacle.z >= 0 && obstacle.z < SLOPEZ ? vec(-obstacle.x, -obstacle.y, 0) : vec(dir.x, dir.y, 0)).rescale(1);
    bool cansmooth = true;
    /* check if there is space atop the stair to move to */
    if(d->physstate != PHYS_STEP_UP)
    {
        vec checkdir = stairdir;
        checkdir.mul(0.1f);
        checkdir.z += maxstep + 0.1f;
        d->o.add(checkdir);
        if(!collide(d))
        {
            d->o = old;
            if(collide(d, vec(0, 0, -1), SLOPEZ)) return false;
            cansmooth = false;
        }
    }

    if(cansmooth)
    {
        d->o = old;
        vec checkdir = stairdir;
        checkdir.z += 1;
        checkdir.mul(maxstep);
        d->o.add(checkdir);
        if(!collide(d, checkdir))
        {
            if(collide(d, vec(0, 0, -1), SLOPEZ))
            {
                d->o = old;
                return false;
            }
        }

        /* try stepping up half as much as forward */
        d->o = old;
        vec smoothdir(dir.x, dir.y, 0);
        float magxy = smoothdir.magnitude();
        if(magxy > 1e-9f)
        {
            if(magxy > 2*dir.z) 
            {
                smoothdir.mul(1/magxy);
                smoothdir.z = 0.5f;
                smoothdir.mul(dir.magnitude()/smoothdir.magnitude());
            }
            else smoothdir.z = dir.z;
            d->o.add(smoothdir);
            d->o.z += maxstep + 0.1f;
            if(collide(d, smoothdir))
            {
                d->o.z -= maxstep + 0.1f;
                if(d->physstate == PHYS_FALL || d->floor != floor)
                {
                    d->timeinair = 0;
                    d->floor = floor;
                    switchfloor(d, dir, d->floor);
                }
                d->physstate = PHYS_STEP_UP;
                return true;
            }
        }
    }

    /* try stepping up */
    d->o = old;     
    d->o.z += dir.magnitude();
    if(collide(d, vec(0, 0, 1)))
    {
        if(d->physstate == PHYS_FALL || d->floor != floor)
        {
            d->timeinair = 0;
            d->floor = floor;
            switchfloor(d, dir, d->floor);
        }
        if(cansmooth) d->physstate = PHYS_STEP_UP;
        return true;
    }
    d->o = old;
    return false;
}

#if 0
bool trystepdown(physent *d, vec &dir, float step, float a, float b)
{
    vec old(d->o);
    vec dv(dir.x*a, dir.y*a, -step*b), v(dv);
    v.mul(STAIRHEIGHT/(step*b));
    d->o.add(v);
    if(!collide(d, vec(0, 0, -1), SLOPEZ))
    {
        d->o = old;
        d->o.add(dv);
        if(collide(d, vec(0, 0, -1))) return true;
    }
    d->o = old;
    return false;
}
#endif

void falling(physent *d, vec &dir, const vec &floor)
{
#if 0
    if(d->physstate >= PHYS_FLOOR && (floor.z == 0.0f || floor.z == 1.0f))
    {
        vec moved(d->o);
        d->o.z -= STAIRHEIGHT + 0.1f;
        if(!collide(d, vec(0, 0, -1), SLOPEZ))
        {
            d->o = moved;
            d->physstate = PHYS_STEP_DOWN;
            return;
        }
        else d->o = moved;
    }
#endif
    if(floor.z > 0.0f && floor.z < SLOPEZ)
    {
        if(floor.z >= WALLZ) switchfloor(d, dir, floor);
        d->timeinair = 0;
        d->physstate = PHYS_SLIDE;
        d->floor = floor;
    }
    else d->physstate = PHYS_FALL;
}

void landing(physent *d, vec &dir, const vec &floor, bool collided)
{
#if 0
    if(d->physstate == PHYS_FALL)
    {
        d->timeinair = 0;
        if(dir.z < 0.0f) dir.z = d->vel.z = 0.0f;
    }
#endif
    switchfloor(d, dir, floor);
    d->timeinair = 0;
    if(d->physstate!=PHYS_STEP_UP || !collided)
        d->physstate = floor.z >= FLOORZ ? PHYS_FLOOR : PHYS_SLOPE;
    d->floor = floor;
}

bool findfloor(physent *d, bool collided, const vec &obstacle, bool &slide, vec &floor)
{
    bool found = false;
    vec moved(d->o);
    d->o.z -= 0.1f;
    if(!collide(d, vec(0, 0, -1), d->physstate == PHYS_SLOPE ? SLOPEZ : FLOORZ))
    {
        floor = wall;
        found = true;
    }
    else if(collided && obstacle.z >= SLOPEZ)
    {
        floor = obstacle;
        found = true;
        slide = false;
    }
    else if(d->physstate == PHYS_STEP_UP || d->physstate == PHYS_SLIDE)
    {
        if(!collide(d, vec(0, 0, -1)) && wall.z > 0.0f)
        {
            floor = wall;
            if(floor.z >= SLOPEZ) found = true;
        }
    }
    else if(d->physstate >= PHYS_SLOPE && d->floor.z < 1.0f)
    {
        if(!collide(d, vec(d->floor).neg(), 0.95f) || !collide(d, vec(0, 0, -1)))
        {
            floor = wall;
            if(floor.z >= SLOPEZ && floor.z < 1.0f) found = true;
        }
    }
    if(collided && (!found || obstacle.z > floor.z)) 
    {
        floor = obstacle;
        slide = !found && (floor.z < WALLZ || floor.z >= SLOPEZ);
    }
    d->o = moved;
    return found;
}

bool move(physent *d, vec &dir)
{
    vec old(d->o);
#if 0
    if(d->physstate == PHYS_STEP_DOWN && dir.z <= 0.0f && game::allowmove(pl) && (d->move || d->strafe))
    {
        float step = dir.magnitude();
        if(trystepdown(d, dir, step, 0.75f, 0.25f)) return true;
        if(trystepdown(d, dir, step, 0.5f, 0.5f)) return true;
        if(trystepdown(d, dir, step, 0.25f, 0.75f)) return true;
        d->o.z -= step;
        if(collide(d, vec(0, 0, -1))) return true;
        d->o = old;
    }
#endif
    bool collided = false, slidecollide = false;
    vec obstacle;
    d->o.add(dir);
    if(!collide(d, dir) || ((d->type==ENT_AI || d->type==ENT_INANIMATE) && !collide(d, vec(0, 0, 0), 0, false)))
    {
        obstacle = wall;
        /* check to see if there is an obstacle that would prevent this one from being used as a floor (or ceiling bump) */
        if(d->type==ENT_PLAYER && ((wall.z>=SLOPEZ && dir.z<0) || (wall.z<=-SLOPEZ && dir.z>0)) && (dir.x || dir.y) && !collide(d, vec(dir.x, dir.y, 0)))
        {
            if(wall.dot(dir) >= 0) slidecollide = true;
            obstacle = wall;
        }
        d->o = old;
        d->o.z -= STAIRHEIGHT;
        d->zmargin = -STAIRHEIGHT;
        if(d->physstate == PHYS_SLOPE || d->physstate == PHYS_FLOOR || (!collide(d, vec(0, 0, -1), SLOPEZ) && (d->physstate==PHYS_STEP_UP || wall.z>=FLOORZ)))
        {
            d->o = old;
            d->zmargin = 0;
            if(trystepup(d, dir, obstacle, STAIRHEIGHT, d->physstate == PHYS_SLOPE || d->physstate == PHYS_FLOOR ? d->floor : vec(wall))) return true;
        }
        else 
        {
            d->o = old; 
            d->zmargin = 0;
        }
        /* can't step over the obstacle, so just slide against it */
        collided = true;
    }
    else if(d->physstate == PHYS_STEP_UP) 
    {
        if(!collide(d, vec(0, 0, -1), SLOPEZ))
        {
            d->o = old;
            if(trystepup(d, dir, vec(0, 0, 1), STAIRHEIGHT, vec(wall))) return true;
            d->o.add(dir);
        }
    }
    vec floor(0, 0, 0);
    bool slide = collided,
         found = findfloor(d, collided, obstacle, slide, floor);
    if(slide || (!collided && floor.z > 0 && floor.z < WALLZ))
    {
        slideagainst(d, dir, slide ? obstacle : floor, found, slidecollide);
        /* if(d->type == ENT_AI || d->type == ENT_INANIMATE) */ d->blocked = true; // INTENSITY: Always note this
    }
    if(found) landing(d, dir, floor, collided);
    else falling(d, dir, floor);
    return !collided;
}

bool bounce(physent *d, float secs, float elasticity, float waterfric)
{
    // make sure bouncers don't start inside geometry
    if(d->physstate!=PHYS_BOUNCE && !collide(d, vec(0, 0, 0), 0, false)) return true;
    int mat = lookupmaterial(vec(d->o.x, d->o.y, d->o.z + (d->aboveeye - d->eyeheight)/2));
    bool water = isliquid(mat);
    if(water) 
    {
        d->vel.z -= GRAVITY/16*secs;
        d->vel.mul(max(1.0f - secs/waterfric, 0.0f));
    }
    else d->vel.z -= GRAVITY*secs;
    vec old(d->o);
    loopi(2)
    {
        vec dir(d->vel);
        dir.mul(secs);
        d->o.add(dir);
        if(collide(d, dir))
        {
            if(inside)
            {
                d->o = old;
                d->vel.mul(-elasticity);
            }
            break;
        }
        else if(hitplayer) break;
        d->o = old;
        float c = wall.dot(d->vel),
              k = 1.0f + (1.0f-elasticity)*c/d->vel.magnitude();
        d->vel.mul(k);
        d->vel.sub(vec(wall).mul(elasticity*2.0f*c));
    }
    if(d->physstate!=PHYS_BOUNCE)
    {
        // make sure bouncers don't start inside geometry
        if(d->o == old) return !hitplayer;
        d->physstate = PHYS_BOUNCE;
    }
    return hitplayer!=0;
}

void avoidcollision(physent *d, const vec &dir, physent *obstacle, float space)
{
    float rad = obstacle->radius+d->radius;
    vec bbmin(obstacle->o);
    bbmin.x -= rad;
    bbmin.y -= rad;
    bbmin.z -= obstacle->eyeheight+d->aboveeye;
    bbmin.sub(space);
    vec bbmax(obstacle->o);
    bbmax.x += rad;
    bbmax.y += rad;
    bbmax.z += obstacle->aboveeye+d->eyeheight;
    bbmax.add(space);

    loopi(3) if(d->o[i] <= bbmin[i] || d->o[i] >= bbmax[i]) return;

    float mindist = 1e16f;
    loopi(3) if(dir[i] != 0)
    {
        float dist = ((dir[i] > 0 ? bbmax[i] : bbmin[i]) - d->o[i]) / dir[i];
        mindist = min(mindist, dist);
    }
    if(mindist >= 0.0f && mindist < 1e15f) d->o.add(vec(dir).mul(mindist));
}

bool movecamera(physent *pl, const vec &dir, float dist, float stepdist)
{
    int steps = (int)ceil(dist/stepdist);
    if(steps <= 0) return true;

    vec d(dir);
    d.mul(dist/steps);
    loopi(steps)
    {
        vec oldpos(pl->o);
        pl->o.add(d);
        if(!collide(pl, vec(0, 0, 0), 0, false))
        {
            pl->o = oldpos;
            return false;
        }
    }
    return true;
}

bool droptofloor(vec &o, float radius, float height)
{
    if(!insideworld(o)) return false;
    vec v(0.0001f, 0.0001f, -1);
    v.normalize();
    if(raycube(o, v, worldsize) >= worldsize) return false;
    physent d;
    d.type = ENT_CAMERA;
    d.o = o;
    d.vel = vec(0, 0, -1);
    d.radius = d.xradius = d.yradius = radius;
    d.eyeheight = height;
    d.aboveeye = radius;
    if(!movecamera(&d, d.vel, worldsize, 1))
    {
        o = d.o;
        return true;
    }
    return false;
}

float dropheight(entity &e)
{
    switch(e.type)
    {
        case ET_PARTICLES:
        case ET_MAPMODEL: return 0.0f;
        default:
            if(e.type >= ET_GAMESPECIFIC) return entities::dropheight(e);
            return 4.0f;
    }
}

void dropenttofloor(entity *e)
{
    droptofloor(e->o, 1.0f, dropheight(*e));
}

void phystest()
{
    static const char *states[] = {"float", "fall", "slide", "slope", "floor", "step up", "step down", "bounce"};
    printf ("PHYS(pl): %s, air %d, floor: (%f, %f, %f), vel: (%f, %f, %f), g: (%f, %f, %f)\n", states[player->physstate], player->timeinair, player->floor.x, player->floor.y, player->floor.z, player->vel.x, player->vel.y, player->vel.z, player->falling.x, player->falling.y, player->falling.z);
    printf ("PHYS(cam): %s, air %d, floor: (%f, %f, %f), vel: (%f, %f, %f), g: (%f, %f, %f)\n", states[camera1->physstate], camera1->timeinair, camera1->floor.x, camera1->floor.y, camera1->floor.z, camera1->vel.x, camera1->vel.y, camera1->vel.z, camera1->falling.x, camera1->falling.y, camera1->falling.z);
}

COMMAND(phystest, "");

void vecfromyawpitch(float yaw, float pitch, int move, int strafe, vec &m)
{
    if(move)
    {
        m.x = move*sinf(RAD*yaw);
        m.y = move*-cosf(RAD*yaw);
    }
    else m.x = m.y = 0;

    if(pitch)
    {
        m.x *= cosf(RAD*pitch);
        m.y *= cosf(RAD*pitch);
        m.z = move*sinf(RAD*pitch);
    }
    else m.z = 0;

    if(strafe)
    {
        m.x += strafe*-cosf(RAD*yaw);
        m.y += strafe*-sinf(RAD*yaw);
    }
}

void vectoyawpitch(const vec &v, float &yaw, float &pitch)
{
    yaw = -(float)atan2(v.x, v.y)/RAD + 180;
    pitch = asin(v.z/v.magnitude())/RAD;
}

VARP(maxroll, 0, 3, 20);
FVAR(straferoll, 0, 0.033f, 90);
VAR(floatspeed, 10, 100, 1000);

void modifyvelocity(physent *pl, bool local, bool water, bool floating, int curtime)
{
    if(floating)
    {
        if(pl->jumping)
        {
            pl->jumping = false;
            pl->vel.z = JUMPVEL;
        }
    }
    else if(pl->physstate >= PHYS_SLOPE || water)
    {
        if(water && !pl->inwater) pl->vel.div(8);
        if(pl->jumping)
        {
            pl->jumping = false;

            pl->vel.z = JUMPVEL; // physics impulse upwards
            if(water) { pl->vel.x /= 8.0f; pl->vel.y /= 8.0f; } // dampen velocity change even harder, gives correct water feel

            game::physicstrigger(pl, local, 1, 0);
        }
    }
    if(!floating && pl->physstate == PHYS_FALL) pl->timeinair += curtime;

    vec m(0.0f, 0.0f, 0.0f);
    if(pl->type==ENT_AI)
    {
        dynent *d = (dynent *)pl;
        if(d->rotspeed && d->yaw!=d->targetyaw)
        {
            float oldyaw = d->yaw, diff = d->rotspeed*curtime/1000.0f, maxdiff = fabs(d->targetyaw-d->yaw);
            if(diff >= maxdiff)
            {
                d->yaw = d->targetyaw;
                d->rotspeed = 0;
            }
            else d->yaw += (d->targetyaw>d->yaw ? 1 : -1) * min(diff, maxdiff);
            d->normalize_yaw(d->targetyaw);
            if(!plcollide(d, vec(0, 0, 0)))
            {
                d->yaw = oldyaw;
                m.x = d->o.x - hitplayer->o.x;
                m.y = d->o.y - hitplayer->o.y;
                if(!m.iszero()) m.normalize();
            }
        }
    }

    if(m.iszero() && game::allowmove(pl) && (pl->move || pl->strafe))
    {
        vecfromyawpitch(pl->yaw, floating || water || pl->type==ENT_CAMERA ? pl->pitch : 0, pl->move, pl->strafe, m);

        if(!floating && pl->physstate >= PHYS_SLOPE)
        {
            /* move up or down slopes in air
             * but only move up slopes in water
             */
            float dz = -(m.x*pl->floor.x + m.y*pl->floor.y)/pl->floor.z;
            m.z = water ? max(m.z, dz) : dz;
        }

        m.normalize();
    }

    vec d(m);
    d.mul(pl->maxspeed);
    if(pl->type==ENT_PLAYER)
    {
        if(floating) 
        {
            if(pl==player) d.mul(floatspeed/100.0f);
        }
        else if(!water && game::allowmove(pl)) d.mul((pl->move && !pl->strafe ? 1.3f : 1.0f) * (pl->physstate < PHYS_SLOPE ? 1.3f : 1.0f)); // EXPERIMENTAL
    }
    float fric = water && !floating ? 20.0f : (pl->physstate >= PHYS_SLOPE || floating ? 6.0f : 30.0f);
    pl->vel.lerp(d, pl->vel, pow(1 - 1/fric, curtime/20.0f));
// old fps friction
//    float friction = water && !floating ? 20.0f : (pl->physstate >= PHYS_SLOPE || floating ? 6.0f : 30.0f);
//    float fpsfric = min(curtime/(20.0f*friction), 1.0f);
//    pl->vel.lerp(pl->vel, d, fpsfric);
}

void modifygravity(physent *pl, bool water, int curtime)
{
    float secs = curtime/1000.0f;
    vec g(0, 0, 0);
    if(pl->physstate == PHYS_FALL) g.z -= GRAVITY*secs;
    else if(pl->floor.z > 0 && pl->floor.z < FLOORZ)
    {
        g.z = -1;
        g.project(pl->floor);
        g.normalize();
        g.mul(GRAVITY*secs);
    }
    if(!water || !game::allowmove(pl) || (!pl->move && !pl->strafe)) pl->falling.add(g);

    if(water || pl->physstate >= PHYS_SLOPE)
    {
        float fric = water ? 2.0f : 6.0f,
              c = water ? 1.0f : clamp((pl->floor.z - SLOPEZ)/(FLOORZ-SLOPEZ), 0.0f, 1.0f);
        pl->falling.mul(pow(1 - c/fric, curtime/20.0f));
// old fps friction
//        float friction = water ? 2.0f : 6.0f,
//              fpsfric = friction/curtime*20.0f,
//              c = water ? 1.0f : clamp((pl->floor.z - SLOPEZ)/(FLOORZ-SLOPEZ), 0.0f, 1.0f);
//        pl->falling.mul(1 - c/fpsfric);
    }
}

// main physics routine, moves a player/monster for a curtime step
// moveres indicated the physics precision (which is lower for monsters and multiplayer prediction)
// local is false for multiplayer prediction

bool moveplayer(physent *pl, int moveres, bool local, int curtime)
{
    int material = lookupmaterial(vec(pl->o.x, pl->o.y, pl->o.z + (3*pl->aboveeye - pl->eyeheight)/4));
    bool water = isliquid(material&MATF_VOLUME);
    bool floating =(pl->type==ENT_PLAYER && (pl->state==CS_EDITING || pl->state==CS_SPECTATOR)) || // INTENSITY: Added second line
                   (pl->type==ENT_CAMERA && editmode && thirdperson && pl == camera1);             // so editmode camera works ok in 3rdp

    float secs = curtime/1000.f;

    // apply gravity
    if(!floating) modifygravity(pl, water, curtime);
    // apply any player generated changes in velocity
    modifyvelocity(pl, local, water, floating, curtime);

    vec d(pl->vel), oldpos(pl->o);
    if(!floating && water) d.mul(0.5f);
    d.add(pl->falling);
    d.mul(secs);

    pl->blocked = false;
    pl->moving = true;
    pl->onplayer = NULL;

    if(floating)                // just apply velocity
    {
        if(pl->physstate != PHYS_FLOAT)
        {
            pl->physstate = PHYS_FLOAT;
            pl->timeinair = 0;
            pl->falling = vec(0, 0, 0);
        }
        pl->o.add(d);
    }
    else                        // apply velocity with collision
    {
        const float f = 1.0f/moveres;
        const int timeinair = pl->timeinair;
        int collisions = 0;

        d.mul(f);
        loopi(moveres) if(!move(pl, d) && ++collisions<5) i--; // discrete steps collision detection & sliding
        if(timeinair > 800 && !pl->timeinair && !water) // if we land after long time must have been a high jump, make thud sound
        {
            game::physicstrigger(pl, local, -1, 0);
        }
    }

    if(pl->state==CS_ALIVE) updatedynentcache(pl);

    if(!pl->timeinair && pl->physstate >= PHYS_FLOOR && pl->vel.squaredlen() < 1e-4f) pl->moving = false;

    pl->lastmoveattempt = lastmillis;
    if(pl->o!=oldpos) pl->lastmove = lastmillis;

    // automatically apply smooth roll when strafing

    if(pl->strafe==0)
    {
        pl->roll = pl->roll/(1+(float)sqrtf((float)curtime)/25);
    }
    else
    {
        pl->roll -= pl->strafe*curtime*straferoll;
        if(pl->roll > maxroll) pl->roll = maxroll;
        else if(pl->roll < -maxroll) pl->roll = -maxroll;
    }

    // play sounds on water transitions

    if(pl->inwater && !water)
    {
        material = lookupmaterial(vec(pl->o.x, pl->o.y, pl->o.z + (pl->aboveeye - pl->eyeheight)/2));
        water = isliquid(material&MATF_VOLUME);
    }
    if(!pl->inwater && water) game::physicstrigger(pl, local, 0, -1, material&MATF_VOLUME);
    else if(pl->inwater && !water) game::physicstrigger(pl, local, 0, 1, pl->inwater);
    pl->inwater = water ? material&MATF_VOLUME : MAT_AIR;

#if 0 // INTENSITY: Use our own system of triggers/events for falling off map or into deadly materials
    if(pl->state==CS_ALIVE && (pl->o.z < 0 || material&MAT_DEATH)) game::suicide(pl);
#else
    if (pl->o.z < 0)
    {
        ScriptValuePtr scriptEntity = LogicSystem::getLogicEntity((dynent*)pl).get()->scriptEntity;

        ScriptEngineManager::getGlobal()->getProperty("ApplicationManager")->getProperty("instance")->call(
#ifdef CLIENT // INTENSITY
                "clientOnEntityOffMap",
#else // SERVER
                "onEntityOffMap",
#endif
            scriptEntity
        );
    }
#endif

    return true;
}

#define PHYSFRAMETIME 5

int physsteps = 0, physframetime = PHYSFRAMETIME, lastphysframe = 0;

void physicsframe()          // optimally schedule physics frames inside the graphics frames
{
    int diff = lastmillis - lastphysframe;
    if(diff <= 0) physsteps = 0;
    else
    {
        extern int gamespeed;
        physframetime = clamp((PHYSFRAMETIME*gamespeed)/100, 1, PHYSFRAMETIME);
        physsteps = (diff + physframetime - 1)/physframetime;
        lastphysframe += physsteps * physframetime;
    }
    cleardynentcache();
}

VAR(physinterp, 0, 1, 1);

void interppos(physent *pl)
{
    pl->o = pl->newpos;

    int diff = lastphysframe - lastmillis;
    if(diff <= 0 || !physinterp) return;
        
    vec deltapos(pl->deltapos);
    deltapos.mul(min(diff, physframetime)/float(physframetime));
    pl->o.add(deltapos);
}

void moveplayer(physent *pl, int moveres, bool local)
{
    // INTENSITY: Don't move an entity not fully set up yet
    if (!pl || !LogicSystem::getLogicEntity(pl).get()) return;

    // INTENSITY: Calculate how many physics frames, on a per-entity basis
    TargetingControl::calcPhysicsFrames(pl);

    // INTENSITY: Per-entity frame times
    fpsent* fpsEntity = dynamic_cast<fpsent*>(pl);
    physsteps = fpsEntity->physsteps, physframetime = fpsEntity->physframetime, lastphysframe = fpsEntity->lastphysframe;
    // INTENSITY

    if(physsteps <= 0)
    {
        if(local) interppos(pl);
        return;
    }
        
    if(local) pl->o = pl->newpos; // Kripken: Note: with this active, position updates from the server
                                  // get overwritten, unless we also change newpos.

    loopi(physsteps-1) moveplayer(pl, moveres, local, physframetime);
    if(local) pl->deltapos = pl->o;
    moveplayer(pl, moveres, local, physframetime);
    if(local)
    {
        pl->newpos = pl->o;
        pl->deltapos.sub(pl->newpos);
        interppos(pl);
    }
}

bool bounce(physent *d, float elasticity, float waterfric)
{
    if(physsteps <= 0) 
    {
        interppos(d);
        return false;
    }

    d->o = d->newpos;
    bool hitplayer = false;
    loopi(physsteps-1)
    {
        if(bounce(d, physframetime/1000.0f, elasticity, waterfric)) hitplayer = true;
    }
    d->deltapos = d->o;
    if(bounce(d, physframetime/1000.0f, elasticity, waterfric)) hitplayer = true; 
    d->newpos = d->o;
    d->deltapos.sub(d->newpos);
    interppos(d);
    return hitplayer;
}

void updatephysstate(physent *d)
{
    if(d->physstate == PHYS_FALL) return;
    d->timeinair = 0;
    vec old(d->o);
    /* Attempt to reconstruct the floor state.
     * May be inaccurate since movement collisions are not considered.
     * If good floor is not found, just keep the old floor and hope it's correct enough.
     */
    switch(d->physstate)
    {
        case PHYS_SLOPE:
        case PHYS_FLOOR:
            d->o.z -= 0.15f;
            if(!collide(d, vec(0, 0, -1), d->physstate == PHYS_SLOPE ? SLOPEZ : FLOORZ))
                d->floor = wall;
            break;

        case PHYS_STEP_UP:
            d->o.z -= STAIRHEIGHT+0.15f;
            if(!collide(d, vec(0, 0, -1), SLOPEZ))
                d->floor = wall;
            break;

        case PHYS_SLIDE:
            d->o.z -= 0.15f;
            if(!collide(d, vec(0, 0, -1)) && wall.z < SLOPEZ)
                d->floor = wall;
            break;
    }
    if(d->physstate > PHYS_FALL && d->floor.z <= 0) d->floor = vec(0, 0, 1);
    d->o = old;
}

const float PLATFORMMARGIN = 0.2f;
const float PLATFORMBORDER = 10.0f;

struct platformcollision
{
    physent *d;
    int next;

    platformcollision() {}
    platformcollision(physent *d, int next) : d(d), next(next) {}
};

bool platformcollide(physent *d, physent *o, const vec &dir, float margin = 0)
{
    if(d->collidetype!=COLLIDE_ELLIPSE || o->collidetype!=COLLIDE_ELLIPSE)
    {
        if(rectcollide(d, dir, o->o,
            o->collidetype==COLLIDE_ELLIPSE ? o->radius : o->xradius,
            o->collidetype==COLLIDE_ELLIPSE ? o->radius : o->yradius, o->aboveeye,
            o->eyeheight + margin))
            return true;
    }
    else if(ellipsecollide(d, dir, o->o, vec(0, 0, 0), o->yaw, o->xradius, o->yradius, o->aboveeye, o->eyeheight + margin))
        return true;
    return false;
}

bool moveplatform(physent *p, const vec &dir)
{
    if(!insideworld(p->newpos)) return false;

    vec oldpos(p->o);
    (p->o = p->newpos).add(dir);
    if(!collide(p, dir, 0, dir.z<=0))
    {
        p->o = oldpos;
        return false;
    }
    p->o = oldpos;

    static vector<physent *> candidates;
    candidates.setsizenodelete(0);
    for(int x = int(max(p->o.x-p->radius-PLATFORMBORDER, 0.0f))>>dynentsize, ex = int(min(p->o.x+p->radius+PLATFORMBORDER, worldsize-1.0f))>>dynentsize; x <= ex; x++)
    for(int y = int(max(p->o.y-p->radius-PLATFORMBORDER, 0.0f))>>dynentsize, ey = int(min(p->o.y+p->radius+PLATFORMBORDER, worldsize-1.0f))>>dynentsize; y <= ey; y++)
    {
        const vector<physent *> &dynents = checkdynentcache(x, y);
        loopv(dynents)
        {
            physent *d = dynents[i];
            if(p==d || d->o.z-d->eyeheight < p->o.z+p->aboveeye || p->o.reject(d->o, p->radius+PLATFORMBORDER+d->radius) || candidates.find(d) >= 0) continue;
            candidates.add(d);
            d->stacks = d->collisions = -1;
        }
    }
    static vector<physent *> passengers, colliders;
    passengers.setsizenodelete(0);
    colliders.setsizenodelete(0);
    static vector<platformcollision> collisions;
    collisions.setsizenodelete(0);
    // build up collision DAG of colliders to be pushed off, and DAG of stacked passengers
    loopv(candidates)
    {
        physent *d = candidates[i];
        // check if the dynent is on top of the platform
        if(!platformcollide(p, d, vec(0, 0, 1), PLATFORMMARGIN)) passengers.add(d);
        vec doldpos(d->o);
        (d->o = d->newpos).add(dir);
        if(!collide(d, dir, 0, false)) colliders.add(d);
        d->o = doldpos;
        loopvj(candidates)
        {
            physent *o = candidates[j];
            if(!platformcollide(d, o, dir))
            {
                collisions.add(platformcollision(d, o->collisions));
                o->collisions = collisions.length() - 1;
            }
            if(d->o.z < o->o.z && !platformcollide(d, o, vec(0, 0, 1), PLATFORMMARGIN))
            {
                collisions.add(platformcollision(o, d->stacks));
                d->stacks = collisions.length() - 1;
            }
        }
    }
    loopv(colliders) // propagate collisions
    {
        physent *d = colliders[i];
        for(int n = d->collisions; n>=0; n = collisions[n].next)
        {
            physent *o = collisions[n].d;
            if(colliders.find(o)<0) colliders.add(o);
        }
    }
    if(dir.z>0)
    {
        loopv(passengers) // if any stacked passengers collide, stop the platform
        {
            physent *d = passengers[i];
            if(colliders.find(d)>=0) return false;
            for(int n = d->stacks; n>=0; n = collisions[n].next)
            {
                physent *o = collisions[n].d;
                if(passengers.find(o)<0) passengers.add(o);
            }
        }
        loopv(passengers)
        {
            physent *d = passengers[i];
            d->o.add(dir);
            d->newpos.add(dir);
            d->lastmove = lastmillis;
            if(dir.x || dir.y) updatedynentcache(d);
        }
    }
    else loopv(passengers) // move any stacked passengers who aren't colliding with non-passengers
    {
        physent *d = passengers[i];
        if(colliders.find(d)>=0) continue;

        d->o.add(dir);
        d->newpos.add(dir);
        d->lastmove = lastmillis;
        if(dir.x || dir.y) updatedynentcache(d);

        for(int n = d->stacks; n>=0; n = collisions[n].next)
        {
            physent *o = collisions[n].d;
            if(passengers.find(o)<0) passengers.add(o);
        }
    }

    p->o.add(dir);
    p->newpos.add(dir);
    p->lastmove = lastmillis;
    if(dir.x || dir.y) updatedynentcache(p);

    return true;
}

#if 0 // INTENSITY: Replaced in our code
#define dir(name,v,d,s,os) ICOMMAND(name, "D", (int *down), \
    { player->s = *down!=0; player->v = player->s ? d : (player->os ? -(d) : 0); } \
);

dir(backward, move,   -1, k_down,  k_up);
dir(forward,  move,    1, k_up,    k_down);
dir(left,     strafe,  1, k_left,  k_right);
dir(right,    strafe, -1, k_right, k_left);

ICOMMAND(jump,   "D", (int *down), { if(!*down || game::canjump()) player->jumping = *down!=0; });
ICOMMAND(attack, "D", (int *down), { game::doattack(*down!=0); });
#endif

bool entinmap(dynent *d, bool avoidplayers)        // brute force but effective way to find a free spawn spot in the map
{
    d->o.z += d->eyeheight; // pos specified is at feet
    vec orig = d->o;
    loopi(999)              // try max 100 times // INTENSITY: Raised to 999 - we really want this to succeed
    {
        if(i)
        {
            d->o = orig;
            d->o.x += (rnd(21)-10)*i/5;  // increasing distance
            d->o.y += (rnd(21)-10)*i/5;
            d->o.z += (rnd(21)-10)*i/5;
        }   

        if(collide(d) && !inside)
        {
            if(hitplayer)
            {
                if(!avoidplayers) continue;
                d->o = orig;
                d->resetinterp();

                return false;
            }

            d->resetinterp();
            return true;
        }
    }
    // leave ent at original pos, possibly stuck
    d->o = orig;
    d->resetinterp();
    conoutf(CON_WARN, "can't find entity spawn spot! (%.1f, %.1f, %.1f)", d->o.x, d->o.y, d->o.z);
    return false;
}

