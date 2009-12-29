struct ragdollskel
{
    struct vert
    {
        vec pos;
        float weight;
    };

    struct tri
    {
        int vert[3];
    };

    struct distlimit
    {
        int vert[2];
        float mindist, maxdist;
    }; 

    struct rotlimit
    {
        int tri[2];
        float maxangle;
        matrix3x3 middle;
    };

    struct joint
    {
        int bone, tri, vert[3];
        float weight;
        matrix3x4 orient;
    };

    struct reljoint
    {
        int bone, parent;
    };

    bool loaded;
    int eye;
    vector<vert> verts;
    vector<tri> tris;
    vector<distlimit> distlimits;
    vector<rotlimit> rotlimits;
    vector<joint> joints;
    vector<reljoint> reljoints;

    ragdollskel() : loaded(false), eye(-1) {}

    void setup()
    {
        loopv(verts) verts[i].weight = 0;
        loopv(joints)
        {
            joint &j = joints[i];
            j.weight = 0;
            vec pos(0, 0, 0);
            loopk(3) if(j.vert[k]>=0) 
            {
                pos.add(verts[j.vert[k]].pos);
                j.weight++;
                verts[j.vert[k]].weight++;
            }
            if(j.weight) j.weight = 1/j.weight;
            pos.mul(j.weight);

            tri &t = tris[j.tri];
            matrix3x3 m;
            const vec &v1 = verts[t.vert[0]].pos,
                      &v2 = verts[t.vert[1]].pos,
                      &v3 = verts[t.vert[2]].pos;
            m.a = vec(v2).sub(v1).normalize();
            m.c.cross(m.a, vec(v3).sub(v1)).normalize();
            m.b.cross(m.c, m.a);

            j.orient = matrix3x4(m, m.transform(pos).neg());        
        }
        loopv(verts) if(verts[i].weight) verts[i].weight = 1/verts[i].weight;
        reljoints.setsize(0);

        loaded = true;
    } 

    void addreljoint(int bone, int parent)
    {
        reljoint &r = reljoints.add();
        r.bone = bone;
        r.parent = parent;
    }
};

struct ragdolldata
{
    struct vert
    {
        vec oldpos, pos, newpos;
        float weight;
        bool collided;

        vert() : pos(0, 0, 0), newpos(0, 0, 0), weight(0), collided(false) {}
    };

    ragdollskel *skel;
    int millis, collidemillis, collisions, floating, lastmove;
    vec offset, center;
    float radius, timestep, scale;
    vert *verts;
    matrix3x3 *tris;
    matrix3x4 *reljoints;

    ragdolldata(ragdollskel *skel, float scale = 1)
        : skel(skel),
          millis(lastmillis),
          collidemillis(0),
          collisions(0),
          floating(0),
          lastmove(lastmillis),
          timestep(0),
          scale(scale),
          verts(new vert[skel->verts.length()]), 
          tris(new matrix3x3[skel->tris.length()]),
          reljoints(skel->reljoints.empty() ? NULL : new matrix3x4[skel->reljoints.length()])
    {
    }

    ~ragdolldata()
    {
        delete[] verts;
        delete[] tris;
        if(reljoints) delete[] reljoints;
    }

    void calctris()
    {
        loopv(skel->tris)
        {
            ragdollskel::tri &t = skel->tris[i];
            matrix3x3 &m = tris[i];
            const vec &v1 = verts[t.vert[0]].pos,
                      &v2 = verts[t.vert[1]].pos,
                      &v3 = verts[t.vert[2]].pos;
            m.a = vec(v2).sub(v1).normalize();
            m.c.cross(m.a, vec(v3).sub(v1)).normalize();
            m.b.cross(m.c, m.a);
        }
    }

    void calcboundsphere()
    {
        center = vec(0, 0, 0);
        loopv(skel->verts) center.add(verts[i].pos);
        center.div(skel->verts.length());
        radius = 0;
        loopv(skel->verts) radius = max(radius, verts[i].pos.dist(center));
    }

    void init(dynent *d)
    {
        float ts = curtime/1000.0f;
        loopv(skel->verts) (verts[i].oldpos = verts[i].pos).sub(vec(d->vel).add(d->falling).mul(ts));
        timestep = ts;

        calctris();
        calcboundsphere();
        offset = d->o;
        offset.sub(skel->eye >= 0 ? verts[skel->eye].pos : center);
        offset.z += (d->eyeheight + d->aboveeye)/2;
    }

    void move(dynent *pl, float ts);
    void updatepos();
    void constrain();
    void constraindist();
    void constrainrot();
};

/*
    seed particle position = avg(modelview * base2anim * spherepos)  
    mapped transform = invert(curtri) * origtrig 
    parented transform = parent{invert(curtri) * origtrig} * (invert(parent{base2anim}) * base2anim)
*/

void ragdolldata::constraindist()
{
    loopv(skel->distlimits)
    {
        ragdollskel::distlimit &d = skel->distlimits[i];
        vert &v1 = verts[d.vert[0]], &v2 = verts[d.vert[1]];
        vec dir = vec(v2.pos).sub(v1.pos);
        float dist = dir.magnitude(), cdist = dist;
        if(dist < d.mindist) cdist = d.mindist;
        else if(dist > d.maxdist) cdist = d.maxdist;
        else continue;
        if(dist < 1e-4f) dir = vec(0, 0, cdist*scale*0.5f);
        else dir.mul(cdist*scale*0.5f/dist);
        vec center = vec(v1.pos).add(v2.pos).mul(0.5f);
        v1.newpos.add(vec(center).sub(dir));
        v1.weight++;
        v2.newpos.add(vec(center).add(dir));
        v2.weight++;
    }
}
        
void ragdolldata::constrainrot()
{
    loopv(skel->rotlimits)
    {
        ragdollskel::rotlimit &r = skel->rotlimits[i];
        matrix3x3 rot;
        rot.transposemul(tris[r.tri[0]], r.middle);
        rot.mul(tris[r.tri[1]]);

        vec axis;
        float angle;
        rot.calcangleaxis(angle, axis);
        angle = r.maxangle - fabs(angle);
        if(angle >= 0) continue; 
        angle += 1e-3f;

        ragdollskel::tri &t1 = skel->tris[r.tri[0]], &t2 = skel->tris[r.tri[1]];
        vec v1[3], v2[3], c1(0, 0, 0), c2(0, 0, 0);
        loopk(3)
        {
            c1.add(v1[k] = verts[t1.vert[k]].pos);
            c2.add(v2[k] = verts[t2.vert[k]].pos);
        }
        c1.div(3);
        c2.div(3);
        matrix3x3 wrot, crot1, crot2;
        static const float wrotc = cosf(0.5f*RAD), wrots = sinf(0.5f*RAD);
        wrot.rotate(wrotc, wrots, axis);
        float w1 = 0, w2 = 0;
        loopk(3) 
        { 
            v1[k].sub(c1);
            v2[k].sub(c2);
            w1 += wrot.transform(v1[k]).dist(v1[k]); 
            w2 += wrot.transform(v2[k]).dist(v2[k]); 
        }
        crot1.rotate(angle*w2/(w1+w2), axis);
        crot2.rotate(-angle*w1/(w1+w2), axis);
        vec r1[3], r2[3], diff1(0, 0, 0), diff2(0, 0, 0);
        loopk(3) 
        { 
            r1[k] = crot1.transform(v1[k]);
            r2[k] = crot2.transform(v2[k]);
            diff1.add(r1[k]).sub(v1[k]);
            diff2.add(r2[k]).sub(v2[k]);
        }
        diff1.div(3).add(c1);
        diff2.div(3).add(c2);
        loopk(3)
        {
            verts[t1.vert[k]].newpos.add(r1[k]).add(diff1);
            verts[t2.vert[k]].newpos.add(r2[k]).add(diff2);
            verts[t1.vert[k]].weight++;
            verts[t2.vert[k]].weight++;
        }
    }
}

extern vec wall;

void ragdolldata::updatepos()
{
    static physent d;
    d.type = ENT_BOUNCE;
    d.radius = d.eyeheight = d.aboveeye = 1;
    loopv(skel->verts)
    {
        vert &v = verts[i];
        if(v.weight)
        {
            d.o = v.newpos.div(v.weight);
            if(collide(&d, vec(v.newpos).sub(v.pos), 0, false)) v.pos = v.newpos;
            else
            {
                vec dir = vec(v.newpos).sub(v.oldpos);
                if(dir.dot(wall) < 0) v.oldpos = vec(v.pos).sub(dir.reflect(wall));
                v.collided = true;
            }
        }
        v.newpos = vec(0, 0, 0);
        v.weight = 0;
    }
}

VAR(ragdollconstrain, 1, 5, 100);

void ragdolldata::constrain()
{
    loopi(ragdollconstrain)
    {
        constraindist();
        updatepos();

        calctris();
        constrainrot();
        updatepos();
    }
}

FVAR(ragdollbodyfric, 0, 0.95f, 1);
FVAR(ragdollbodyfricscale, 0, 2, 10);
FVAR(ragdollwaterfric, 0, 0.85f, 1);
FVAR(ragdollgroundfric, 0, 0.8f, 1);
FVAR(ragdollairfric, 0, 0.996f, 1);
VAR(ragdollexpireoffset, 0, 1000, 30000);
VAR(ragdollwaterexpireoffset, 0, 3000, 30000);
VAR(ragdollexpiremillis, 1, 1000, 30000);
VAR(ragdolltimestepmin, 1, 5, 50);
VAR(ragdolltimestepmax, 1, 10, 50);

void ragdolldata::move(dynent *pl, float ts)
{
    extern float GRAVITY; // INTENSITY: Removed 'const'
    float expirefric = collidemillis && lastmillis > collidemillis ? max(1 - float(lastmillis - collidemillis)/ragdollexpiremillis, 0.0f) : 1;
    if(!expirefric) return;
    if(timestep) expirefric *= ts/timestep;

    int material = lookupmaterial(vec(center.x, center.y, center.z + radius/2));
    bool water = isliquid(material&MATF_VOLUME);
    if(!pl->inwater && water) game::physicstrigger(pl, true, 0, -1, material&MATF_VOLUME);
    else if(pl->inwater && !water)
    {
        material = lookupmaterial(center);
        water = isliquid(material&MATF_VOLUME);
        if(!water) game::physicstrigger(pl, true, 0, 1, pl->inwater);
    }
    pl->inwater = water ? material&MATF_VOLUME : MAT_AIR;
    
    physent d;
    d.type = ENT_BOUNCE;
    d.radius = d.eyeheight = d.aboveeye = 1;
    float airfric = ragdollairfric + min((ragdollbodyfricscale*collisions)/skel->verts.length(), 1.0f)*(ragdollbodyfric - ragdollairfric);
    collisions = 0;
    loopv(skel->verts)
    {
        vert &v = verts[i];
        vec curpos = v.pos, dpos = vec(v.pos).sub(v.oldpos);
        dpos.mul(pow((water ? ragdollwaterfric : 1.0f) * (v.collided ? ragdollgroundfric : airfric), ts*1000.0f/ragdolltimestepmin)*expirefric);
        v.pos.z -= GRAVITY*ts*ts;
        if(water) v.pos.z += 0.25f*sinf(detrnd(size_t(this)+i, 360)*RAD + lastmillis/10000.0f*M_PI)*ts;
        v.pos.add(dpos);
        if(v.pos.z < 0) { v.pos.z = 0; curpos = v.pos; collisions++; }
        d.o = v.pos;
        vec dir = vec(v.pos).sub(curpos);
        v.collided = !collide(&d, dir, 0, false);
        if(v.collided)
        {
            v.oldpos = vec(curpos).sub(dir.reflect(wall));
            v.pos = curpos; 
            collisions++;
        }   
        else v.oldpos = curpos;
    }

    timestep = ts;
    if(collisions)
    {
        floating = 0;
        if(!collidemillis) collidemillis = lastmillis + (water ? ragdollwaterexpireoffset : ragdollexpireoffset);
    }
    else if(++floating > 1 && lastmillis < collidemillis + ragdollexpiremillis) collidemillis = 0;

    constrain();
    calctris();
    calcboundsphere();
}    

FVAR(ragdolleyesmooth, 0, 0.5f, 1);
VAR(ragdolleyesmoothmillis, 1, 250, 10000);

void moveragdoll(dynent *d)
{
    if(!curtime || !d->ragdoll) return;

    if(!d->ragdoll->collidemillis || lastmillis < d->ragdoll->collidemillis + ragdollexpiremillis)
    {
        int lastmove = d->ragdoll->lastmove;
        while(d->ragdoll->lastmove + (lastmove == d->ragdoll->lastmove ? ragdolltimestepmin : ragdolltimestepmax) <= lastmillis)
        {
            int timestep = min(ragdolltimestepmax, lastmillis - d->ragdoll->lastmove);
            d->ragdoll->move(d, timestep/1000.0f);
            d->ragdoll->lastmove += timestep;
        }
    }

    vec eye = d->ragdoll->skel->eye >= 0 ? d->ragdoll->verts[d->ragdoll->skel->eye].pos : d->ragdoll->center;
    eye.add(d->ragdoll->offset);
    float k = pow(ragdolleyesmooth, float(curtime)/ragdolleyesmoothmillis);
    d->o.mul(k).add(eye.mul(1-k));
}

void cleanragdoll(dynent *d)
{
    DELETEP(d->ragdoll);
}

