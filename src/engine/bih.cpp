#include "engine.h"

bool BIH::triintersect(tri &t, const vec &o, const vec &ray, float maxdist, float &dist, int mode, tri *noclip)
{
    vec p;
    p.cross(ray, t.c);
    float det = t.b.dot(p);
    if(det == 0) return false;
    vec r(o); 
    r.sub(t.a);
    float u = r.dot(p) / det; 
    if(u < 0 || u > 1) return false;
    vec q; 
    q.cross(r, t.b);
    float v = ray.dot(q) / det;
    if(v < 0 || u + v > 1) return false;
    float f = t.c.dot(q) / det;
    if(f < 0 || f > maxdist) return false;
    if(!(mode&RAY_SHADOW) && &t >= noclip) return false;
    if(t.tex && (mode&RAY_ALPHAPOLY)==RAY_ALPHAPOLY && (t.tex->alphamask || (loadalphamask(t.tex), t.tex->alphamask)))
    {
        int si = clamp(int(t.tex->xs * (t.tc[0] + u*(t.tc[2] - t.tc[0]) + v*(t.tc[4] - t.tc[0]))), 0, t.tex->xs-1),
            ti = clamp(int(t.tex->ys * (t.tc[1] + u*(t.tc[3] - t.tc[1]) + v*(t.tc[5] - t.tc[1]))), 0, t.tex->ys-1);
        if(!(t.tex->alphamask[ti*((t.tex->xs+7)/8) + si/8] & (1<<(si%8)))) return false;
    }
    dist = f;
    return true;
}

struct BIHStack
{
    BIHNode *node;
    float tmin, tmax;
};

bool BIH::traverse(const vec &o, const vec &ray, float maxdist, float &dist, int mode)
{
    if(!numnodes) return false;

    vec invray(ray.x ? 1/ray.x : 1e16f, ray.y ? 1/ray.y : 1e16f, ray.z ? 1/ray.z : 1e16f);
    float tmin, tmax;
    float t1 = (bbmin.x - o.x)*invray.x, 
          t2 = (bbmax.x - o.x)*invray.x;
    if(invray.x > 0) { tmin = t1; tmax = t2; } else { tmin = t2; tmax = t1; }
    t1 = (bbmin.y - o.y)*invray.y;
    t2 = (bbmax.y - o.y)*invray.y;
    if(invray.y > 0) { tmin = max(tmin, t1); tmax = min(tmax, t2); } else { tmin = max(tmin, t2); tmax = min(tmax, t1); }
    t1 = (bbmin.z - o.z)*invray.z;
    t2 = (bbmax.z - o.z)*invray.z;
    if(invray.z > 0) { tmin = max(tmin, t1); tmax = min(tmax, t2); } else { tmin = max(tmin, t2); tmax = min(tmax, t1); }
    if(tmin >= maxdist || tmin>=tmax) return false;
    tmax = min(tmax, maxdist);

    static vector<BIHStack> stack;
    stack.setsizenodelete(0);

    ivec order(ray.x>0 ? 0 : 1, ray.y>0 ? 0 : 1, ray.z>0 ? 0 : 1);
    BIHNode *curnode = &nodes[0];
    for(;;)
    {
        int axis = curnode->axis();
        int nearidx = order[axis], faridx = nearidx^1;
        float nearsplit = (curnode->split[nearidx] - o[axis])*invray[axis],
              farsplit = (curnode->split[faridx] - o[axis])*invray[axis];

        if(nearsplit <= tmin)
        {
            if(farsplit < tmax)
            {
                if(!curnode->isleaf(faridx))
                {
                    curnode = &nodes[curnode->childindex(faridx)];
                    tmin = max(tmin, farsplit);
                    continue;
                }
                else if(triintersect(tris[curnode->childindex(faridx)], o, ray, maxdist, dist, mode, noclip)) return true;
            }
        }
        else if(curnode->isleaf(nearidx))
        {
            if(triintersect(tris[curnode->childindex(nearidx)], o, ray, maxdist, dist, mode, noclip)) return true;
            if(farsplit < tmax)
            {
                if(!curnode->isleaf(faridx))
                {
                    curnode = &nodes[curnode->childindex(faridx)];
                    tmin = max(tmin, farsplit);
                    continue;
                }
                else if(triintersect(tris[curnode->childindex(faridx)], o, ray, maxdist, dist, mode, noclip)) return true;
            }
        }
        else
        {
            if(farsplit < tmax)
            {
                if(!curnode->isleaf(faridx))
                {
                    BIHStack &save = stack.add();
                    save.node = &nodes[curnode->childindex(faridx)];
                    save.tmin = max(tmin, farsplit);
                    save.tmax = tmax;
                }
                else if(triintersect(tris[curnode->childindex(faridx)], o, ray, maxdist, dist, mode, noclip)) return true;
            }
            curnode = &nodes[curnode->childindex(nearidx)];
            tmax = min(tmax, nearsplit);
            continue;
        }
        if(stack.empty()) return false;
        BIHStack &restore = stack.pop();
        curnode = restore.node;
        tmin = restore.tmin;
        tmax = restore.tmax;
    }
}

static BIH::tri *sorttris = NULL;
static int sortaxis = 0;

static int bihsort(const ushort *x, const ushort *y)
{
    BIH::tri &xtri = sorttris[*x], &ytri = sorttris[*y];
    float xmin = min(xtri.a[sortaxis], min(xtri.b[sortaxis], xtri.c[sortaxis])),
          ymin = min(ytri.a[sortaxis], min(ytri.b[sortaxis], ytri.c[sortaxis]));
    if(xmin < ymin) return -1;
    if(xmin > ymin) return 1;
    return 0;
}

void BIH::build(vector<BIHNode> &buildnodes, ushort *indices, int numindices, int depth)
{
    maxdepth = max(maxdepth, depth);
   
    vec vmin(1e16f, 1e16f, 1e16f), vmax(-1e16f, -1e16f, -1e16f);
    loopi(numindices)
    {
        tri &tri = tris[indices[i]];
        loopk(3)
        {
            float amin = min(tri.a[k], min(tri.b[k], tri.c[k])),
                  amax = max(tri.a[k], max(tri.b[k], tri.c[k]));
            vmin[k] = min(vmin[k], amin);
            vmax[k] = max(vmax[k], amax);
        }
    }
    if(depth==1)
    {
        bbmin = vmin;
        bbmax = vmax;
    }

    int axis = 2;
    loopk(2) if(vmax[k] - vmin[k] > vmax[axis] - vmin[axis]) axis = k;

/*
    sorttris = tris;
    sortaxis = axis;
    qsort(indices, numindices, sizeof(ushort), (int (__cdecl *)(const void *, const void *))bihsort);
    tri &median = tris[numindices/2];
    float split = min(median.a[axis], min(median.b[axis], median.c[axis]));
*/

    float split = 0.5f*(vmax[axis] + vmin[axis]);

    float splitleft = SHRT_MIN, splitright = SHRT_MAX;
    int left, right;
    for(left = 0, right = numindices; left < right;)
    {
        tri &tri = tris[indices[left]];
        float amin = min(tri.a[axis], min(tri.b[axis], tri.c[axis])),
              amax = max(tri.a[axis], max(tri.b[axis], tri.c[axis]));
        if(max(split - amin, 0.0f) > max(amax - split, 0.0f)) 
        {
            ++left;
            splitleft = max(splitleft, amax);
        }
        else 
        { 
            --right; 
            swap(indices[left], indices[right]); 
            splitright = min(splitright, amin);
        }
    }

    if(!left || right==numindices)
    {
        sorttris = tris;
        sortaxis = axis;
        qsort(indices, numindices, sizeof(ushort), (int (__cdecl *)(const void *, const void *))bihsort);

        left = right = numindices/2;
        splitleft = SHRT_MIN;
        splitright = SHRT_MAX;
        loopi(numindices)
        {
            tri &tri = tris[indices[i]];
            if(i < left) splitleft = max(splitleft, max(tri.a[axis], max(tri.b[axis], tri.c[axis])));
            else splitright = min(splitright, min(tri.a[axis], min(tri.b[axis], tri.c[axis])));
        }
    }

    int node = buildnodes.length();
    buildnodes.add();
    buildnodes[node].split[0] = short(ceil(splitleft));
    buildnodes[node].split[1] = short(floor(splitright));

    if(left==1) buildnodes[node].child[0] = (axis<<14) | indices[0];
    else
    {
        buildnodes[node].child[0] = (axis<<14) | buildnodes.length();
        build(buildnodes, indices, left, depth+1);
    }

    if(numindices-right==1) buildnodes[node].child[1] = (1<<15) | (left==1 ? 1<<14 : 0) | indices[right];
    else 
    {
        buildnodes[node].child[1] = (left==1 ? 1<<14 : 0) | buildnodes.length();
        build(buildnodes, &indices[right], numindices-right, depth+1);
    }
}
 
BIH::BIH(vector<tri> *t)
{
    numtris = t[0].length() + t[1].length();
    if(!numtris) 
    {
        tris = NULL;
        numnodes = 0;
        nodes = NULL;
        maxdepth = 0;
        return;
    }

    tris = new tri[numtris];
    noclip = &tris[t[0].length()];
    memcpy(tris, t[0].getbuf(), t[0].length()*sizeof(tri));
    memcpy(noclip, t[1].getbuf(), t[1].length()*sizeof(tri));
    
    vector<BIHNode> buildnodes;
    ushort *indices = new ushort[numtris];
    loopi(numtris) indices[i] = i;

    maxdepth = 0;

    build(buildnodes, indices, numtris);

    delete[] indices;

    numnodes = buildnodes.length();
    nodes = new BIHNode[numnodes];
    memcpy(nodes, buildnodes.getbuf(), numnodes*sizeof(BIHNode));

    // convert tri.b/tri.c to edges
    loopi(numtris)
    {
        tri &tri = tris[i];
        tri.b.sub(tri.a);
        tri.c.sub(tri.a);
    }
}

static inline void yawray(vec &o, vec &ray, float angle)
{
    angle *= RAD;
    float c = cosf(angle), s = sinf(angle),
          ox = o.x, oy = o.y,
          rx = ray.x, ry = ray.y;
    o.x = ox*c - oy*s;
    o.y = oy*c + ox*s;
    ray.x = rx*c - ry*s;
    ray.y = ry*c + rx*s;
    ray.normalize();
}

bool mmintersect(const extentity &e, const vec &o, const vec &ray, float maxdist, int mode, float &dist)
{
    LogicEntityPtr entity = LogicSystem::getLogicEntity(e); // INTENSITY
    model *m = entity.get() ? entity->getModel() : NULL; // INTENSITY
    if(!m) return false;
    if(mode&RAY_SHADOW)
    {
        if(!m->shadow || checktriggertype(e.attr3, TRIG_COLLIDE|TRIG_DISAPPEAR)) return false;
    }
    else if((mode&RAY_ENTS)!=RAY_ENTS && !m->collide) return false;
//    if((mode&RAY_ENTS)!=RAY_ENTS && m->collisionsonlyfortriggering) return false; // INTENSITY: Might need this
    if(!m->bih && !m->setBIH()) return false;
    if(!maxdist) maxdist = 1e16f;
    vec yo(o);
    yo.sub(e.o);
    float yaw = -180.0f-(float)((e.attr1+7)-(e.attr1+7)%15);
    vec yray(ray);
    if(yaw != 0) yawray(yo, yray, yaw);
    return m->bih->traverse(yo, yray, maxdist, dist, mode);
}

