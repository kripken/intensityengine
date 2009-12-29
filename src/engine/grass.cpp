#include "engine.h"

VARP(grass, 0, 0, 1);
VAR(dbggrass, 0, 0, 1);
VARP(grassdist, 0, 256, 10000);
FVARP(grasstaper, 0, 0.2, 1);
FVARP(grassstep, 0.5, 2, 8);
VARP(grassheight, 1, 4, 64);

#define NUMGRASSWEDGES 8

static struct grasswedge
{
    vec dir, edge1, edge2;
    plane bound1, bound2;

    grasswedge(int i) :
      dir(2*M_PI*(i+0.5f)/float(NUMGRASSWEDGES), 0),
      edge1(vec(2*M_PI*i/float(NUMGRASSWEDGES), 0).div(cos(M_PI/NUMGRASSWEDGES))),
      edge2(vec(2*M_PI*(i+1)/float(NUMGRASSWEDGES), 0).div(cos(M_PI/NUMGRASSWEDGES))),
      bound1(vec(2*M_PI*(i/float(NUMGRASSWEDGES) - 0.5f), 0), 0),
      bound2(vec(2*M_PI*((i+1)/float(NUMGRASSWEDGES) + 0.5f), 0), 0)
    {}
} grasswedges[NUMGRASSWEDGES] = { 0, 1, 2, 3, 4, 5, 6, 7 };

struct grassvert
{
    vec pos;
    uchar color[4];
    float u, v, lmu, lmv;
};

static vector<grassvert> grassverts;

struct grassgroup
{
    const grasstri *tri;
    float dist;
    int tex, lmtex, offset, numquads;
};

static vector<grassgroup> grassgroups;

#define NUMGRASSOFFSETS 32

static float grassoffsets[NUMGRASSOFFSETS] = { -1 }, grassanimoffsets[NUMGRASSOFFSETS];
static int lastgrassanim = -1;

VARR(grassanimmillis, 0, 3000, 60000);
FVARR(grassanimscale, 0, 0.03f, 1);

static void animategrass()
{
    loopi(NUMGRASSOFFSETS) grassanimoffsets[i] = grassanimscale*sinf(2*M_PI*(grassoffsets[i] + lastmillis/float(grassanimmillis)));
    lastgrassanim = lastmillis;
}

static inline bool clipgrassquad(const grasstri &g, vec &p1, vec &p2)
{
    loopi(g.numv)
    {
        float dist1 = g.e[i].dist(p1), dist2 = g.e[i].dist(p2);
        if(dist1 <= 0)
        {
            if(dist2 <= 0) return false;
            p1.add(vec(p2).sub(p1).mul(dist1 / (dist1 - dist2)));
        }
        else if(dist2 <= 0)
            p2.add(vec(p1).sub(p2).mul(dist2 / (dist2 - dist1)));
    }
    return true;
}

VARR(grassscale, 1, 2, 64);
 
static void gengrassquads(grassgroup *&group, const grasswedge &w, const grasstri &g, Texture *tex)
{
    float t = camera1->o.dot(w.dir);
    int tstep = int(ceil(t/grassstep));
    float tstart = tstep*grassstep, tfrac = tstart - t;

    float t1 = w.dir.dot(g.v[0]), t2 = w.dir.dot(g.v[1]), t3 = w.dir.dot(g.v[2]),
          tmin = min(t1, min(t2, t3)),
          tmax = max(t1, max(t2, t3));
    if(g.numv>3)
    {
        float t4 = w.dir.dot(g.v[3]);
        tmin = min(tmin, t4);
        tmax = max(tmax, t4);
    }
 
    if(tmax < tstart || tmin > t + grassdist) return;

    int minstep = max(int(ceil(tmin/grassstep)) - tstep, 1),
        maxstep = int(floor(min(tmax, t + grassdist)/grassstep)) - tstep,
        numsteps = maxstep - minstep + 1;

    float texscale = (grassscale*tex->ys)/float(grassheight*tex->xs), animscale = grassheight*texscale;
    vec tc;
    tc.cross(g.surface, w.dir).mul(texscale);

    int color = tstep + maxstep;
    if(color < 0) color = NUMGRASSOFFSETS - (-color)%NUMGRASSOFFSETS;
    color += numsteps + NUMGRASSOFFSETS - numsteps%NUMGRASSOFFSETS;

    float taperdist = grassdist*grasstaper,
          taperscale = 1.0f / (grassdist - taperdist);

    for(int i = maxstep; i >= minstep; i--, color--)
    {
        float dist = i*grassstep + tfrac;
        vec p1 = vec(w.edge1).mul(dist).add(camera1->o),
            p2 = vec(w.edge2).mul(dist).add(camera1->o);
        p1.z = g.surface.zintersect(p1);
        p2.z = g.surface.zintersect(p2);

        if(!clipgrassquad(g, p1, p2)) continue;

        if(!group)
        {
            group = &grassgroups.add();
            group->tri = &g;
            group->tex = tex->id;
            group->lmtex = lightmaptexs.inrange(g.lmid) ? lightmaptexs[g.lmid].id : notexture->id;
            group->offset = grassverts.length();
            group->numquads = 0;
            if(lastgrassanim!=lastmillis) animategrass();
        }
  
        group->numquads++;
 
        float offset = grassoffsets[color%NUMGRASSOFFSETS],
              animoffset = animscale*grassanimoffsets[color%NUMGRASSOFFSETS],
              tc1 = tc.dot(p1) + offset, tc2 = tc.dot(p2) + offset,
              lm1u = g.tcu.dot(p1), lm1v = g.tcv.dot(p1),
              lm2u = g.tcu.dot(p2), lm2v = g.tcv.dot(p2),
              fade = dist > taperdist ? (grassdist - dist)*taperscale : 1,
              height = grassheight * fade;
        uchar color[4] = { 255, 255, 255, uchar(fade*255) };

        #define GRASSVERT(n, tcv, modify) { \
            grassvert &gv = grassverts.add(); \
            gv.pos = p##n; \
            memcpy(gv.color, color, sizeof(color)); \
            gv.u = tc##n; gv.v = tcv; \
            gv.lmu = lm##n##u; gv.lmv = lm##n##v; \
            modify; \
        }
    
        GRASSVERT(2, 0, { gv.pos.z += height; gv.u += animoffset; });
        GRASSVERT(1, 0, { gv.pos.z += height; gv.u += animoffset; });
        GRASSVERT(1, 1, );
        GRASSVERT(2, 1, );
    }
}             

static void gengrassquads(vtxarray *va)
{
    loopv(*va->grasstris)
    {
        grasstri &g = (*va->grasstris)[i];
        if(isvisiblesphere(g.radius, g.center) >= VFC_FOGGED) continue;
        float dist = g.center.dist(camera1->o);
        if(dist - g.radius > grassdist) continue;
            
        Slot &s = lookuptexture(g.texture, false);
        if(!s.grasstex) 
        {
            if(!s.autograss) continue;
            s.grasstex = textureload(s.autograss, 2);
        }

        grassgroup *group = NULL;
        loopi(NUMGRASSWEDGES)
        {
            grasswedge &w = grasswedges[i];    
            if(w.bound1.dist(g.center) > g.radius || w.bound2.dist(g.center) > g.radius) continue;
            gengrassquads(group, w, g, s.grasstex);
        }
        if(group) group->dist = dist;
    }
}

static inline int comparegrassgroups(const grassgroup *x, const grassgroup *y)
{
    if(x->dist > y->dist) return -1;
    else if(x->dist < y->dist) return 1;
    else return 0;
}

void generategrass()
{
    if(!grass || !grassdist) return;

    grassgroups.setsizenodelete(0);
    grassverts.setsizenodelete(0);

    if(grassoffsets[0] < 0) loopi(NUMGRASSOFFSETS) grassoffsets[i] = rnd(0x1000000)/float(0x1000000);

    loopi(NUMGRASSWEDGES)
    {
        grasswedge &w = grasswedges[i];
        w.bound1.offset = -camera1->o.dot(w.bound1);
        w.bound2.offset = -camera1->o.dot(w.bound2);
    }

    extern vtxarray *visibleva;
    for(vtxarray *va = visibleva; va; va = va->next)
    {
        if(!va->grasstris || va->occluded >= OCCLUDE_GEOM) continue;
        if(va->distance > grassdist) continue;
        if(reflecting || refracting>0 ? va->o.z+va->size<reflectz : va->o.z>=reflectz) continue;
        gengrassquads(va);
    }

    grassgroups.sort(comparegrassgroups);
}

void rendergrass()
{
    if(!grass || !grassdist || grassgroups.empty() || dbggrass) return;

    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    SETSHADER(grass);

    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, sizeof(grassvert), grassverts[0].pos.v);

    glEnableClientState(GL_COLOR_ARRAY);
    glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(grassvert), grassverts[0].color);

    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glTexCoordPointer(2, GL_FLOAT, sizeof(grassvert), &grassverts[0].u);

    if(renderpath!=R_FIXEDFUNCTION || maxtmus>=2)
    {
        glActiveTexture_(GL_TEXTURE1_ARB);
        glClientActiveTexture_(GL_TEXTURE1_ARB);
        glEnable(GL_TEXTURE_2D);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glTexCoordPointer(2, GL_FLOAT, sizeof(grassvert), &grassverts[0].lmu);
        if(renderpath==R_FIXEDFUNCTION) setuptmu(1, "P * T x 2"); 
        glClientActiveTexture_(GL_TEXTURE0_ARB);
        glActiveTexture_(GL_TEXTURE0_ARB);
    }

    int texid = -1, lmtexid = -1;
    loopv(grassgroups)
    {
        grassgroup &g = grassgroups[i];

        if(reflecting || refracting)
        {
            if(refracting < 0 ?
                min(g.tri->numv>3 ? min(g.tri->v[0].z, g.tri->v[3].z) : g.tri->v[0].z, min(g.tri->v[1].z, g.tri->v[2].z)) > reflectz :
                max(g.tri->numv>3 ? max(g.tri->v[0].z, g.tri->v[3].z) : g.tri->v[0].z, max(g.tri->v[1].z, g.tri->v[2].z)) + grassheight < reflectz) 
                continue;
            if(isvisiblesphere(g.tri->radius, g.tri->center) >= VFC_FOGGED) continue;
        }

        if(texid != g.tex)
        {
            glBindTexture(GL_TEXTURE_2D, g.tex);
            texid = g.tex;
        }
        if(lmtexid != g.lmtex)
        {
            if(renderpath!=R_FIXEDFUNCTION || maxtmus>=2)
            {
                glActiveTexture_(GL_TEXTURE1_ARB);
                glBindTexture(GL_TEXTURE_2D, g.lmtex);
                glActiveTexture_(GL_TEXTURE0_ARB);
            }
            lmtexid = g.lmtex;
        }

        glDrawArrays(GL_QUADS, g.offset, 4*g.numquads);
        xtravertsva += 4*g.numquads;
    }

    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);

    if(renderpath!=R_FIXEDFUNCTION || maxtmus>=2)
    {
        glActiveTexture_(GL_TEXTURE1_ARB);
        glClientActiveTexture_(GL_TEXTURE1_ARB);
        if(renderpath==R_FIXEDFUNCTION) resettmu(1);
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        glDisable(GL_TEXTURE_2D);
        glClientActiveTexture_(GL_TEXTURE0_ARB);
        glActiveTexture_(GL_TEXTURE0_ARB);
    }

    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
    glEnable(GL_CULL_FACE);
}

