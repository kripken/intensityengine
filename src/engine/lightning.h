#define MAXLIGHTNINGSTEPS 64
#define LIGHTNINGSTEP 8
int lnjitterx[MAXLIGHTNINGSTEPS], lnjittery[MAXLIGHTNINGSTEPS];
int lastlnjitter = 0;

VAR(lnjittermillis, 0, 100, 1000);
VAR(lnjitterradius, 0, 2, 100);

static void setuplightning()
{
    if(lastmillis-lastlnjitter > lnjittermillis)
    {
        lastlnjitter = lastmillis - (lastmillis%lnjittermillis);
        loopi(MAXLIGHTNINGSTEPS)
        {
            lnjitterx[i] = -lnjitterradius + rnd(2*lnjitterradius + 1);
            lnjittery[i] = -lnjitterradius + rnd(2*lnjitterradius + 1);
        }
    }
}

static void renderlightning(const vec &o, const vec &d, float sz, float tx, float ty, float tsz)
{
    vec step(d);
    step.sub(o);
    float len = step.magnitude();
    int numsteps = clamp(int(ceil(len/LIGHTNINGSTEP)), 2, MAXLIGHTNINGSTEPS);
    step.div(numsteps+1);
    int jitteroffset = detrnd(int(o.x+o.y+o.z), MAXLIGHTNINGSTEPS);
    vec cur(o), up, right;
    up.orthogonal(step);
    up.normalize();
    right.cross(up, step);
    right.normalize();
    glBegin(GL_QUAD_STRIP);
    loopj(numsteps)
    {
        vec next(cur);
        next.add(step);
        if(j+1==numsteps) next = d;
        else
        {
            next.add(vec(right).mul(sz*lnjitterx[(j+jitteroffset)%MAXLIGHTNINGSTEPS]));
            next.add(vec(up).mul(sz*lnjittery[(j+jitteroffset)%MAXLIGHTNINGSTEPS]));
        }
        vec dir1 = next, dir2 = next, across;
        dir1.sub(cur);
        dir2.sub(camera1->o);
        across.cross(dir2, dir1).normalize().mul(sz);
        float tx1 = j&1 ? tx : tx+tsz, tx2 = j&1 ? tx+tsz : tx;
        glTexCoord2f(tx2, ty+tsz); glVertex3f(cur.x-across.x, cur.y-across.y, cur.z-across.z);
        glTexCoord2f(tx2, ty);     glVertex3f(cur.x+across.x, cur.y+across.y, cur.z+across.z);
        if(j+1==numsteps)
        {
            glTexCoord2f(tx1, ty+tsz); glVertex3f(next.x-across.x, next.y-across.y, next.z-across.z);
            glTexCoord2f(tx1, ty);     glVertex3f(next.x+across.x, next.y+across.y, next.z+across.z);
        }
        cur = next;
    }
    glEnd();
}

struct lightningrenderer : listrenderer
{
    lightningrenderer()
        : listrenderer("packages/particles/lightning.jpg", PT_LIGHTNING|PT_TRACK|PT_GLARE)
    {}

    void startrender()
    {
        glDisable(GL_CULL_FACE);
    }

    void endrender()
    {
        glEnable(GL_CULL_FACE);
    }

    void update()
    {
        setuplightning();
    }

    void seedemitter(particleemitter &pe, const vec &o, const vec &d, int fade, float size, int gravity)
    {
        pe.maxfade = max(pe.maxfade, fade);
        pe.extendbb(o, size);
        pe.extendbb(d, size);
    }

    void renderpart(listparticle *p, const vec &o, const vec &d, int blend, int ts, uchar *color)
    {
        blend = min(blend<<2, 255);
        if(type&PT_MOD) //multiply alpha into color
            glColor3ub((color[0]*blend)>>8, (color[1]*blend)>>8, (color[2]*blend)>>8);
        else
            glColor4ub(color[0], color[1], color[2], blend);
        float tx = 0, ty = 0, tsz = 1;
        if(type&PT_RND4)
        {
            int i = detrnd((size_t)p, 4);
            tx = 0.5f*(i&1);
            ty = 0.5f*((i>>1)&1);
            tsz = 0.5f;
        }
        renderlightning(o, d, p->size, tx, ty, tsz);
    }
};
static lightningrenderer lightnings;

