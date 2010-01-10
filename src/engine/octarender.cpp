// INTENSITY: Version: rev 1759 in sauerbraten SVN, June 3 2009. Fix issue with deleting vs, following our report
// octarender.cpp: fill vertex arrays with different cube surfaces.

#include "engine.h"

#include "intensity_texture.h"

VARF(floatvtx, 0, 0, 1, allchanged());

#define GENVERTS(type, ptr, offset, body) \
    { \
        type *f = (type *)ptr; \
        loopv(verts) \
        { \
            const vertex &v = verts[i]; \
            f->x = v.x offset; \
            f->y = v.y offset; \
            f->z = v.z offset; \
            body; \
            f++; \
        } \
    }
#define GENVERTSUV(type, ptr, offset, body) GENVERTS(type, ptr, offset, { f->u = v.u; f->v = v.v; body; })

void genverts(vector<vertex> &verts, void *buf, const vec &origin)
{
    if(renderpath==R_FIXEDFUNCTION)
    {
        if(nolights)
        {
            if(floatvtx) { GENVERTS(fvertexffc, buf, , { f->add(origin); }); }
            else { GENVERTS(vertexffc, buf, - 0x8000, { f->reserved = 0; }); }
        }
        else if(floatvtx) { GENVERTSUV(fvertexff, buf, , { f->add(origin); }); }
        else { GENVERTSUV(vertexff, buf, - 0x8000, { f->reserved = 0; }); }
    }
    else if(floatvtx) { GENVERTSUV(fvertex, buf, , { f->add(origin); f->n = v.n; }); }
    else { GENVERTSUV(vertex, buf, - 0x8000, { f->reserved = 0; f->n = v.n; }); }
}

struct vboinfo
{
    int uses;
    uchar *data;
};

static inline uint hthash(GLuint key)
{
    return key;
}

static inline bool htcmp(GLuint x, GLuint y)
{
    return x==y;
}

hashtable<GLuint, vboinfo> vbos;

VAR(printvbo, 0, 0, 1);
VARFN(vbosize, maxvbosize, 0, 1<<15, 1<<16, allchanged());

enum
{
    VBO_VBUF = 0,
    VBO_EBUF,
    VBO_SKYBUF,
    NUMVBO
};

static vector<uchar> vbodata[NUMVBO];
static vector<vtxarray *> vbovas[NUMVBO];
static int vbosize[NUMVBO];

void destroyvbo(GLuint vbo)
{
    vboinfo *exists = vbos.access(vbo);
    if(!exists) return;
    vboinfo &vbi = *exists;
    if(vbi.uses <= 0) return;
    vbi.uses--;
    if(!vbi.uses) 
    {
        if(hasVBO) glDeleteBuffers_(1, &vbo);
        else if(vbi.data) delete[] vbi.data;
        vbos.remove(vbo);
    }
}

void genvbo(int type, void *buf, int len, vtxarray **vas, int numva)
{
    GLuint vbo;
    uchar *data = NULL;
    if(hasVBO)
    {
        glGenBuffers_(1, &vbo);
        GLenum target = type==VBO_VBUF ? GL_ARRAY_BUFFER_ARB : GL_ELEMENT_ARRAY_BUFFER_ARB;
        glBindBuffer_(target, vbo);
        glBufferData_(target, len, buf, GL_STATIC_DRAW_ARB);
        glBindBuffer_(target, 0);
    }
    else
    {
        static GLuint nextvbo = 0;
        if(!nextvbo) nextvbo++; // just in case it ever wraps around
        vbo = nextvbo++;
        data = new uchar[len];
        memcpy(data, buf, len);
    }
    vboinfo &vbi = vbos[vbo]; 
    vbi.uses = numva;
    vbi.data = data;
 
    if(printvbo) conoutf(CON_DEBUG, "vbo %d: type %d, size %d, %d uses", vbo, type, len, numva);

    loopi(numva)
    {
        vtxarray *va = vas[i];
        switch(type)
        {
            case VBO_VBUF: 
                va->vbuf = vbo; 
                if(!hasVBO) va->vdata = (vertex *)(data + (size_t)va->vdata);
                break;
            case VBO_EBUF: 
                va->ebuf = vbo; 
                if(!hasVBO) va->edata = (ushort *)(data + (size_t)va->edata);
                break;
            case VBO_SKYBUF: 
                va->skybuf = vbo; 
                if(!hasVBO) va->skydata = (ushort *)(data + (size_t)va->skydata);
                break;
        }
    }
}

bool readva(vtxarray *va, ushort *&edata, uchar *&vdata)
{
    if(!va->vbuf || !va->ebuf) return false;

    edata = new ushort[3*va->tris];
    vdata = new uchar[va->verts*VTXSIZE];

    if(hasVBO)
    {
        glBindBuffer_(GL_ELEMENT_ARRAY_BUFFER_ARB, va->ebuf);
        glGetBufferSubData_(GL_ELEMENT_ARRAY_BUFFER_ARB, (size_t)va->edata, 3*va->tris*sizeof(ushort), edata);
        glBindBuffer_(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);

        glBindBuffer_(GL_ARRAY_BUFFER_ARB, va->vbuf);
        glGetBufferSubData_(GL_ARRAY_BUFFER_ARB, va->voffset*VTXSIZE, va->verts*VTXSIZE, vdata);
        glBindBuffer_(GL_ARRAY_BUFFER_ARB, 0);
        return true;
    }
    else
    {
        memcpy(edata, va->edata, 3*va->tris*sizeof(ushort));
        memcpy(vdata, (uchar *)va->vdata + va->voffset*VTXSIZE, va->verts*VTXSIZE);
        return true;
    }
}

void flushvbo(int type = -1)
{
    if(type < 0)
    {
        loopi(NUMVBO) flushvbo(i);
        return;
    }

    vector<uchar> &data = vbodata[type];
    if(data.empty()) return;
    vector<vtxarray *> &vas = vbovas[type];
    genvbo(type, data.getbuf(), data.length(), vas.getbuf(), vas.length());
    data.setsizenodelete(0);
    vas.setsizenodelete(0);
    vbosize[type] = 0;
}

uchar *addvbo(vtxarray *va, int type, int numelems, int elemsize)
{
    vbosize[type] += numelems;

    vector<uchar> &data = vbodata[type];
    vector<vtxarray *> &vas = vbovas[type];

    vas.add(va);

    int len = numelems*elemsize;
    uchar *buf = data.reserve(len).buf;
    data.advance(len);
    return buf; 
}
 
struct verthash
{
    static const int SIZE = 1<<13;
    int table[SIZE];
    vector<vertex> verts;
    vector<int> chain;

    verthash() { clearverts(); }

    void clearverts() 
    { 
        memset(table, -1, sizeof(table));
        chain.setsizenodelete(0); 
        verts.setsizenodelete(0);
    }

    int addvert(const vvec &v, short tu, short tv, const bvec &n)
    {
        const uchar *iv = (const uchar *)&v;
        uint h = 5381;
        loopl(sizeof(v)) h = ((h<<5)+h)^iv[l];
        h = h&(SIZE-1);
        for(int i = table[h]; i>=0; i = chain[i])
        {
            const vertex &c = verts[i];
            if(c.x==v.x && c.y==v.y && c.z==v.z && c.n==n)
            {
                 if(!tu && !tv) return i; 
                 if(c.u==tu && c.v==tv) return i;
            }
        }
        if(verts.length() >= USHRT_MAX) return -1;
        vertex &vtx = verts.add();
        ((vvec &)vtx) = v;
        vtx.u = tu;
        vtx.v = tv;
        vtx.n = n;
        chain.add(table[h]);
        return table[h] = verts.length()-1;
    }
};

struct sortkey
{
     ushort tex, lmid, layer, envmap;

     sortkey() {}
     sortkey(ushort tex, uchar lmid, uchar layer = LAYER_TOP, ushort envmap = EMID_NONE)
      : tex(tex), lmid(lmid), layer(layer), envmap(envmap)
     {}

     bool operator==(const sortkey &o) const { return tex==o.tex && lmid==o.lmid && layer==o.layer && envmap==o.envmap; }
};

struct sortval
{
     int unlit;
     usvector dims[6];

     sortval() : unlit(0) {}
};

static inline bool htcmp(const sortkey &x, const sortkey &y)
{
    return x == y;
}

static inline uint hthash(const sortkey &k)
{
    return k.tex + k.lmid*9741;
}

struct vacollect : verthash
{
    ivec origin;
    int size;
    hashtable<sortkey, sortval> indices;
    vector<sortkey> texs;
    vector<grasstri> grasstris;
    vector<materialsurface> matsurfs;
    vector<octaentities *> mapmodels;
    usvector skyindices, explicitskyindices;
    int worldtris, skytris, skyfaces, skyclip, skyarea;

    void clear()
    {
        clearverts();
        worldtris = skytris = 0;
        skyfaces = 0;
        skyclip = INT_MAX;
        skyarea = 0;
        indices.clear();
        skyindices.setsizenodelete(0);
        explicitskyindices.setsizenodelete(0);
        matsurfs.setsizenodelete(0);
        mapmodels.setsizenodelete(0);
        grasstris.setsizenodelete(0);
        texs.setsizenodelete(0);
    }

    void remapunlit(vector<sortkey> &remap)
    {
        uint lastlmid[4] = { LMID_AMBIENT, LMID_AMBIENT, LMID_AMBIENT, LMID_AMBIENT }, 
             firstlmid[4] = { LMID_AMBIENT, LMID_AMBIENT, LMID_AMBIENT, LMID_AMBIENT };
        int firstlit[4] = { -1, -1, -1, -1 };
        loopv(texs)
        {
            sortkey &k = texs[i];
            if(k.lmid>=LMID_RESERVED) 
            {
                LightMapTexture &lmtex = lightmaptexs[k.lmid];
                int type = lmtex.type&LM_TYPE;
                if(k.layer&LAYER_BLEND) type += 2;
                lastlmid[type] = lmtex.unlitx>=0 ? k.lmid : LMID_AMBIENT;
                if(firstlmid[type]==LMID_AMBIENT && lastlmid[type]!=LMID_AMBIENT)
                {
                    firstlit[type] = i;
                    firstlmid[type] = lastlmid[type];
                }
            }
            else if(k.lmid==LMID_AMBIENT)
            {
                Shader *s = lookuptexture(k.tex, false).shader;
                int type = s->type&SHADER_NORMALSLMS ? LM_BUMPMAP0 : LM_DIFFUSE;
                if(k.layer&LAYER_BLEND) type += 2;
                if(lastlmid[type]!=LMID_AMBIENT)
                {
                    sortval &t = indices[k];
                    if(t.unlit<=0) t.unlit = lastlmid[type];
                }
            }
        }
        loopj(2)
        {
            int offset = 2*j;
            if(firstlmid[offset]==LMID_AMBIENT && firstlmid[offset+1]==LMID_AMBIENT) continue;
            loopi(max(firstlit[offset], firstlit[offset+1]))
            {
                sortkey &k = texs[i];
                if(j ? !(k.layer&LAYER_BLEND) : k.layer&LAYER_BLEND) continue;
                if(k.lmid!=LMID_AMBIENT) continue;
                Shader *s = lookuptexture(k.tex, false).shader;
                int type = offset + (s->type&SHADER_NORMALSLMS ? LM_BUMPMAP0 : LM_DIFFUSE);
                if(firstlmid[type]==LMID_AMBIENT) continue;
                indices[k].unlit = firstlmid[type];
            }
        }  
        loopv(remap)
        {
            sortkey &k = remap[i];
            sortval &t = indices[k];
            if(t.unlit<=0) continue; 
            LightMapTexture &lm = lightmaptexs[t.unlit];
            short u = short(ceilf((lm.unlitx + 0.5f) * SHRT_MAX/lm.w)), 
                  v = short(ceilf((lm.unlity + 0.5f) * SHRT_MAX/lm.h));
            loopl(6) loopvj(t.dims[l])
            {
                vertex &vtx = verts[t.dims[l][j]];
                if(!vtx.u && !vtx.v)
                {
                    vtx.u = u;
                    vtx.v = v;
                }
                else if(vtx.u != u || vtx.v != v) 
                {
                    // necessary to copy these in case vechash reallocates verts before copying vtx
                    vvec vv = vtx;
                    bvec n = vtx.n;
                    t.dims[l][j] = addvert(vv, u, v, n);
                }
            }
            sortval *dst = indices.access(sortkey(k.tex, t.unlit, k.layer, k.envmap));
            if(dst) loopl(6) loopvj(t.dims[l]) dst->dims[l].add(t.dims[l][j]);
        }
    }
                    
    void optimize()
    {
        vector<sortkey> remap;
        enumeratekt(indices, sortkey, k, sortval, t,
            loopl(6) if(t.dims[l].length() && t.unlit<=0)
            {
                if(k.lmid>=LMID_RESERVED && lightmaptexs[k.lmid].unlitx>=0)
                {
                    sortkey ukey(k.tex, LMID_AMBIENT, k.layer, k.envmap);
                    sortval *uval = indices.access(ukey);
                    if(uval && uval->unlit<=0)
                    {
                        if(uval->unlit<0) texs.removeobj(ukey);
                        else remap.add(ukey);
                        uval->unlit = k.lmid;
                    }
                }
                else if(k.lmid==LMID_AMBIENT)
                {
                    remap.add(k);
                    t.unlit = -1;
                }
                texs.add(k);
                break;
            }
        );
        texs.sort(texsort);

        remapunlit(remap);

        matsurfs.setsize(optimizematsurfs(matsurfs.getbuf(), matsurfs.length()));
    }

    static int texsort(const sortkey *x, const sortkey *y)
    {
        if(x->layer < y->layer) return -1;
        if(x->layer > y->layer) return 1;
        if(x->tex == y->tex) 
        {
            if(x->lmid < y->lmid) return -1;
            if(x->lmid > y->lmid) return 1;
            if(x->envmap < y->envmap) return -1;
            if(x->envmap > y->envmap) return 1;
            return 0;
        }
        if(renderpath!=R_FIXEDFUNCTION)
        {
            Slot &xs = lookuptexture(x->tex, false), &ys = lookuptexture(y->tex, false);
            if(xs.shader < ys.shader) return -1;
            if(xs.shader > ys.shader) return 1;
            if(xs.params.length() < ys.params.length()) return -1;
            if(xs.params.length() > ys.params.length()) return 1;
        }
        if(x->tex < y->tex) return -1;
        else return 1;
    }

    void setupdata(vtxarray *va)
    {
        va->verts = verts.length();
        va->tris = worldtris/3;
        va->vbuf = 0;
        va->vdata = 0;
        va->minvert = 0;
        va->maxvert = va->verts-1;
        va->voffset = 0;
        if(va->verts)
        {
            if(vbovas[VBO_VBUF].length())
            {
                vtxarray *loc = vbovas[VBO_VBUF][0];
                if(((va->o.x^loc->o.x)|(va->o.y^loc->o.y)|(va->o.z^loc->o.z))&~VVEC_INT_MASK)
                    flushvbo();
            }
            if(vbosize[VBO_VBUF] + verts.length() > maxvbosize || 
               vbosize[VBO_EBUF] + worldtris > USHRT_MAX ||
               vbosize[VBO_SKYBUF] + skytris > USHRT_MAX) 
                flushvbo();

            va->voffset = vbosize[VBO_VBUF];
            uchar *vdata = addvbo(va, VBO_VBUF, va->verts, VTXSIZE);
            genverts(verts, vdata, ivec(va->o).mask(~VVEC_INT_MASK).shl(VVEC_FRAC).tovec());
            va->minvert += va->voffset;
            va->maxvert += va->voffset;
        }

        va->matbuf = NULL;
        va->matsurfs = matsurfs.length();
        if(va->matsurfs) 
        {
            va->matbuf = new materialsurface[matsurfs.length()];
            memcpy(va->matbuf, matsurfs.getbuf(), matsurfs.length()*sizeof(materialsurface));
        }

        va->skybuf = 0;
        va->skydata = 0;
        va->sky = skyindices.length();
        va->explicitsky = explicitskyindices.length();
        if(va->sky + va->explicitsky)
        {
            va->skydata += vbosize[VBO_SKYBUF];
            ushort *skydata = (ushort *)addvbo(va, VBO_SKYBUF, va->sky+va->explicitsky, sizeof(ushort));
            memcpy(skydata, skyindices.getbuf(), va->sky*sizeof(ushort));
            memcpy(skydata+va->sky, explicitskyindices.getbuf(), va->explicitsky*sizeof(ushort));
            if(va->voffset) loopi(va->sky+va->explicitsky) skydata[i] += va->voffset; 
        }

        va->eslist = NULL;
        va->texs = texs.length();
        va->blends = 0;
        va->ebuf = 0;
        va->edata = 0;
        if(va->texs)
        {
            va->eslist = new elementset[va->texs];
            va->edata += vbosize[VBO_EBUF];
            ushort *edata = (ushort *)addvbo(va, VBO_EBUF, worldtris, sizeof(ushort)), *curbuf = edata;
            while(va->texs && texs[va->texs-1].layer&LAYER_BLEND) { va->texs--; va->blends++; }
            loopv(texs)
            {
                const sortkey &k = texs[i];
                const sortval &t = indices[k];
                elementset &e = va->eslist[i];
                e.texture = k.tex;
                e.lmid = t.unlit>0 ? t.unlit : k.lmid;
                e.layer = k.layer;
                e.envmap = k.envmap;
                ushort *startbuf = curbuf;
                loopl(6) 
                {
                    e.minvert[l] = USHRT_MAX;
                    e.maxvert[l] = 0;

                    if(t.dims[l].length())
                    {
                        memcpy(curbuf, t.dims[l].getbuf(), t.dims[l].length() * sizeof(ushort));

                        loopvj(t.dims[l])
                        {
                            curbuf[j] += va->voffset;
                            e.minvert[l] = min(e.minvert[l], curbuf[j]);
                            e.maxvert[l] = max(e.maxvert[l], curbuf[j]);
                        }

                        curbuf += t.dims[l].length();
                    }
                    e.length[l] = curbuf-startbuf;
                }
                if(k.layer&LAYER_BLEND) va->tris -= e.length[5]/3;
            }
        }

        va->texmask = 0;
        loopi(va->texs+va->blends)
        {
            Slot &slot = lookuptexture(va->eslist[i].texture, false);
            loopvj(slot.sts) va->texmask |= 1<<slot.sts[j].type;
        }

        if(grasstris.length())
        {
            va->grasstris = new vector<grasstri>;
            va->grasstris->move(grasstris);
            useshaderbyname("grass");
        }

        if(mapmodels.length()) va->mapmodels = new vector<octaentities *>(mapmodels);
    }

    bool emptyva()
    {
        return verts.empty() && matsurfs.empty() && skyindices.empty() && explicitskyindices.empty() && grasstris.empty() && mapmodels.empty();
    }            
} vc;

int recalcprogress = 0;
#define progress(s)     if((recalcprogress++&0xFFF)==0) renderprogress(recalcprogress/(float)allocnodes, s);

vector<tjoint> tjoints;

vvec shadowmapmin, shadowmapmax;

int calcshadowmask(vvec *vv)
{
    extern vec shadowdir;
    int mask = 0, used = 0;
    if(vv[0]==vv[2]) return 0;
    vec v0 = vv[0].tovec(), v2 = vv[2].tovec().sub(v0);
    if(vv[0]!=vv[1] && vv[1]!=vv[2])
    {
        vec v1 = vv[1].tovec().sub(v0), n;
        n.cross(v1, v2);
        if(n.dot(shadowdir)>0) { mask |= 1; used |= 0x7; } 
    }
    if(vv[0]!=vv[3] && vv[2]!=vv[3])
    {
        vec v3 = vv[3].tovec().sub(v0), n;
        n.cross(v2, v3);
        if(n.dot(shadowdir)>0) { mask |= 2; used |= 0xD; }
    }
    if(used) loopi(4) if(used&(1<<i))
    {
        const vvec &v = vv[i];
        loopk(3)
        {
            if(v[k]<shadowmapmin[k]) shadowmapmin[k] = v[k];
            if(v[k]>shadowmapmax[k]) shadowmapmax[k] = v[k];
        }
    }
    return mask;
}

VARFP(filltjoints, 0, 1, 1, allchanged());

void reduceslope(ivec &n)
{
    int mindim = -1, minval = 64;
    loopi(3) if(n[i])
    {
        int val = abs(n[i]);
        if(mindim < 0 || val < minval)
        {
            mindim = i;
            minval = val;
        }
    }
    if(!(n[R[mindim]]%minval) && !(n[C[mindim]]%minval)) n.div(minval);
    while(!((n.x|n.y|n.z)&1)) n.shr(1);
}

struct texcoords
{
    short u, v;
};

void addtris(const sortkey &key, int orient, vvec *vv, surfacenormals *normals, texcoords tc[4], int index[4], int shadowmask, int tj)
{
    int dim = dimension(orient), &total = key.tex==DEFAULT_SKY ? vc.skytris : vc.worldtris;
    loopi(2) if(index[0]!=index[i+1] && index[i+1]!=index[i+2] && index[i+2]!=index[0])
    {
        usvector &idxs = key.tex==DEFAULT_SKY ? vc.explicitskyindices : vc.indices[key].dims[2*dim + ((shadowmask>>i)&1)];
        int left = index[2*i], mid = index[2*i + 1], right = index[(2*i + 2)%4];
        loopj(2)
        {
            int e1 = 2*i + j, edge = orient*4 + e1;
            if(tj < 0 || tjoints[tj].edge > edge) continue;
            int e2 = (e1 + 1)%4;
            texcoords &tc1 = tc[e1], &tc2 = tc[e2];
            bvec n1, n2;
            if(renderpath!=R_FIXEDFUNCTION && normals)
            {
                n1 = normals->normals[e1];
                n2 = normals->normals[e2];
            }
            else n1 = n2 = bvec(128, 128, 128);
            const vvec &vv1 = vv[e1], &vv2 = vv[e2];
            ivec d;
            loopk(3) d[k] = int(vv1[k]) - int(vv2[k]);
            int axis = abs(d.x) > abs(d.y) ? (abs(d.x) > abs(d.z) ? 0 : 2) : (abs(d.y) > abs(d.z) ? 1 : 2);
            if(d[axis] < 0) d.neg();
            reduceslope(d);
            int offset1 = vv1[axis] / d[axis], offset2 = vv2[axis] / d[axis];
            ivec o;
            loopk(3) o[k] = int(vv1[k]) - offset1*d[k];
            float doffset = 1.0f / (offset2 - offset1);

            if(j)
            {
                int tmp = right;
                right = left;
                left = mid;
                mid = tmp;
            }

            while(tj >= 0)
            {
                tjoint &t = tjoints[tj];
                if(t.edge != edge) break;
                vvec vvt;
                loopk(3) vvt[k] = ushort(o[k] + t.offset*d[k]);
                float offset = (t.offset - offset1) * doffset;
                short ut = short(tc1.u + (tc2.u-tc1.u)*offset),
                      vt = short(tc1.v + (tc2.v-tc1.v)*offset);
                bvec nt;
                loopk(3) nt[k] = uchar(n1[k] + (n2[k] - n1[k])*offset);
                int nextindex = vc.addvert(vvt, ut, vt, nt);
                if(nextindex < 0 || total + 3 > USHRT_MAX) return;
                total += 3;
                idxs.add(right);
                idxs.add(left);
                idxs.add(nextindex);
                tj = t.next;
                left = nextindex;
            }
        }

        if(total + 3 > USHRT_MAX) return;
        total += 3;
        idxs.add(right);
        idxs.add(left);
        idxs.add(mid);
    }
}

void addgrasstri(int face, vvec *vv, int numv, ushort texture, ushort lmid, texcoords tc[4])
{
    grasstri &g = vc.grasstris.add();
    int i1 = 2*face, i2 = i1+1, i3 = (i1+2)%4;
    g.v[0] = vv[i1].tovec(vc.origin);
    g.v[1] = vv[i2].tovec(vc.origin);
    g.v[2] = vv[i3].tovec(vc.origin);
    if(numv>3) g.v[3] = vv[3].tovec(vc.origin);
    g.numv = numv;

    g.surface.toplane(g.v[0], g.v[1], g.v[2]);
    if(g.surface.z <= 0) { vc.grasstris.pop(); return; }

    loopi(numv)
    {
        vec edir = g.v[(i+1)%numv];
        edir.sub(g.v[i]);
        g.e[i].cross(g.surface, edir).normalize();
        g.e[i].offset = -g.e[i].dot(g.v[i]);
    }

    g.center = vec(0, 0, 0);
    loopk(numv) g.center.add(g.v[k]);
    g.center.div(numv);
    g.radius = 0;
    loopk(numv) g.radius = max(g.radius, g.v[k].dist(g.center));

    vec area, bx, by;
    area.cross(vec(g.v[1]).sub(g.v[0]), vec(g.v[2]).sub(g.v[0]));
    float scale;
    int px, py;

    if(fabs(area.x) >= fabs(area.y) && fabs(area.x) >= fabs(area.z))
        scale = 1/area.x, px = 1, py = 2;
    else if(fabs(area.y) >= fabs(area.x) && fabs(area.y) >= fabs(area.z))
        scale = -1/area.y, px = 0, py = 2;
    else
        scale = 1/area.z, px = 0, py = 1;

    bx.x = (g.v[2][py] - g.v[0][py])*scale;
    bx.y = (g.v[2][px] - g.v[0][px])*scale;
    bx.z = bx.x*g.v[2][px] - bx.y*g.v[2][py];

    by.x = (g.v[2][py] - g.v[1][py])*scale;
    by.y = (g.v[2][px] - g.v[1][px])*scale;
    by.z = by.x*g.v[1][px] - by.y*g.v[1][py] - 1;
    by.sub(bx);

    float tc1u = tc[i1].u/float(SHRT_MAX),
          tc1v = tc[i1].v/float(SHRT_MAX),
          tc2u = (tc[i2].u - tc[i1].u)/float(SHRT_MAX),
          tc2v = (tc[i2].v - tc[i1].v)/float(SHRT_MAX),
          tc3u = (tc[i3].u - tc[i1].u)/float(SHRT_MAX),
          tc3v = (tc[i3].v - tc[i1].v)/float(SHRT_MAX);
        
    g.tcu = vec4(0, 0, 0, tc1u - (bx.z*tc2u + by.z*tc3u));
    g.tcu[px] = bx.x*tc2u + by.x*tc3u;
    g.tcu[py] = -(bx.y*tc2u + by.y*tc3u);

    g.tcv = vec4(0, 0, 0, tc1v - (bx.z*tc2v + by.z*tc3v));
    g.tcv[px] = bx.x*tc2v + by.x*tc3v;
    g.tcv[py] = -(bx.y*tc2v + by.y*tc3v);

    g.texture = texture;
    g.lmid = lmid;
}
 
void addcubeverts(int orient, int size, vvec *vv, ushort texture, surfaceinfo *surface, surfacenormals *normals, int tj = -1, ushort envmap = EMID_NONE, int grassy = 0)
{
    int index[4];
    int shadowmask = texture==DEFAULT_SKY ? 0 : calcshadowmask(vv);
    LightMap *lm = NULL;
    LightMapTexture *lmtex = NULL;
    if(!nolights && surface && lightmaps.inrange(surface->lmid-LMID_RESERVED))
    {
        lm = &lightmaps[surface->lmid-LMID_RESERVED];
        if((lm->type&LM_TYPE)==LM_DIFFUSE ||
            ((lm->type&LM_TYPE)==LM_BUMPMAP0 &&
                lightmaps.inrange(surface->lmid+1-LMID_RESERVED) &&
                (lightmaps[surface->lmid+1-LMID_RESERVED].type&LM_TYPE)==LM_BUMPMAP1))
            lmtex = &lightmaptexs[lm->tex];
        else lm = NULL;
    }
    texcoords tc[4];
    loopk(4)
    {
        if(lmtex)
        {
            tc[k].u = short(ceilf((lm->offsetx + surface->x + (surface->texcoords[k*2] / 255.0f) * (surface->w - 1) + 0.5f) * SHRT_MAX/lmtex->w));
            tc[k].v = short(ceilf((lm->offsety + surface->y + (surface->texcoords[k*2 + 1] / 255.0f) * (surface->h - 1) + 0.5f) * SHRT_MAX/lmtex->h));
        }
        else tc[k].u = tc[k].v = 0;
        index[k] = vc.addvert(vv[k], tc[k].u, tc[k].v, renderpath!=R_FIXEDFUNCTION && normals ? normals->normals[k] : bvec(128, 128, 128));
        if(index[k] < 0) return;
    }

    if(texture == DEFAULT_SKY)
    {
        loopk(4) vc.skyclip = min(vc.skyclip, int(vv[k].z>>VVEC_FRAC));
        vc.skyfaces |= 0x3F&~(1<<orient);
    }

    int lmid = LMID_AMBIENT;
    if(surface)
    {
        if(surface->lmid < LMID_RESERVED) lmid = surface->lmid;
        else if(lm) lmid = lm->tex;
    }

    sortkey key(texture, lmid, surface ? surface->layer&LAYER_BLEND : LAYER_TOP, envmap);
    addtris(key, orient, vv, normals, tc, index, shadowmask, tj);

    if(grassy) 
    {
        int faces = 0;
        loopi(2) if(index[0]!=index[i+1] && index[i+1]!=index[i+2] && index[i+2]!=index[0]) faces |= 1<<i;
        if(grassy>1 && faces==3) addgrasstri(0, vv, 4, texture, lmid, tc);
        else loopi(2) if(faces&(1<<i)) addgrasstri(i, vv, 3, texture, lmid, tc);
    }
}

struct edgegroup
{
    ivec slope, origin;
    int axis;
};

static uint hthash(const edgegroup &g)
{
    return g.slope.x^g.slope.y^g.slope.z^g.origin.x^g.origin.y^g.origin.z;
}

static bool htcmp(const edgegroup &x, const edgegroup &y) 
{ 
    return x.slope==y.slope && x.origin==y.origin;
}

enum
{
    CE_START = 1<<0,
    CE_END   = 1<<1,
    CE_FLIP  = 1<<2,
    CE_DUP   = 1<<3
};

struct cubeedge
{
    cube *c;
    int next, offset;
    ushort size;
    uchar index, flags;
};

vector<cubeedge> cubeedges;
hashtable<edgegroup, int> edgegroups(1<<13);

void gencubeedges(cube &c, int x, int y, int z, int size)
{
    ivec vv[4];
    int mergeindex = 0, vis;
    loopi(6) if((vis = visibletris(c, i, x, y, z, size)))
    {
        if(c.ext && c.ext->merged&(1<<i))
        {
            if(!(c.ext->mergeorigin&(1<<i))) continue;

            const mergeinfo &m = c.ext->merges[mergeindex++];
            vvec mv[4];
            ivec mo(x, y, z);
            genmergedverts(c, i, mo, size, m, mv);
            loopj(4)
            {
                loopk(3) vv[j][k] = mv[j][k] + ((mo[k]&~VVEC_INT_MASK)<<VVEC_FRAC);
            }
        } 
        else 
        {
            int order = vis&4 || faceconvexity(c, i)<0 ? 1 : 0;
            loopj(4)
            {
                int k = fv[i][(j+order)&3];
                if(isentirelysolid(c)) vv[j] = cubecoords[k];
                else genvectorvert(cubecoords[k], c, vv[j]);
                vv[j].mul(size/(8>>VVEC_FRAC)).add(ivec(x, y, z).mul(1<<VVEC_FRAC));
            }
            if(!(vis&1)) vv[1] = vv[0];
            if(!(vis&2)) vv[3] = vv[0];
        }
        loopj(4)
        {
            int e1 = j, e2 = (j+1)%4;
            ivec d = vv[e2];
            d.sub(vv[e1]);
            if(d.iszero()) continue;
            int axis = abs(d.x) > abs(d.y) ? (abs(d.x) > abs(d.z) ? 0 : 2) : (abs(d.y) > abs(d.z) ? 1 : 2);
            if(d[axis] < 0)
            {
                d.neg();
                swap(e1, e2);
            }
            reduceslope(d);

            int t1 = vv[e1][axis]/d[axis],
                t2 = vv[e2][axis]/d[axis];
            edgegroup g;
            loopk(3) g.origin[k] = vv[e1][k] - t1*d[k];
            g.slope = d;
            g.axis = axis;
            cubeedge ce;
            ce.c = &c;
            ce.offset = t1;
            ce.size = t2 - t1;
            ce.index = i*4+j;
            ce.flags = CE_START | CE_END | (e1!=j ? CE_FLIP : 0);
            ce.next = -1;

            bool insert = true;
            int *exists = edgegroups.access(g);
            if(exists)
            {
                int prev = -1, cur = *exists;
                while(cur >= 0)
                {
                    cubeedge &p = cubeedges[cur];
                    if(p.flags&CE_DUP ? 
                        ce.offset>=p.offset && ce.offset+ce.size<=p.offset+p.size : 
                        ce.offset==p.offset && ce.size==p.size)
                    {
                        p.flags |= CE_DUP;
                        insert = false;
                        break;
                    }
                    else if(ce.offset >= p.offset)
                    {
                        if(ce.offset == p.offset+p.size) ce.flags &= ~CE_START;
                        prev = cur;
                        cur = p.next;
                    }
                    else break;
                }
                if(insert)
                {
                    ce.next = cur;
                    while(cur >= 0)
                    {
                        cubeedge &p = cubeedges[cur];
                        if(ce.offset+ce.size==p.offset) { ce.flags &= ~CE_END; break; }
                        cur = p.next;
                    }
                    if(prev>=0) cubeedges[prev].next = cubeedges.length();
                    else *exists = cubeedges.length();
                }
            }
            else edgegroups[g] = cubeedges.length();

            if(insert) cubeedges.add(ce);
        }
    }
}

void gencubeedges(cube *c = worldroot, int x = 0, int y = 0, int z = 0, int size = worldsize>>1)
{
    progress("fixing t-joints...");
    loopi(8)
    {
        ivec o(i, x, y, z, size);
        if(c[i].ext) c[i].ext->tjoints = -1;
        if(c[i].children) gencubeedges(c[i].children, o.x, o.y, o.z, size>>1);
        else if(!isempty(c[i])) gencubeedges(c[i], o.x, o.y, o.z, size);
    }
}

void gencubeverts(cube &c, int x, int y, int z, int size, int csi, uchar &vismask, uchar &clipmask)
{
    freeclipplanes(c);                          // physics planes based on rendering
    if(c.ext) c.ext->visible = 0;

    int tj = c.ext ? c.ext->tjoints : -1, numblends = 0, vis;
    loopi(6) if((vis = visibletris(c, i, x, y, z, size)))
    {
        if(c.texture[i]!=DEFAULT_SKY) vismask |= 1<<i;

        cubeext &e = ext(c);

        // this is necessary for physics to work, even if the face is merged
        if(touchingface(c, i)) 
        {
            e.visible |= 1<<i;
            if(c.texture[i]!=DEFAULT_SKY && faceedges(c, i)==F_SOLID) clipmask |= 1<<i;
        }

        if(e.surfaces && e.surfaces[i].layer&LAYER_BLEND) numblends++; 

        if(e.merged&(1<<i)) continue;

        int order = vis&4 || faceconvexity(c, i)<0 ? 1 : 0;
        vvec vv[4];
        loopk(4) calcvert(c, x, y, z, size, vv[k], fv[i][(k+order)&3]);
        if(!(vis&1)) vv[1] = vv[0];
        if(!(vis&2)) vv[3] = vv[0];

        ushort envmap = EMID_NONE, envmap2 = EMID_NONE;
        Slot &slot = lookuptexture(c.texture[i], false),
             *layer = slot.layer ? &lookuptexture(slot.layer, false) : NULL;
        if(slot.shader->type&SHADER_ENVMAP)
        {
            loopvj(slot.sts) if(slot.sts[j].type==TEX_ENVMAP) { envmap = EMID_CUSTOM; break; }
            if(envmap==EMID_NONE) envmap = closestenvmap(i, x, y, z, size); 
        }
        if(layer && layer->shader->type&SHADER_ENVMAP)
        {
            loopvj(layer->sts) if(layer->sts[j].type==TEX_ENVMAP) { envmap2 = EMID_CUSTOM; break; }
            if(envmap2==EMID_NONE) envmap2 = closestenvmap(i, x, y, z, size); 
        }
        while(tj >= 0 && tjoints[tj].edge < i*4) tj = tjoints[tj].next;
        int hastj = tj >= 0 && tjoints[tj].edge/4 == i ? tj : -1;
        int grassy = slot.autograss && i!=O_BOTTOM ? (vis!=3 || faceconvexity(c, i) ? 1 : 2) : 0;
        if(e.surfaces && e.surfaces[i].layer&LAYER_BLEND)
        {
            addcubeverts(i, size, vv, c.texture[i], &e.surfaces[i], e.normals ? &e.normals[i] : NULL, hastj, envmap, grassy);
            addcubeverts(i, size, vv, slot.layer, &e.surfaces[5+numblends], e.normals ? &e.normals[i] : NULL, hastj, envmap2);
        }
        else
        {
            ushort tex = c.texture[i];
            if(e.surfaces && e.surfaces[i].layer==LAYER_BOTTOM) { tex = slot.layer; envmap = envmap2; grassy = 0; }
            addcubeverts(i, size, vv, tex, e.surfaces ? &e.surfaces[i] : NULL, e.normals ? &e.normals[i] : NULL, hastj, envmap, grassy);
        }
    }
    else if(touchingface(c, i))
    {
        if(visibleface(c, i, x, y, z, size, MAT_AIR, MAT_NOCLIP, MATF_CLIP)) ext(c).visible |= 1<<i;
        if(faceedges(c, i)==F_SOLID) clipmask |= 1<<i;
    }
}

bool skyoccluded(cube &c, int orient)
{
    if(isempty(c)) return false;
//    if(c.texture[orient] == DEFAULT_SKY) return true;
    if(touchingface(c, orient) && faceedges(c, orient) == F_SOLID) return true;
    return false;
}

int hasskyfaces(cube &c, int x, int y, int z, int size, int faces[6])
{
    int numfaces = 0;
    if(x == 0 && !skyoccluded(c, O_LEFT)) faces[numfaces++] = O_LEFT;
    if(x + size == worldsize && !skyoccluded(c, O_RIGHT)) faces[numfaces++] = O_RIGHT;
    if(y == 0 && !skyoccluded(c, O_BACK)) faces[numfaces++] = O_BACK;
    if(y + size == worldsize && !skyoccluded(c, O_FRONT)) faces[numfaces++] = O_FRONT;
    if(z == 0 && !skyoccluded(c, O_BOTTOM)) faces[numfaces++] = O_BOTTOM;
    if(z + size == worldsize && !skyoccluded(c, O_TOP)) faces[numfaces++] = O_TOP;
    return numfaces;
}

vector<cubeface> skyfaces[6];
 
void minskyface(cube &cu, int orient, const ivec &co, int size, mergeinfo &orig)
{   
    mergeinfo mincf;
    mincf.u1 = orig.u2;
    mincf.u2 = orig.u1;
    mincf.v1 = orig.v2;
    mincf.v2 = orig.v1;
    mincubeface(cu, orient, co, size, orig, mincf);
    orig.u1 = max(mincf.u1, orig.u1);
    orig.u2 = min(mincf.u2, orig.u2);
    orig.v1 = max(mincf.v1, orig.v1);
    orig.v2 = min(mincf.v2, orig.v2);
}  

void genskyfaces(cube &c, const ivec &o, int size)
{
    if(isentirelysolid(c)) return;

    int faces[6],
        numfaces = hasskyfaces(c, o.x, o.y, o.z, size, faces);
    if(!numfaces) return;

    loopi(numfaces)
    {
        int orient = faces[i], dim = dimension(orient);
        cubeface m;
        m.c = NULL;
        m.u1 = (o[C[dim]]&VVEC_INT_MASK)<<VVEC_FRAC; 
        m.u2 = ((o[C[dim]]&VVEC_INT_MASK)+size)<<VVEC_FRAC;
        m.v1 = (o[R[dim]]&VVEC_INT_MASK)<<VVEC_FRAC;
        m.v2 = ((o[R[dim]]&VVEC_INT_MASK)+size)<<VVEC_FRAC;
        minskyface(c, orient, o, size, m);
        if(m.u1 >= m.u2 || m.v1 >= m.v2) continue;
        vc.skyarea += (int(m.u2-m.u1)*int(m.v2-m.v1) + (1<<(2*VVEC_FRAC))-1)>>(2*VVEC_FRAC);
        skyfaces[orient].add(m);
    }
}

void addskyverts(const ivec &o, int size)
{
    loopi(6)
    {
        int dim = dimension(i), c = C[dim], r = R[dim];
        vector<cubeface> &sf = skyfaces[i]; 
        if(sf.empty()) continue;
        vc.skyfaces |= 0x3F&~(1<<opposite(i));
        sf.setsizenodelete(mergefaces(i, sf.getbuf(), sf.length()));
        loopvj(sf)
        {
            mergeinfo &m = sf[j];
            int index[4];
            loopk(4)
            {
                const ivec &coords = cubecoords[fv[i][3-k]];
                vvec vv;
                vv[dim] = (o[dim]&VVEC_INT_MASK)<<VVEC_FRAC;
                if(coords[dim]) vv[dim] += size<<VVEC_FRAC;
                vv[c] = coords[c] ? m.u2 : m.u1;
                vv[r] = coords[r] ? m.v2 : m.v1;
                index[k] = vc.addvert(vv, 0, 0, bvec(128, 128, 128));
                if(index[k] < 0) goto nextskyface;
                vc.skyclip = min(vc.skyclip, int(vv.z>>VVEC_FRAC));
            }
            if(vc.skytris + 6 > USHRT_MAX) break;
            vc.skytris += 6;
            vc.skyindices.add(index[0]);
            vc.skyindices.add(index[1]);
            vc.skyindices.add(index[2]);

            vc.skyindices.add(index[0]);
            vc.skyindices.add(index[2]);
            vc.skyindices.add(index[3]);
        nextskyface:;
        }
        sf.setsizenodelete(0);
    }
}
                    
////////// Vertex Arrays //////////////

int allocva = 0;
int wtris = 0, wverts = 0, vtris = 0, vverts = 0, glde = 0, gbatches = 0;
vector<vtxarray *> valist, varoot;

vtxarray *newva(int x, int y, int z, int size)
{
    vc.optimize();

    vtxarray *va = new vtxarray;
    va->parent = NULL;
    va->o = ivec(x, y, z);
    va->size = size;
    va->skyarea = vc.skyarea;
    va->skyfaces = vc.skyfaces;
    va->skyclip = vc.skyclip < INT_MAX ? vc.skyclip + (z&~VVEC_INT_MASK) : INT_MAX;
    va->curvfc = VFC_NOT_VISIBLE;
    va->occluded = OCCLUDE_NOTHING;
    va->query = NULL;
    va->mapmodels = NULL;
    va->bbmin = ivec(-1, -1, -1);
    va->bbmax = ivec(-1, -1, -1);
    va->grasstris = NULL;
    va->hasmerges = 0;

    vc.setupdata(va);

    wverts += va->verts;
    wtris  += va->tris;
    allocva++;
    valist.add(va);

    return va;
}

void destroyva(vtxarray *va, bool reparent)
{
    wverts -= va->verts;
    wtris -= va->tris;
    allocva--;
    valist.removeobj(va);
    if(!va->parent) varoot.removeobj(va);
    if(reparent)
    {
        if(va->parent) va->parent->children.removeobj(va);
        loopv(va->children)
        {
            vtxarray *child = va->children[i];
            child->parent = va->parent;
            if(child->parent) child->parent->children.add(child);
        }
    }
    if(va->vbuf) destroyvbo(va->vbuf);
    if(va->ebuf) destroyvbo(va->ebuf);
    if(va->skybuf) destroyvbo(va->skybuf);
    if(va->eslist) delete[] va->eslist;
    if(va->matbuf) delete[] va->matbuf;
    if(va->mapmodels) delete va->mapmodels;
    if(va->grasstris) delete va->grasstris;
    delete va;
}

void clearvas(cube *c)
{
    loopi(8)
    {
        if(c[i].ext)
        {
            if(c[i].ext->va) destroyva(c[i].ext->va, false);
            c[i].ext->va = NULL;
            c[i].ext->tjoints = -1;
        }
        if(c[i].children) clearvas(c[i].children);
    }
}

void updatevabb(vtxarray *va, bool force)
{
    if(!force && va->bbmin.x >= 0) return;

    va->bbmin = va->geommin;
    va->bbmax = va->geommax;
    loopk(3)
    {
        va->bbmin[k] = min(va->bbmin[k], va->matmin[k]);
        va->bbmax[k] = max(va->bbmax[k], va->matmax[k]);
    }
    loopv(va->children)
    {
        vtxarray *child = va->children[i];
        updatevabb(child, force);
        loopk(3)
        {
            va->bbmin[k] = min(va->bbmin[k], child->bbmin[k]);
            va->bbmax[k] = max(va->bbmax[k], child->bbmax[k]);
        }
    }
    if(va->mapmodels) 
    {
        loopv(*va->mapmodels)
        {
            octaentities *oe = (*va->mapmodels)[i];
            loopk(3)
            {
                va->bbmin[k] = min(va->bbmin[k], oe->bbmin[k]);
                va->bbmax[k] = max(va->bbmax[k], oe->bbmax[k]);
            }
        }
    }

    if(va->skyfaces)
    {
        va->skyfaces |= 0x80;
        if(va->sky) loop(dim, 3) if(va->skyfaces&(3<<(2*dim)))
        {
            int r = R[dim], c = C[dim];
            if((va->skyfaces&(1<<(2*dim)) && va->o[dim] < va->bbmin[dim]) ||
               (va->skyfaces&(2<<(2*dim)) && va->o[dim]+va->size > va->bbmax[dim]) ||
               va->o[r] < va->bbmin[r] || va->o[r]+va->size > va->bbmax[r] ||
               va->o[c] < va->bbmin[c] || va->o[c]+va->size > va->bbmax[c])
            {
                va->skyfaces &= ~0x80;
                break;
            }
        }
    }
}

void updatevabbs(bool force)
{
    loopv(varoot) updatevabb(varoot[i], force);
}

struct mergedface
{   
    uchar orient;
    ushort tex, envmap;
    vvec v[4];
    surfaceinfo *surface;
    surfacenormals *normals;
    int tjoints;
};  

static int vahasmerges = 0, vamergemax = 0;
static vector<mergedface> vamerges[VVEC_INT];

void genmergedfaces(cube &c, const ivec &co, int size, int minlevel = -1)
{
    if(!c.ext || !c.ext->merges || isempty(c)) return;
    int index = 0, tj = c.ext->tjoints, numblends = 0;
    loopi(6) 
    {
        if(c.ext->surfaces && c.ext->surfaces[i].layer&LAYER_BLEND) numblends++;
        if(!(c.ext->mergeorigin & (1<<i))) continue;
        mergeinfo &m = c.ext->merges[index++];
        if(m.u1>=m.u2 || m.v1>=m.v2) continue;
        mergedface mf;
        mf.orient = i;
        mf.tex = c.texture[i];
        mf.envmap = EMID_NONE;
        mf.surface = c.ext->surfaces ? &c.ext->surfaces[i] : NULL;
        mf.normals = c.ext->normals ? &c.ext->normals[i] : NULL;
        mf.tjoints = -1;
        genmergedverts(c, i, co, size, m, mf.v);
        int level = calcmergedsize(i, co, size, m, mf.v);
        if(level > minlevel)
        {
            while(tj >= 0 && tjoints[tj].edge < i*4) tj = tjoints[tj].next;
            if(tj >= 0 && tjoints[tj].edge/4 == i) mf.tjoints = tj;

            Slot &slot = lookuptexture(mf.tex, false),
                 *layer = slot.layer ? &lookuptexture(slot.layer, false) : NULL;
            ushort envmap2 = EMID_NONE;
            if(slot.shader->type&SHADER_ENVMAP)
            {
                loopvj(slot.sts) if(slot.sts[j].type==TEX_ENVMAP) { mf.envmap = EMID_CUSTOM; break; }
                if(mf.envmap==EMID_NONE) mf.envmap = closestenvmap(i, co.x, co.y, co.z, size);
            }
            if(layer && layer->shader->type&SHADER_ENVMAP)
            {
                loopvj(layer->sts) if(layer->sts[j].type==TEX_ENVMAP) { envmap2 = EMID_CUSTOM; break; }
                if(envmap2==EMID_NONE) envmap2 = closestenvmap(i, co.x, co.y, co.z, size);
            }

            if(c.ext->surfaces)
            {
                if(c.ext->surfaces[i].layer&LAYER_BLEND)
                {
                    mergedface mf2 = mf;
                    mf2.tex = slot.layer;
                    mf2.envmap = envmap2;
                    mf2.surface = &c.ext->surfaces[5+numblends];
                    vamerges[level].add(mf2);
                }
                else if(c.ext->surfaces[i].layer==LAYER_BOTTOM)
                {
                    mf.tex = slot.layer;
                    mf.envmap = envmap2;
                }
            } 

            vamerges[level].add(mf);
            vamergemax = max(vamergemax, level);
            vahasmerges |= MERGE_ORIGIN;
        }
    }
}

void findmergedfaces(cube &c, const ivec &co, int size, int csi, int minlevel)
{
    if(c.ext && c.ext->va && !(c.ext->va->hasmerges&MERGE_ORIGIN)) return;
    if(c.children)
    {
        loopi(8)
        {
            ivec o(i, co.x, co.y, co.z, size/2); 
            findmergedfaces(c.children[i], o, size/2, csi-1, minlevel);
        }
    }
    else if(c.ext && c.ext->merges) genmergedfaces(c, co, size, minlevel);
}

void addmergedverts(int level)
{
    vector<mergedface> &mfl = vamerges[level];
    if(mfl.empty()) return;
    loopv(mfl)
    {
        mergedface &mf = mfl[i];
        Slot &slot = lookuptexture(mf.tex, false);
        int grassy = slot.autograss && mf.orient!=O_BOTTOM && (!mf.surface || mf.surface->layer!=LAYER_BOTTOM) ? 2 : 0;
        addcubeverts(mf.orient, 1<<level, mf.v, mf.tex, mf.surface, mf.normals, mf.tjoints, mf.envmap, grassy);
        vahasmerges |= MERGE_USE;
    }
    mfl.setsizenodelete(0);
}

static uchar unusedmask;

void rendercube(cube &c, int cx, int cy, int cz, int size, int csi, uchar &vismask = unusedmask, uchar &clipmask = unusedmask)  // creates vertices and indices ready to be put into a va
{
    //if(size<=16) return;
    if(c.ext && c.ext->va) 
    {
        vismask = c.children ? c.vismask : 0x3F;
        clipmask = c.children ? c.clipmask : 0;
        return;                            // don't re-render
    }

    if(c.children)
    {
        uchar visparent = 0, clipparent = 0x3F;
        uchar clipchild[8];
        loopi(8)
        {
            ivec o(i, cx, cy, cz, size/2);
            rendercube(c.children[i], o.x, o.y, o.z, size/2, csi-1, c.vismasks[i], clipchild[i]);
            uchar mask = (1<<octacoord(0, i)) | (4<<octacoord(1, i)) | (16<<octacoord(2, i));
            visparent |= c.vismasks[i];
            clipparent &= (clipchild[i]&mask) | ~mask;
            clipparent &= ~(c.vismasks[i] & (mask^0x3F));
        }
        vismask = c.vismask = visparent;
        clipmask = c.clipmask = clipparent;

        if(csi < VVEC_INT && vamerges[csi].length()) addmergedverts(csi);

        if(c.ext)
        {
            if(c.ext->ents && c.ext->ents->mapmodels.length()) vc.mapmodels.add(c.ext->ents);
        }
        return;
    }
    
    genskyfaces(c, ivec(cx, cy, cz), size);

    vismask = clipmask = 0;

    if(!isempty(c)) gencubeverts(c, cx, cy, cz, size, csi, vismask, clipmask);

    if(c.ext)
    {
        if(c.ext->ents && c.ext->ents->mapmodels.length()) vc.mapmodels.add(c.ext->ents);
        if(c.ext->material != MAT_AIR) genmatsurfs(c, cx, cy, cz, size, vc.matsurfs, vismask, clipmask);
        if(c.ext->merges) genmergedfaces(c, ivec(cx, cy, cz), size);
        if(c.ext->merged & ~c.ext->mergeorigin) vahasmerges |= MERGE_PART;
    }

    if(csi < VVEC_INT && vamerges[csi].length()) addmergedverts(csi);
}

void calcgeombb(int cx, int cy, int cz, int size, ivec &bbmin, ivec &bbmax)
{
    vvec vmin(cx, cy, cz), vmax = vmin;
    vmin.add(size);

    loopv(vc.verts)
    {
        vvec &v = vc.verts[i];
        loopj(3)
        {
            if(v[j]<vmin[j]) vmin[j] = v[j];
            if(v[j]>vmax[j]) vmax[j] = v[j];
        }
    }

    bbmin = vmin.toivec(cx, cy, cz);
    loopi(3) vmax[i] += (1<<VVEC_FRAC)-1;
    bbmax = vmax.toivec(cx, cy, cz);
}

void calcmatbb(int cx, int cy, int cz, int size, ivec &bbmin, ivec &bbmax)
{
    bbmax = ivec(cx, cy, cz);
    (bbmin = bbmax).add(size);
    loopv(vc.matsurfs)
    {
        materialsurface &m = vc.matsurfs[i];
        switch(m.material)
        {
            case MAT_WATER:
            case MAT_GLASS:
            case MAT_LAVA:
                break;

            default:
                continue;
        }

        int dim = dimension(m.orient),
            r = R[dim],
            c = C[dim];
        bbmin[dim] = min(bbmin[dim], m.o[dim]);
        bbmax[dim] = max(bbmax[dim], m.o[dim]);

        bbmin[r] = min(bbmin[r], m.o[r]);
        bbmax[r] = max(bbmax[r], m.o[r] + m.rsize);

        bbmin[c] = min(bbmin[c], m.o[c]);
        bbmax[c] = max(bbmax[c], m.o[c] + m.csize);
    }
}

void setva(cube &c, int cx, int cy, int cz, int size, int csi)
{
    ASSERT(size <= VVEC_INT_MASK+1);

    vc.origin = ivec(cx, cy, cz);
    vc.size = size;

    shadowmapmin = vvec(cx+size, cy+size, cz+size);
    shadowmapmax = vvec(cx, cy, cz);

    rendercube(c, cx, cy, cz, size, csi);

    ivec bbmin, bbmax;

    calcgeombb(cx, cy, cz, size, bbmin, bbmax);

    addskyverts(ivec(cx, cy, cz), size);

    if(!vc.emptyva())
    {
        vtxarray *va = newva(cx, cy, cz, size);
        ext(c).va = va;
        va->geommin = bbmin;
        va->geommax = bbmax;
        calcmatbb(cx, cy, cz, size, va->matmin, va->matmax);
        va->shadowmapmin = shadowmapmin.toivec(va->o);
        loopk(3) shadowmapmax[k] += (1<<VVEC_FRAC)-1;
        va->shadowmapmax = shadowmapmax.toivec(va->o);
        va->hasmerges = vahasmerges;
    }

    vc.clear();
}

VARF(vacubemax, 64, 512, 256*256, allchanged());
VARF(vacubesize, 32, 128, VVEC_INT_MASK+1, allchanged());
VARF(vacubemin, 0, 128, 256*256, allchanged());

int updateva(cube *c, int cx, int cy, int cz, int size, int csi)
{
    progress("recalculating geometry...");
    static int faces[6];
    int ccount = 0, cmergemax = vamergemax, chasmerges = vahasmerges;
    loopi(8)                                    // counting number of semi-solid/solid children cubes
    {
        int count = 0, childpos = varoot.length();
        ivec o(i, cx, cy, cz, size);
        vamergemax = 0;
        vahasmerges = 0;
        if(c[i].ext && c[i].ext->va) 
        {
            //count += vacubemax+1;       // since must already have more then max cubes
            varoot.add(c[i].ext->va);
            if(c[i].ext->va->hasmerges&MERGE_ORIGIN) findmergedfaces(c[i], o, size, csi, csi);
        }
        else
        {
            if(c[i].children) count += updateva(c[i].children, o.x, o.y, o.z, size/2, csi-1);
            else if(!isempty(c[i]) || hasskyfaces(c[i], o.x, o.y, o.z, size, faces)) count++;
            int tcount = count + (csi < VVEC_INT ? vamerges[csi].length() : 0);
            if(tcount > vacubemax || (tcount >= vacubemin && size >= vacubesize) || size == min(VVEC_INT_MASK+1, worldsize/2)) 
            {
                setva(c[i], o.x, o.y, o.z, size, csi);
                if(c[i].ext && c[i].ext->va)
                {
                    while(varoot.length() > childpos)
                    {
                        vtxarray *child = varoot.pop();
                        c[i].ext->va->children.add(child);
                        child->parent = c[i].ext->va;
                    }
                    varoot.add(c[i].ext->va);
                    if(vamergemax > size)
                    {
                        cmergemax = max(cmergemax, vamergemax);
                        chasmerges |= vahasmerges&~MERGE_USE;
                    }
                    continue;
                }
                else count = 0;
            }
        }
        if(csi < VVEC_INT-1 && vamerges[csi].length()) vamerges[csi+1].move(vamerges[csi]);
        cmergemax = max(cmergemax, vamergemax);
        chasmerges |= vahasmerges;
        ccount += count;
    }

    vamergemax = cmergemax;
    vahasmerges = chasmerges;

    return ccount;
}

void buildclipmasks(cube &c, uchar &vismask = unusedmask, uchar &clipmask = unusedmask)
{
    if(c.ext && c.ext->va)
    {
        vismask = c.children ? c.vismask : 0x3F;
        clipmask = c.children ? c.clipmask : 0;
        return;
    }
    if(!c.children)
    {
        if(isempty(c)) c.vismask = c.clipmask = 0;
        vismask = clipmask = 0;
        return;
    }
    uchar visparent = 0, clipparent = 0x3F;
    uchar clipchild[8];
    loopi(8)
    {
        buildclipmasks(c.children[i], c.vismasks[i], clipchild[i]);
        uchar mask = (1<<octacoord(0, i)) | (4<<octacoord(1, i)) | (16<<octacoord(2, i));
        visparent |= c.vismasks[i];
        clipparent &= (clipchild[i]&mask) | ~mask;
        clipparent &= ~(c.vismasks[i] & (mask^0x3F));
    }
    vismask = c.vismask = visparent;
    clipmask = c.clipmask = clipparent;
}

void addtjoint(const edgegroup &g, const cubeedge &e, int offset)
{
    int vcoord = (g.slope[g.axis]*offset + g.origin[g.axis]) & ((VVEC_INT_MASK<<VVEC_FRAC) | ((1<<VVEC_FRAC)-1));
    tjoint &tj = tjoints.add();
    tj.offset = vcoord / g.slope[g.axis];
    tj.edge = e.index;

    int prev = -1, cur = ext(*e.c).tjoints;
    while(cur >= 0)
    {
        tjoint &o = tjoints[cur];
        if(tj.edge < o.edge || (tj.edge==o.edge && (e.flags&CE_FLIP ? tj.offset > o.offset : tj.offset < o.offset))) break;
        prev = cur;
        cur = o.next;
    }

    tj.next = cur;
    if(prev < 0) e.c->ext->tjoints = tjoints.length()-1;
    else tjoints[prev].next = tjoints.length()-1; 
}

void findtjoints(int cur, const edgegroup &g)
{
    int active = -1;
    while(cur >= 0)
    {
        cubeedge &e = cubeedges[cur];
        int prevactive = -1, curactive = active;
        while(curactive >= 0)
        {
            cubeedge &a = cubeedges[curactive];
            if(a.offset+a.size <= e.offset)
            {
                if(prevactive >= 0) cubeedges[prevactive].next = a.next;
                else active = a.next;
            }
            else
            {
                prevactive = curactive;
                if(!(a.flags&CE_DUP))
                {
                    if(e.flags&CE_START && e.offset > a.offset && e.offset < a.offset+a.size)
                        addtjoint(g, a, e.offset);
                    if(e.flags&CE_END && e.offset+e.size > a.offset && e.offset+e.size < a.offset+a.size)
                        addtjoint(g, a, e.offset+e.size);
                }
                if(!(e.flags&CE_DUP))
                {
                    if(a.flags&CE_START && a.offset > e.offset && a.offset < e.offset+e.size)
                        addtjoint(g, e, a.offset);
                    if(a.flags&CE_END && a.offset+a.size > e.offset && a.offset+a.size < e.offset+e.size)
                        addtjoint(g, e, a.offset+a.size);
                }
            }
            curactive = a.next;
        }
        int next = e.next;
        e.next = active;
        active = cur;
        cur = next;
    }
}

void octarender()                               // creates va s for all leaf cubes that don't already have them
{
    int csi = 0;
    while(1<<csi < worldsize) csi++;

    recalcprogress = 0;
    varoot.setsizenodelete(0);
    updateva(worldroot, 0, 0, 0, worldsize/2, csi-1);
    flushvbo();

    loopi(8) buildclipmasks(worldroot[i]);

    explicitsky = 0;
    skyarea = 0;
    loopv(valist)
    {
        vtxarray *va = valist[i];
        explicitsky += va->explicitsky;
        skyarea += va->skyarea;
    }

    extern vtxarray *visibleva;
    visibleva = NULL;
}

void precachetextures()
{
#ifdef CLIENT
    IntensityTexture::resetBackgroundLoading(); // INTENSITY: see below for backgroundLoading
#endif

    vector<int> texs;
    loopv(valist)
    {
        vtxarray *va = valist[i];
        loopj(va->texs) if(texs.find(va->eslist[j].texture) < 0) texs.add(va->eslist[j].texture);
    }
    loopv(texs)
    {
        loadprogress = float(i+1)/texs.length();
        lookuptexture(texs[i]);
    }

#ifdef CLIENT
    IntensityTexture::doBackgroundLoading(true); // INTENSITY: lookuptexture just queues, now, so here we need to flush all the requests
#endif

    loadprogress = 0;
}

void allchanged(bool load)
{
    renderprogress(0, "clearing vertex arrays...");
    clearvas(worldroot);
    resetqueries();
    if(load) initenvmaps();
    guessshadowdir();
    entitiesinoctanodes();
    tjoints.setsizenodelete(0);
    if(filltjoints)
    {
        recalcprogress = 0;
        gencubeedges();
        enumeratekt(edgegroups, edgegroup, g, int, e, findtjoints(e, g));
        cubeedges.setsizenodelete(0);
        edgegroups.clear();
    }
    octarender();
    if(load) precachetextures();
    setupmaterials();
    invalidatepostfx();
    updatevabbs(true);
    resetblobs();
    if(load) 
    {
        seedparticles();
        genenvmaps();
    }
}

void recalc()
{
    allchanged(true);
}

COMMAND(recalc, "");

