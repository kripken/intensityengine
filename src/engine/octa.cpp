// core world management routines

#include "engine.h"

cube *worldroot = newcubes(F_SOLID);
int allocnodes = 0;

cubeext *newcubeext(cube &c)
{
    if(c.ext) return c.ext;
    c.ext = new cubeext;
    c.ext->material = MAT_AIR;
    c.ext->visible = 0;
    c.ext->merged = 0;
    c.ext->mergeorigin = 0;
    c.ext->va = NULL;
    c.ext->clip = NULL;
    c.ext->surfaces = NULL;
    c.ext->normals = NULL;
    c.ext->ents = NULL;
    c.ext->merges = NULL;
    c.ext->tjoints = -1;
    return c.ext;
}

cube *newcubes(uint face)
{
    cube *c = new cube[8];
    loopi(8)
    {
        c->children = NULL;
        c->ext = NULL;
        setfaces(*c, face);
        loopl(6)
        {
            c->texture[l] = 2+l;
        }
        c++;
    }
    allocnodes++;
    return c-8;
}

int familysize(cube &c)
{
    int size = 1;
    if(c.children) loopi(8) size += familysize(c.children[i]);
    return size;
}

void freeocta(cube *c)
{
    if(!c) return;
    loopi(8) discardchildren(c[i]);
    delete[] c;
    allocnodes--;
}

void freecubeext(cube &c)
{
    DELETEP(c.ext);
}

void discardchildren(cube &c)
{
    if(c.ext)
    {
        if(c.ext->va) destroyva(c.ext->va);
        c.ext->va = NULL;
        c.ext->merged = 0;
        c.ext->tjoints = -1;
        freesurfaces(c);
        freenormals(c);
        freeclipplanes(c);
        freeoctaentities(c);
        freemergeinfo(c);
        freecubeext(c);
    }
    if(c.children)
    {
        freeocta(c.children);
        c.children = NULL;
    }
}

void getcubevector(cube &c, int d, int x, int y, int z, ivec &p)
{
    ivec v(d, x, y, z);

    loopi(3)
        p[i] = edgeget(cubeedge(c, i, v[R[i]], v[C[i]]), v[D[i]]);
}

void setcubevector(cube &c, int d, int x, int y, int z, ivec &p)
{
    ivec v(d, x, y, z);

    loopi(3)
        edgeset(cubeedge(c, i, v[R[i]], v[C[i]]), v[D[i]], p[i]);
}

void optiface(uchar *p, cube &c)
{
    loopi(4) if(edgeget(p[i], 0)!=edgeget(p[i], 1)) return;
    emptyfaces(c);
}

void printcube()
{
    cube &c = lookupcube(lu.x, lu.y, lu.z, 0); // assume this is cube being pointed at
    conoutf(CON_DEBUG, "= %p = (%d, %d, %d) @ %d", &c, lu.x, lu.y, lu.z, lusize);
    conoutf(CON_DEBUG, " x  %.8x", c.faces[0]);
    conoutf(CON_DEBUG, " y  %.8x", c.faces[1]);
    conoutf(CON_DEBUG, " z  %.8x", c.faces[2]);
}

COMMAND(printcube, "");

bool isvalidcube(cube &c)
{
    clipplanes p;
    genclipplanes(c, 0, 0, 0, 256, p);
    loopi(8) // test that cube is convex
    {
        vvec v;
        calcvert(c, 0, 0, 0, 256, v, i);
        if(!pointincube(p, v.tovec()))
            return false;
    }
    return true;
}

void validatec(cube *c, int size)
{
    loopi(8)
    {
        if(c[i].children)
        {
            if(size<=(8>>VVEC_FRAC))
            {
                solidfaces(c[i]);
                discardchildren(c[i]);
            }
            else validatec(c[i].children, size>>1);
        }
        else
        {
            loopj(3) optiface((uchar *)&(c[i].faces[j]), c[i]);
            loopj(12)
                if(edgeget(c[i].edges[j], 1)>8 ||
                   edgeget(c[i].edges[j], 1)<edgeget(c[i].edges[j], 0))
                    emptyfaces(c[i]);
        }
    }
}

ivec lu;
int lusize;
cube &lookupcube(int tx, int ty, int tz, int tsize)
{
    int size = worldsize;
    int x = 0, y = 0, z = 0;
    cube *c = worldroot;
    for(;;)
    {
        size >>= 1;
        ASSERT(size);
        if(tz>=z+size) { z += size; c += 4; }
        if(ty>=y+size) { y += size; c += 2; }
        if(tx>=x+size) { x += size; c += 1; }
        //if(tsize==size) break;
        if(abs(tsize)>=size) break;
        if(c->children==NULL)
        {
            //if(!tsize) break;
            if(tsize<=0) break;
            subdividecube(*c);
        }
        c = c->children;
    }
    lu.x = x;
    lu.y = y;
    lu.z = z;
    lusize = size;
    return *c;
}

cube &neighbourcube(int x, int y, int z, int size, int rsize, int orient)
{
    switch(orient)
    {
        case O_BOTTOM: z -= size; break;
        case O_TOP:    z += size; break;
        case O_BACK:   y -= size; break;
        case O_FRONT:  y += size; break;
        case O_LEFT:   x -= size; break;
        case O_RIGHT:  x += size; break;
    }
    return lookupcube(x, y, z, rsize);
}

int lookupmaterial(const vec &v)
{
    ivec o(v);
    if(!insideworld(o)) return MAT_AIR;
    int scale = worldscale-1;
    cube *c = &worldroot[octastep(o.x, o.y, o.z, scale)];
    while(c->children)
    {
        scale--;
        c = &c->children[octastep(o.x, o.y, o.z, scale)];
    }
    return c->ext ? c->ext->material : MAT_AIR;
}

////////// (re)mip //////////

int getmippedtexture(cube &p, int orient)
{
    cube *c = p.children;
    int d = dimension(orient);
    int dc = dimcoord(orient);
    int tex[4] = {-1,-1,-1,-1};
    loop(x,2) loop(y,2)
    {
        int n = octaindex(d, x, y, dc);
        if(isempty(c[n]))
            n = oppositeocta(d, n);
        if(isempty(c[n]))
            continue;

        loopk(3)
            if(tex[k] == c[n].texture[orient])
                return tex[k];

        if(c[n].texture[orient] > 0) // assume 0 is sky. favour non-sky tex
            tex[x*2+y] = c[n].texture[orient];
    }

    loopk(4)
        if(tex[k]>0) return tex[k];

    return p.texture[orient];
}

void forcemip(cube &c)
{   
    cube *ch = c.children;
    emptyfaces(c);

    loopi(8) loopj(8)
    {
        int n = i^(j==3 ? 4 : (j==4 ? 3 : j));
        if(!isempty(ch[n])) // breadth first search for cube near vert
        {
            ivec v, p(i);
            getcubevector(ch[n], 2, p.x, p.y, p.z, v);

            loopk(3) // adjust vert to parent size
            {
                if(octacoord(k, n) == 1)
                    v[k] += 8;
                v[k] >>= 1;
            }

            setcubevector(c, 2, p.x, p.y, p.z, v);
            break;
        }
    }

    loopj(6)
        c.texture[j] = getmippedtexture(c, j);
}

int midedge(const ivec &a, const ivec &b, int xd, int yd, bool &perfect)
{   
    int ax = a[xd], bx = b[xd];
    int ay = a[yd], by = b[yd];
    if(ay==by) return ay;
    if(ax==bx) { perfect = false; return ay; }
    bool crossx = (ax<8 && bx>8) || (ax>8 && bx<8);
    bool crossy = (ay<8 && by>8) || (ay>8 && by<8);
    if(crossy && !crossx) { midedge(a,b,yd,xd,perfect); return 8; } // to test perfection
    if(ax<=8 && bx<=8) return ax>bx ? ay : by;
    if(ax>=8 && bx>=8) return ax<bx ? ay : by;
    int risex = (by-ay)*(8-ax)*256;
    int s = risex/(bx-ax);
    int y = s/256 + ay;
    if(((abs(s)&0xFF)!=0) || // ie: rounding error
        (crossy && y!=8) ||
        (y<0 || y>16)) perfect = false;
    return crossy ? 8 : min(max(y, 0), 16);
}  
    
bool subdividecube(cube &c, bool fullcheck, bool brighten)
{   
    if(c.children) return true;
	if(isempty(c) || isentirelysolid(c))
    {
        int mat = c.ext ? c.ext->material : MAT_AIR;
		c.children = newcubes(isempty(c) ? F_EMPTY : F_SOLID);
        loopi(8)
        {
            loopl(6) c.children[i].texture[l] = c.texture[l];
            if(mat!=MAT_AIR) ext(c.children[i]).material = mat;
            if(brighten && !isempty(c)) brightencube(c.children[i]);
        }
        return true;
    }
    cube *ch = c.children = newcubes(F_SOLID);
    bool perfect = true, p1, p2;
    int mat = c.ext ? c.ext->material : MAT_AIR; 
    ivec v[8];
    loopi(8)
    {
        ivec p(i);
        getcubevector(c, 2, p.x, p.y, p.z, v[i]);
        v[i].mul(2);
        if(mat!=MAT_AIR) ext(ch[i]).material = mat;
    }

    loopj(6)
    {
        int d = dimension(j);
        int z = dimcoord(j);
        int e[3][3];
        ivec *v1[2][2];
        loop(y, 2) loop(x, 2)
            v1[x][y] = v+octaindex(d, x, y, z);

        loop(y, 3) loop(x, 3)       // gen edges
        {
            if(x==1 && y==1)        // center
            {
                int c1 = midedge(*v1[0][0], *v1[1][1], R[d], d, p1 = perfect);
                int c2 = midedge(*v1[0][1], *v1[1][0], R[d], d, p2 = perfect);
                e[x][y] = z ? max(c1,c2) : min(c1,c2);
                perfect = e[x][y]==c1 ? p1 : p2;
            }
            else if(((x+y)&1)==0)   // corner
                e[x][y] = (*v1[x>>1][y>>1])[d];
            else                    // edge
            {
                int a = min(x, y), b = x&1;
                e[x][y] = midedge(*v1[a][a], *v1[a^b][a^(1-b)], x==1?R[d]:C[d], d, perfect);
            }
        }

        loopi(8)
        {
            ivec o(i);
            ch[i].texture[j] = c.texture[j];
            loop(y, 2) loop(x, 2) // assign child edges
            {
                int ce = e[x+o[R[d]]][y+o[C[d]]];
                if(o[D[d]]) ce -= 8;
                ce = min(max(ce, 0), 8);
                uchar &f = cubeedge(ch[i], d, x, y);
                edgeset(f, z, ce);
            }
        }
    }

    validatec(ch, worldsize);
    if(fullcheck) loopi(8) if(!isvalidcube(ch[i])) // not so good...
    {
        emptyfaces(ch[i]);
        perfect=false;
    }
    if(brighten) loopi(8) if(!isempty(ch[i])) brightencube(ch[i]);
    return perfect;
}

bool crushededge(uchar e, int dc) { return dc ? e==0 : e==0x88; }

int visibleorient(cube &c, int orient)
{
    loopi(2) loopj(2)
    {
        int a = faceedgesidx[orient][i*2 + 0];
        int b = faceedgesidx[orient][i*2 + 1];
        if(crushededge(c.edges[a],j) &&
           crushededge(c.edges[b],j) &&
           touchingface(c, orient)) return ((a>>2)<<1) + j;
    }
    return orient;
}

VAR(mipvis, 0, 0, 1);

static int remipprogress = 0, remiptotal = 0;

bool remip(cube &c, int x, int y, int z, int size)
{
    if(c.ext)
    {
        c.ext->merged = 0;
        if(c.ext->merges) freemergeinfo(c);
    }

    cube *ch = c.children;
    if(!ch) 
    {
        if(size<<1 <= VVEC_INT_MASK+1) return true;
        subdividecube(c);
        ch = c.children;
    }
    else if((remipprogress++&0xFFF)==1) renderprogress(float(remipprogress)/remiptotal, "remipping...");

    bool perfect = true;
    loopi(8)
    {
        ivec o(i, x, y, z, size);
        if(!remip(ch[i], o.x, o.y, o.z, size>>1)) perfect = false;
    }

    solidfaces(c); // so texmip is more consistent    
    loopj(6)
        c.texture[j] = getmippedtexture(c, j); // parents get child texs regardless

    if(!perfect) return false;
    if(size<<1 > VVEC_INT_MASK+1) return false;

    uchar mat = MAT_AIR;
    loopi(8)
    {
        mat = ch[i].ext ? ch[i].ext->material : MAT_AIR;
        if((mat&MATF_CLIP) == MAT_NOCLIP)
        {
            if(i > 0) return false;
            while(++i < 8) if(!ch[i].ext || ch[i].ext->material != mat) return false;
            break;
        }
        else if(!isentirelysolid(ch[i]))
        {
            while(++i < 8)
            {
                uchar omat = ch[i].ext ? ch[i].ext->material : MAT_AIR;
                if(isentirelysolid(ch[i]) ? (omat&MATF_CLIP) == MAT_NOCLIP : mat != omat) return false;
            }
            break;
        }
    }

    cube n = c;
    forcemip(n);
    n.children = NULL;
    if(!subdividecube(n, false, false))
        { freeocta(n.children); return false; }

    cube *nh = n.children;
    uchar vis[6] = {0, 0, 0, 0, 0, 0}; 
    loopi(8)
    {
        if(ch[i].faces[0] != nh[i].faces[0] ||
           ch[i].faces[1] != nh[i].faces[1] ||
           ch[i].faces[2] != nh[i].faces[2])
            { freeocta(nh); return false; }

        if(isempty(ch[i]) && isempty(nh[i])) continue;

        ivec o(i, x, y, z, size);
        loop(orient, 6)
            if(visibleface(ch[i], orient, o.x, o.y, o.z, size))
            {
                if(ch[i].texture[orient] != n.texture[orient]) { freeocta(nh); return false; }
                vis[orient] |= 1<<i;
            }
    }
    if(mipvis) loop(orient, 6)
    {
        int mask = 0;
        loop(x, 2) loop(y, 2) mask |= 1<<octaindex(dimension(orient), x, y, dimcoord(orient));
        if(vis[orient]&mask && (vis[orient]&mask)!=mask) { freeocta(nh); return false; }
    }

    freeocta(nh);
    discardchildren(c);
    loopi(3) c.faces[i] = n.faces[i];
    if(mat!=MAT_AIR) ext(c).material = mat;
    brightencube(c);
    return true;
}

void mpremip(bool local)
{
    extern selinfo sel;
    if(local) game::edittrigger(sel, EDIT_REMIP);
    remipprogress = 1;
    remiptotal = allocnodes;
    loopi(8)
    {
        ivec o(i, 0, 0, 0, worldsize>>1);
        remip(worldroot[i], o.x, o.y, o.z, worldsize>>2);
    }
    calcmerges();
    if(!local) allchanged();
}

void remip_()
{
    mpremip(true);
    allchanged();
}

COMMANDN(remip, remip_, "");

uchar &edgelookup(cube &c, const ivec &p, int dim)
{
   return c.edges[dim*4 +(p[C[dim]]>>3)*2 +(p[R[dim]]>>3)];
}

int edgeval(cube &c, const ivec &p, int dim, int coord)
{
    return edgeget(edgelookup(c,p,dim), coord);
}  
    
void genvertp(cube &c, ivec &p1, ivec &p2, ivec &p3, plane &pl)
{
    int dim = 0;
    if(p1.y==p2.y && p2.y==p3.y) dim = 1;
    else if(p1.z==p2.z && p2.z==p3.z) dim = 2;

    int coord = p1[dim];
    
    ivec v1(p1), v2(p2), v3(p3);
    v1[D[dim]] = edgeval(c, p1, dim, coord);
    v2[D[dim]] = edgeval(c, p2, dim, coord);
    v3[D[dim]] = edgeval(c, p3, dim, coord);
    
    pl.toplane(v1.tovec(), v2.tovec(), v3.tovec());
}
    
bool threeplaneintersect(plane &pl1, plane &pl2, plane &pl3, vec &dest)
{
    vec &t1 = dest, t2, t3, t4;
    t1.cross(pl1, pl2); t4 = t1; t1.mul(pl3.offset);
    t2.cross(pl3, pl1);          t2.mul(pl2.offset);
    t3.cross(pl2, pl3);          t3.mul(pl1.offset);
    t1.add(t2);
    t1.add(t3);
    t1.mul(-1);
    float d = t4.dot(pl3);
    if(d==0) return false;
    t1.div(d);
    return true;
}

void genedgespanvert(ivec &p, cube &c, vec &v)
{
    ivec p1(8-p.x, p.y, p.z);
    ivec p2(p.x, 8-p.y, p.z);
    ivec p3(p.x, p.y, 8-p.z);

    cube s;
    solidfaces(s);

    plane plane1, plane2, plane3;
    genvertp(c, p, p1, p2, plane1);
    genvertp(c, p, p2, p3, plane2);
    genvertp(c, p, p3, p1, plane3);
    if(plane1==plane2) genvertp(s, p, p1, p2, plane1);
    if(plane1==plane3) genvertp(s, p, p1, p2, plane1);
    if(plane2==plane3) genvertp(s, p, p2, p3, plane2);

    ASSERT(threeplaneintersect(plane1, plane2, plane3, v));
    //ASSERT(v.x>=0 && v.x<=8);
    //ASSERT(v.y>=0 && v.y<=8);
    //ASSERT(v.z>=0 && v.z<=8);
    v.x = max(0.0f, min(8.0f, v.x));
    v.y = max(0.0f, min(8.0f, v.y));
    v.z = max(0.0f, min(8.0f, v.z));
}

void edgespan2vectorcube(cube &c)
{   
    vec v;

    if(c.children) loopi(8) edgespan2vectorcube(c.children[i]);

    if(isentirelysolid(c) || isempty(c)) return;

    cube n = c;

    loop(x,2) loop(y,2) loop(z,2)
    {
        ivec p(8*x, 8*y, 8*z);
        genedgespanvert(p, c, v);

        edgeset(cubeedge(n, 0, y, z), x, int(v.x+0.49f));
        edgeset(cubeedge(n, 1, z, x), y, int(v.y+0.49f));
        edgeset(cubeedge(n, 2, x, y), z, int(v.z+0.49f));
    }

    c = n;
}

void converttovectorworld()
{
    conoutf(CON_WARN, "WARNING: old map, use savecurrentmap");
    loopi(8) edgespan2vectorcube(worldroot[i]);
}

void genvectorvert(const ivec &p, cube &c, ivec &v)
{
    v.x = edgeval(c, p, 0, p.x);
    v.y = edgeval(c, p, 1, p.y);
    v.z = edgeval(c, p, 2, p.z);
}

const ivec cubecoords[8] = // verts of bounding cube
{
    ivec(8, 8, 0),
    ivec(0, 8, 0),
    ivec(0, 8, 8),
    ivec(8, 8, 8),
    ivec(8, 0, 8),
    ivec(0, 0, 8),
    ivec(0, 0, 0),
    ivec(8, 0, 0),
};

const ushort fv[6][4] = // indexes for cubecoords, per each vert of a face orientation
{
    { 2, 1, 6, 5 },
    { 3, 4, 7, 0 },
    { 4, 5, 6, 7 },
    { 1, 2, 3, 0 },
    { 6, 1, 0, 7 },
    { 5, 4, 3, 2 },
};

const uchar fvmasks[64] = // mask of verts used given a mask of visible face orientations
{
    0x00, 0x66, 0x99, 0xFF, 0xF0, 0xF6, 0xF9, 0xFF,
    0x0F, 0x6F, 0x9F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xC3, 0xE7, 0xDB, 0xFF, 0xF3, 0xF7, 0xFB, 0xFF,
    0xCF, 0xEF, 0xDF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0x3C, 0x7E, 0xBD, 0xFF, 0xFC, 0xFE, 0xFD, 0xFF,
    0x3F, 0x7F, 0xBF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
};

const uchar faceedgesidx[6][4] = // ordered edges surrounding each orient
{//1st face,2nd face
    { 4, 5, 8, 10 },
    { 6, 7, 9, 11 },
    { 0, 2, 8, 9  },
    { 1, 3, 10,11 },
    { 0, 1, 4, 6 },
    { 2, 3, 5, 7 },
};

const uchar faceedgesrcidx[6][4] =
{//0..1 = row edges, 2..3 = column edges
    { 4,  5,  8, 10 },
    { 6,  7,  9, 11 },
    { 8,  9,  0, 2 },
    { 10, 11, 1, 3 },
    { 0,  1,  4, 6 },
    { 2,  3,  5, 7 },
};

bool flataxisface(cube &c, int orient)
{
    uint face = c.faces[dimension(orient)];
    if(dimcoord(orient)) face >>= 4;
    face &= 0x0F0F0F0F;
    return face == 0x01010101*(face&0x0F);
}

bool touchingface(cube &c, int orient)
{
    uint face = c.faces[dimension(orient)];
    return dimcoord(orient) ? (face&0xF0F0F0F0)==0x80808080 : (face&0x0F0F0F0F)==0;
}

int faceconvexity(cube &c, int orient)
{
   /* // fast approximation
    vec v[4];
    int d = dimension(orient);
    loopi(4) vertrepl(c, *(ivec *)cubecoords[fv[orient][i]], v[i], d, dimcoord(orient));
    int n = (int)(v[0][d] - v[1][d] + v[2][d] - v[3][d]);
    if (!dimcoord(orient)) n *= -1;
    return n; // returns +ve if convex when tris are verts 012, 023. -ve for concave.
    */
    // slow perfect
    ivec v[4];

    if(flataxisface(c, orient)) return 0;

    loopi(4) genvectorvert(cubecoords[fv[orient][i]], c, v[i]);

    ivec n;
    n.cross(v[1].sub(v[0]), v[2].sub(v[0]));
    int x = n.dot(v[0]), y = n.dot(v[3]);
    if(x < y) return -1;     // concave
    else if(x > y) return 1; // convex
    else return 0;           // flat
}

int faceorder(cube &c, int orient)
{
/*
    uchar *edges = &c.edges[4*dimension(orient)];
    uchar h[4];
    loopi(4) h[i] = dimcoord(orient) ? edges[i]>>4 : 8-(edges[i]&0xF);
    if(h[0]+h[3]<h[1]+h[2]) return 1;
    else return 0;
*/
    return faceconvexity(c, orient)<0 ? 1 : 0;
}

int faceverts(cube &c, int orient, int vert) // gets above 'fv' so that each face is convex
{
    return fv[orient][(vert + faceorder(c, orient))&3];
}

uint faceedges(cube &c, int orient)
{
    uchar edges[4];
    loopk(4) edges[k] = c.edges[faceedgesidx[orient][k]];
    return *(uint *)edges;
}

struct facevec
{
    int x, y;

    facevec() {}
    facevec(int x, int y) : x(x), y(y) {}

    bool operator==(const facevec &f) const { return x == f.x && y == f.y; }
};

static inline void genfacevecs(cube &c, int orient, const ivec &pos, int size, bool solid, facevec *fvecs)
{
    int dim = dimension(orient), coord = dimcoord(orient);
    const ushort *fvo = fv[orient];
    loopi(4)
    {
        const ivec &cc = cubecoords[fvo[i]];
        facevec &f = fvecs[coord ? i : 3 - i];
        int x, y;
        if(solid)
        {
            x = cc[C[dim]];
            y = cc[R[dim]];
        }
        else
        {
            x = edgeval(c, cc, C[dim], cc[C[dim]]);
            y = edgeval(c, cc, R[dim], cc[R[dim]]);
        }
        f.x = x*size/(8>>VVEC_FRAC)+(pos[C[dim]]<<VVEC_FRAC);
        f.y = y*size/(8>>VVEC_FRAC)+(pos[R[dim]]<<VVEC_FRAC);
    }
}

static inline int genoppositefacevecs(cube &c, int orient, const ivec &pos, int size, facevec *fvecs)
{
    int dim = dimension(orient), coord = dimcoord(orient), touching = 0;
    const ushort *fvo = fv[orient];
    loopi(4)
    {
        const ivec &cc = cubecoords[fvo[coord ? i : 3 - i]];
        if(edgeval(c, cc, dim, cc[dim]) == coord*8)
        {
            int x = edgeval(c, cc, C[dim], cc[C[dim]]),
                y = edgeval(c, cc, R[dim], cc[R[dim]]);
            facevec &f = fvecs[touching++];
            f.x = x*size/(8>>VVEC_FRAC)+(pos[C[dim]]<<VVEC_FRAC);
            f.y = y*size/(8>>VVEC_FRAC)+(pos[R[dim]]<<VVEC_FRAC);
        }
    }
    return touching;
}

static inline int clipfacevecy(const facevec &o, const facevec &dir, int cx, int cy, int size, facevec &r)
{
    if(dir.x >= 0)
    {
        if(cx <= o.x || cx >= o.x+dir.x) return 0;
    }
    else if(cx <= o.x+dir.x || cx >= o.x) return 0;

    int t = (o.y-cy) + (cx-o.x)*dir.y/dir.x;
    if(t <= 0 || t >= size) return 0;

    r.x = cx;
    r.y = cy + t;
    return 1;
}

static inline int clipfacevecx(const facevec &o, const facevec &dir, int cx, int cy, int size, facevec &r)
{
    if(dir.y >= 0)
    {
        if(cy <= o.y || cy >= o.y+dir.y) return 0;
    }
    else if(cy <= o.y+dir.y || cy >= o.y) return 0;

    int t = (o.x-cx) + (cy-o.y)*dir.x/dir.y;
    if(t <= 0 || t >= size) return 0;

    r.x = cx + t;
    r.y = cy;
    return 1;
}

static inline int clipfacevec(const facevec &o, const facevec &dir, int cx, int cy, int size, facevec *rvecs)
{
    int r = 0;

    if(o.x >= cx && o.x <= cx+size &&
       o.y >= cy && o.y <= cy+size &&
       ((o.x != cx && o.x != cx+size) || (o.y != cy && o.y != cy+size)))
    {
        rvecs[0].x = o.x;
        rvecs[0].y = o.y;
        r++;
    }

    r += clipfacevecx(o, dir, cx, cy, size, rvecs[r]);
    r += clipfacevecx(o, dir, cx, cy+size, size, rvecs[r]);
    r += clipfacevecy(o, dir, cx, cy, size, rvecs[r]);
    r += clipfacevecy(o, dir, cx+size, cy, size, rvecs[r]);

    ASSERT(r <= 2);
    return r;
}

static inline bool insideface(const facevec *p, int nump, const facevec *o, int numo)
{
    int bounds = 0;
    loopi(numo)
    {
        const facevec &cur = o[i], &next = o[i+1 < numo ? i+1 : 0];
        if(cur == next) continue;
        facevec dir(next.x-cur.x, next.y-cur.y);
        int offset = dir.x*cur.y - dir.y*cur.x;
        loopj(nump) if(dir.x*p[j].y - dir.y*p[j].x > offset) return false;
        bounds++;
    }
    return bounds>=3;
}

static inline int clipfacevecs(const facevec *o, int cx, int cy, int size, facevec *rvecs)
{   
    cx <<= VVEC_FRAC;
    cy <<= VVEC_FRAC;
    size <<= VVEC_FRAC;

    int r = 0;
    loopi(4)
    {
        const facevec &cur = o[i], &next = o[(i+1)%4];
        if(cur == next) continue; 
        facevec dir(next.x-cur.x, next.y-cur.y); 
        r += clipfacevec(cur, dir, cx, cy, size, &rvecs[r]);
    }
    facevec corner[4] = {facevec(cx, cy), facevec(cx+size, cy), facevec(cx+size, cy+size), facevec(cx, cy+size)};
    loopi(4) if(insideface(&corner[i], 1, o, 4)) rvecs[r++] = corner[i];
    ASSERT(r <= 8);
    return r;
}

bool collapsedface(uint cfe)
{   
    return ((cfe >> 4) & 0x0F0F) == (cfe & 0x0F0F) ||
           ((cfe >> 20) & 0x0F0F) == ((cfe >> 16) & 0x0F0F);
}  
   
static inline bool occludesface(cube &c, int orient, const ivec &o, int size, const ivec &vo, int vsize, uchar vmat, uchar nmat, uchar matmask, const facevec *vf)
{
    int dim = dimension(orient);
    if(!c.children)
    {
         if(nmat != MAT_AIR && c.ext && (c.ext->material&matmask) == nmat)
         {
            facevec nf[8];
            return clipfacevecs(vf, o[C[dim]], o[R[dim]], size, nf) < 3;
         }
         if(isentirelysolid(c)) return true;
         if(vmat != MAT_AIR && c.ext && ((c.ext->material&matmask) == vmat || (isliquid(vmat) && isclipped(c.ext->material&MATF_VOLUME)))) return true;
         if(touchingface(c, orient) && faceedges(c, orient) == F_SOLID) return true;
         facevec cf[8];
         int numc = clipfacevecs(vf, o[C[dim]], o[R[dim]], size, cf);
         if(numc < 3) return true;
         if(isempty(c)) return false;
         facevec of[4];
         int numo = genoppositefacevecs(c, orient, o, size, of);
         return numo >= 3 && insideface(cf, numc, of, numo);
    }

    size >>= 1;
    int coord = dimcoord(orient);
    loopi(8) if(octacoord(dim, i) == coord)
    {
        if(!occludesface(c.children[i], orient, ivec(i, o.x, o.y, o.z, size), size, vo, vsize, vmat, nmat, matmask, vf)) return false;
    }

    return true;
}

bool visibleface(cube &c, int orient, int x, int y, int z, int size, uchar mat, uchar nmat, uchar matmask)
{
    uint cfe = faceedges(c, orient);
    if(mat != MAT_AIR)
    {
        if(cfe==F_SOLID && touchingface(c, orient)) return false;
    }
    else
    {
        if(!touchingface(c, orient)) return true;
        if(collapsedface(cfe)) return false;
    }

    cube &o = neighbourcube(x, y, z, size, -size, orient);
    if(&o==&c) return false;

    if(lusize > size || (lusize == size && !o.children))
    {
        if(nmat != MAT_AIR && o.ext && (o.ext->material&matmask) == nmat) return true;
        if(isentirelysolid(o)) return false;
        if(mat != MAT_AIR && o.ext && ((o.ext->material&matmask) == mat || (isliquid(mat) && (o.ext->material&MATF_VOLUME) == MAT_GLASS))) return false;
        if(isempty(o)) return true;
        if(touchingface(o, opposite(orient)) && faceedges(o, opposite(orient)) == F_SOLID) return false;

        ivec vo(x, y, z);
        vo.mask(VVEC_INT_MASK);
        lu.mask(VVEC_INT_MASK);
        facevec cf[4], of[4];
        genfacevecs(c, orient, vo, size, mat != MAT_AIR, cf);
        int numo = genoppositefacevecs(o, opposite(orient), lu, lusize, of);
        return numo < 3 || !insideface(cf, 4, of, numo);
    }

    ivec vo(x, y, z);
    vo.mask(VVEC_INT_MASK);
    lu.mask(VVEC_INT_MASK);
    facevec cf[4];
    genfacevecs(c, orient, vo, size, mat != MAT_AIR, cf);
    return !occludesface(o, opposite(orient), lu, lusize, vo, size, mat, nmat, matmask, cf);
}

// more expensive version that checks both triangles of a face independently
int visibletris(cube &c, int orient, int x, int y, int z, int size)
{
    int dim = dimension(orient), coord = dimcoord(orient);
    uint face = c.faces[dim];
    if(coord) face = (face&0xF0F0F0F0)^0x80808080;
    else face &= 0x0F0F0F0F;

    int notouch = 0;
    if(face&0xFF) notouch++;
    if(face&0xFF00) notouch++;
    if(face&0xFF0000) notouch++;
    if(face&0xFF000000) notouch++; 
    if(notouch>=2) return 3;

    if(collapsedface(faceedges(c, orient))) return 0;

    cube &o = neighbourcube(x, y, z, size, -size, orient);
    if(&o==&c) return 0;

    ivec vo(x, y, z);
    vo.mask(VVEC_INT_MASK);
    lu.mask(VVEC_INT_MASK);
    facevec cf[4], of[4];
    int opp = opposite(orient), numo = 0;
    if(lusize > size || (lusize == size && !o.children))
    {
        if(isempty(o)) return 3;
        if(!notouch && (isentirelysolid(o) || (touchingface(o, opp) && faceedges(o, opp) == F_SOLID))) return 0;

        genfacevecs(c, orient, vo, size, false, cf);
        numo = genoppositefacevecs(o, opp, lu, lusize, of);
        if(numo < 3) return 3;
        if(!notouch && insideface(cf, 4, of, numo)) return 0; 
    }
    else
    {
        genfacevecs(c, orient, vo, size, false, cf);
        if(!notouch && occludesface(o, opp, lu, lusize, vo, size, MAT_AIR, MAT_AIR, MATF_VOLUME, cf)) return 0;
    }

    static const int trimasks[2][2] = { { 0x7, 0xD }, { 0xE, 0xB } };
    static const int triverts[2][2][2][3] =
    { // order
        { // coord
            { { 1, 2, 3 }, { 0, 1, 3 } }, // verts
            { { 0, 1, 2 }, { 0, 2, 3 } }
        },
        { // coord
            { { 0, 1, 2 }, { 3, 0, 2 } }, // verts
            { { 1, 2, 3 }, { 1, 3, 0 } }
        }
    };

    int convex = faceconvexity(c, orient),
        order = convex < 0 ? 1 : 0,
        vis = 3, 
        touching = 0;
    loopi(4)
    {
        const ivec &cc = cubecoords[fv[orient][i]];
        if(edgeval(c, cc, dim, cc[dim]) == coord*8) touching |= 1<<i;
    }
    facevec tf[4];
    
    for(;;)
    {
        loopi(2) if((touching&trimasks[order][i])==trimasks[order][i])
        {    
            const int *verts = triverts[order][coord][i];
            int v1 = verts[0], v2 = verts[1], v3 = verts[2];
            if(cf[v1]==cf[v2] || cf[v2]==cf[v3] || cf[v3]==cf[v1]) { notouch = 1; continue; }
            tf[0] = cf[v1]; tf[1] = cf[v2]; tf[2] = cf[v3];
            if(!numo)
            {
                tf[3] = cf[v3];
                if(!occludesface(o, opp, lu, lusize, vo, size, MAT_AIR, MAT_AIR, MATF_VOLUME, tf)) continue;
            }
            else if(!insideface(tf, 3, of, numo)) continue;
            return vis & ~(1<<i);
        }
        if(notouch || vis&4) break;
        if(c.ext && c.ext->surfaces) // compat for old lightmaps that can't be reordered
        {
            const uchar *tc = c.ext->surfaces[orient].texcoords;
            if((tc[0]!=tc[6] || tc[1]!=tc[7]) && (tc[0]!=tc[2] || tc[1]!=tc[3])) break;
        }
        vis |= 4;
        order++;
    }
    
    return 3;
}

void calcvert(cube &c, int x, int y, int z, int size, vvec &v, int i, bool solid)
{
    ivec vv;
    if(solid || isentirelysolid(c)) vv = cubecoords[i];
    else genvectorvert(cubecoords[i], c, vv);
    v = vvec(vv.v);
    // avoid overflow
    if(size>=8) v.mul(size/8);
    else v.div(8/size);
    v.add(vvec(x, y, z));
}

void calcvert(cube &c, int x, int y, int z, int size, vec &v, int i, bool solid)
{
    ivec vv;
    if(solid || isentirelysolid(c)) vv = cubecoords[i];
    else genvectorvert(cubecoords[i], c, vv);
    v = vv.tovec().mul(size/8.0f).add(vec(x, y, z));
}

int calcverts(cube &c, int x, int y, int z, int size, vvec *verts, bool *usefaces)
{
    int vertused = 0;
    loopi(6) if((usefaces[i] = visibleface(c, i, x, y, z, size))) vertused |= fvmasks[1<<i];
    //loopk(4) vertused |= 1<<faceverts(c,i,k);
    loopi(8) if(vertused&(1<<i)) calcvert(c, x, y, z, size, verts[i], i);
    return vertused;
}

int genclipplane(cube &c, int orient, vec *v, plane *clip)
{
    int planes = 0;
    vec p[4];
    loopk(4) p[k] = v[faceverts(c,orient,k)];

    if(p[0]==p[2]) return 0;
    if(p[0]!=p[1] && p[1]!=p[2]) clip[planes++].toplane(p[0], p[1], p[2]);
    if(p[0]!=p[3] && p[2]!=p[3] && (!planes || faceconvexity(c, orient))) clip[planes++].toplane(p[0], p[2], p[3]);
    return planes;
}

void genclipplanes(cube &c, int x, int y, int z, int size, clipplanes &p)
{
    bool usefaces[6];
    vvec sv[8];
    vec v[8];
    vec mx(x, y, z), mn(x+size, y+size, z+size);
    int vertused = calcverts(c, x, y, z, size, sv, usefaces);

    loopi(8)
    {
        if(!(vertused&(1<<i))) // need all verts for proper box
            calcvert(c, x, y, z, size, sv[i], i);

        v[i] = sv[i].tovec(x, y, z);

        loopj(3) // generate tight bounding box
        {
            mn[j] = min(mn[j], v[i].v[j]);
            mx[j] = max(mx[j], v[i].v[j]);
        }
    }

    p.r = mx;     // radius of box
    p.r.sub(mn);
    p.r.mul(0.5f);
    p.o = mn;     // center of box
    p.o.add(p.r);

    p.size = 0;
    loopi(6) if(usefaces[i] && !touchingface(c, i)) // generate actual clipping planes
    {
        p.size += genclipplane(c, i, v, &p.p[p.size]);
    }
}

static int mergefacecmp(const cubeface *x, const cubeface *y)
{
    if(x->v2 < y->v2) return -1;
    if(x->v2 > y->v2) return 1;
    if(x->u1 < y->u1) return -1;
    if(x->u1 > y->u1) return 1;
    return 0;
}

static int mergefacev(int orient, cubeface *m, int sz, cubeface &n)
{
    for(int i = sz-1; i >= 0; --i)
    {
        if(m[i].v2 < n.v1) break;
        if(m[i].v2 == n.v1 && m[i].u1 == n.u1 && m[i].u2 == n.u2)
        {
            if(m[i].c) ext(*m[i].c).merged |= 1<<orient;
            if(n.c) ext(*n.c).merged |= 1<<orient;
            n.v1 = m[i].v1;
            memmove(&m[i], &m[i+1], (sz - (i+1)) * sizeof(cubeface));
            return 1;
        }
    }
    return 0;
}

static int mergefaceu(int orient, cubeface &m, cubeface &n)
{
    if(m.v1 == n.v1 && m.v2 == n.v2 && m.u2 == n.u1)
    {
        if(m.c) ext(*m.c).merged |= 1<<orient;
        if(n.c) ext(*n.c).merged |= 1<<orient;
        n.u1 = m.u1;
        return 1;
    }
    return 0;
}

static int mergeface(int orient, cubeface *m, int sz, cubeface &n)
{  
    for(bool merged = false; sz; merged = true)
    {
        int vmerged = mergefacev(orient, m, sz, n);
        sz -= vmerged; 
        if(!vmerged && merged) break;
        if(!sz) break;
        int umerged = mergefaceu(orient, m[sz-1], n);
        sz -= umerged;
        if(!umerged) break;
    }
    m[sz++] = n;
    return sz;
}

int mergefaces(int orient, cubeface *m, int sz)
{   
    qsort(m, sz, sizeof(cubeface), (int (__cdecl *)(const void *, const void *))mergefacecmp);

    int nsz = 0;
    loopi(sz) nsz = mergeface(orient, m, nsz, m[i]);
    return nsz;
}

struct cfkey
{   
    uchar orient;
    ushort tex;
    ivec n;
    int offset;
};

static inline bool htcmp(const cfkey &x, const cfkey &y)
{   
    return x.orient == y.orient && x.tex == y.tex && x.n == y.n && x.offset == y.offset;
}

static inline uint hthash(const cfkey &k)
{   
    return hthash(k.n)^k.offset^k.tex^k.orient;
}

struct cfval
{   
    vector<cubeface> faces;
};

static hashtable<cfkey, cfval> cfaces;

void mincubeface(cube &cu, int orient, const ivec &o, int size, const mergeinfo &orig, mergeinfo &cf)
{
    int dim = dimension(orient);
    if(cu.children)
    {
        size >>= 1;
        int coord = dimcoord(orient);
        loopi(8) if(octacoord(dim, i) == coord)
            mincubeface(cu.children[i], orient, ivec(i, o.x, o.y, o.z, size), size, orig, cf);
        return;
    }
    int c = C[dim], r = R[dim];
    ushort uco = ushort((o[c]&VVEC_INT_MASK)<<VVEC_FRAC), vco = ushort((o[r]&VVEC_INT_MASK)<<VVEC_FRAC);
    ushort uc1 = uco, vc1 = vco, uc2 = ushort(size<<VVEC_FRAC)+uco, vc2 = ushort(size<<VVEC_FRAC)+vco;
    uc1 = max(uc1, orig.u1);
    uc2 = min(uc2, orig.u2);
    vc1 = max(vc1, orig.v1);
    vc2 = min(vc2, orig.v2);
    if(!isempty(cu) && touchingface(cu, orient))
    {
        uchar r1 = cu.edges[faceedgesrcidx[orient][0]], r2 = cu.edges[faceedgesrcidx[orient][1]],
              c1 = cu.edges[faceedgesrcidx[orient][2]], c2 = cu.edges[faceedgesrcidx[orient][3]];
        ushort scale = ushort(size/(8>>VVEC_FRAC)),
              u1 = max(c1&0xF, c2&0xF)*scale+uco, u2 = min(c1>>4, c2>>4)*scale+uco,
              v1 = max(r1&0xF, r2&0xF)*scale+vco, v2 = min(r1>>4, r2>>4)*scale+vco;
        u1 = max(u1, orig.u1);
        u2 = min(u2, orig.u2);
        v1 = max(v1, orig.v1);
        v2 = min(v2, orig.v2);
        if(v2-v1==vc2-vc1)
        {
            if(u2-u1==uc2-uc1) return;
            if(u1==uc1) uc1 = u2;
            if(u2==uc2) uc2 = u1;
        }
        else if(u2-u1==uc2-uc1)
        {
            if(v1==vc1) vc1 = v2;
            if(v2==vc2) vc2 = v1;
        }    
    }
    if(uc1==uc2 || vc1==vc2) return;
    cf.u1 = min(cf.u1, uc1);
    cf.u2 = max(cf.u2, uc2);
    cf.v1 = min(cf.v1, vc1);
    cf.v2 = max(cf.v2, vc2);
}

bool mincubeface(cube &cu, int orient, const ivec &co, int size, mergeinfo &orig)
{
    cube &nc = neighbourcube(co.x, co.y, co.z, size, -size, orient);
    mergeinfo mincf;
    mincf.u1 = orig.u2;
    mincf.u2 = orig.u1;
    mincf.v1 = orig.v2;
    mincf.v2 = orig.v1;
    mincubeface(nc, opposite(orient), lu, lusize, orig, mincf);
    bool smaller = false;
    if(mincf.u1 > orig.u1) { orig.u1 = mincf.u1; smaller = true; }
    if(mincf.u2 < orig.u2) { orig.u2 = mincf.u2; smaller = true; }
    if(mincf.v1 > orig.v1) { orig.v1 = mincf.v1; smaller = true; }
    if(mincf.v2 < orig.v2) { orig.v2 = mincf.v2; smaller = true; }
    return smaller;
}

VAR(minface, 0, 1, 1);

bool gencubeface(cube &cu, int orient, const ivec &co, int size, ivec &n, int &offset, cubeface &cf)
{   
    uchar cfe[4];
    *(uint *)cfe = faceedges(cu, orient);
    if(cfe[0]!=cfe[1] || cfe[2]!=cfe[3] || (cfe[0]>>4)==(cfe[0]&0xF) || (cfe[2]>>4)==(cfe[2]&0xF)) return false;
    if(faceconvexity(cu, orient)) return false;

    cf.c = &cu;

    ivec v[4];
    loopi(4) genvectorvert(cubecoords[fv[orient][i]], cu, v[i]);

    int scale = size/(8>>VVEC_FRAC);
    v[3].mul(scale);
    int dim = dimension(orient), c = C[dim], r = R[dim];
    cf.u1 = cf.u2 = ushort(v[3][c]);
    cf.v1 = cf.v2 = ushort(v[3][r]);

    loopi(3)
    {
        ushort uc = ushort(v[i][c]*scale), vc = ushort(v[i][r]*scale);
        cf.u1 = min(cf.u1, uc);
        cf.u2 = max(cf.u2, uc);
        cf.v1 = min(cf.v1, vc);
        cf.v2 = max(cf.v2, vc);
    }

    ivec vo(co);
    vo.mask(VVEC_INT_MASK);
    vo.mul(1<<VVEC_FRAC);

    ushort uco = ushort(vo[c]), vco = ushort(vo[r]);
    cf.u1 += uco;
    cf.u2 += uco;
    cf.v1 += vco;
    cf.v2 += vco;

    v[1].sub(v[0]);
    v[2].sub(v[0]);
    n.cross(v[1], v[2]);

    // reduce the normal as much as possible without resorting to floating point
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
    if(!(n[R[mindim]]%minval) && !(n[C[mindim]]%minval))
    {
        n[mindim] /= minval;
        n[R[mindim]] /= minval;
        n[C[mindim]] /= minval;
    }
    while((n[0]&1)==0 && (n[1]&1)==0 && (n[2]&1)==0)
    {
        n[0] >>= 1;
        n[1] >>= 1;
        n[2] >>= 1;
    }

    v[3].add(vo);
    offset = -n.dot(v[3]);

    if(minface && touchingface(cu, orient) && mincubeface(cu, orient, co, size, cf))
    {
        ext(cu).merged |= 1<<orient;
    }    

    return true;
}

void addmergeinfo(cube &c, int orient, cubeface &cf)
{
    if(!c.ext) newcubeext(c);
    int index = 0;
    loopi(orient) if(c.ext->mergeorigin&(1<<i)) index++;
    int total = index;
    loopi(6-orient-1) if(c.ext->mergeorigin&(1<<(i+orient+1))) total++;
    mergeinfo *m = new mergeinfo[total+1];
    if(index) memcpy(m, c.ext->merges, index*sizeof(mergeinfo));
    if(total>index) memcpy(&m[index+1], &c.ext->merges[index], (total-index)*sizeof(mergeinfo));
    if(c.ext->merges) delete[] c.ext->merges;
    c.ext->merges = m;
    m += index;
    c.ext->mergeorigin |= 1<<orient;
    m->u1 = cf.u1;
    m->u2 = cf.u2;
    m->v1 = cf.v1;
    m->v2 = cf.v2;
}

void freemergeinfo(cube &c)
{
    if(!c.ext) return;
    c.ext->mergeorigin = 0;
    DELETEA(c.ext->merges);
}

VAR(maxmerge, 0, 6, VVEC_INT-1);

static int genmergeprogress = 0; 

void genmergeinfo(cube *c = worldroot, const ivec &o = ivec(0, 0, 0), int size = worldsize>>1)
{
    if((genmergeprogress++&0xFFF)==0) renderprogress(float(genmergeprogress)/allocnodes, "merging surfaces...");
    loopi(8)
    {
        ivec co(i, o.x, o.y, o.z, size);
        if(c[i].ext)
        {
            if(c[i].ext->merges) freemergeinfo(c[i]);
            c[i].ext->merged = 0;
        }
        if(c[i].children) genmergeinfo(c[i].children, co, size>>1);
        else if(!isempty(c[i])) loopj(6) if(visibleface(c[i], j, co.x, co.y, co.z, size))
        {
            cfkey k;
            cubeface cf;
            if(gencubeface(c[i], j, co, size, k.n, k.offset, cf))
            {
                if(size >= 1<<maxmerge || c == worldroot)
                {
                    if(c[i].ext && c[i].ext->merged&(1<<j)) addmergeinfo(c[i], j, cf);
                    continue;
                }
                k.orient = j;
                k.tex = c[i].texture[j];
                cfaces[k].faces.add(cf);
            }
        }
        if((size == 1<<maxmerge || c == worldroot) && cfaces.numelems)
        {
            ASSERT(size <= 1<<maxmerge);
            enumeratekt(cfaces, cfkey, key, cfval, val,
                val.faces.setsize(mergefaces(key.orient, val.faces.getbuf(), val.faces.length()));
                loopvj(val.faces) if(val.faces[j].c->ext && val.faces[j].c->ext->merged&(1<<key.orient))
                {
                    addmergeinfo(*val.faces[j].c, key.orient, val.faces[j]);
                }
            );
            cfaces.clear();
        }

    }
}

void genmergedverts(cube &cu, int orient, const ivec &co, int size, const mergeinfo &m, vvec *vv, plane *p)
{   
    ivec vo(co);
    vo.mask(VVEC_INT_MASK);
    vo.mul(1<<VVEC_FRAC);

    ivec v[3], n;
    loopi(3) genvectorvert(cubecoords[faceverts(cu, orient, i)], cu, v[i]);
    v[1].sub(v[0]);
    v[2].sub(v[0]);
    n.cross(v[1], v[2]);

    ASSERT(size >= 8>>VVEC_FRAC);
    v[0].mul(size/(8>>VVEC_FRAC));
    v[0].add(vo);
    int offset = -n.dot(v[0]);

    int dim = dimension(orient), c = C[dim], r = R[dim];
    loopi(4)
    {
        const ivec &coords = cubecoords[fv[orient][i]];
        int cc = coords[c] ? m.u2 : m.u1,
            rc = coords[r] ? m.v2 : m.v1,
            dc = -(offset + n[c]*cc + n[r]*rc)/n[dim];
        vv[i][c] = ushort(cc);
        vv[i][r] = ushort(rc);
        vv[i][dim] = ushort(dc);
    }

    if(p) 
    {
        ivec po(co);
        po.mask(~VVEC_INT_MASK);
        vec pn(n.tovec());
        float mag = pn.magnitude();
        pn.div(mag);
        *p = plane(pn, (offset-(n.dot(po)<<VVEC_FRAC))/(mag*(1<<VVEC_FRAC)));
    }
}

int calcmergedsize(int orient, const ivec &co, int size, const mergeinfo &m, const vvec *vv)
{
    int dim = dimension(orient), c = C[dim], r = R[dim];
    ushort d1 = vv[3][dim], d2 = d1;
    loopi(3)
    {
        d1 = min(d1, vv[i][dim]);
        d2 = max(d2, vv[i][dim]);
    }
    int bits = 0;
    while(1<<bits < size) ++bits;
    bits += VVEC_FRAC;
    ivec mo(co);
    mo.mask(VVEC_INT_MASK);
    mo.mul(1<<VVEC_FRAC);
    while(bits<VVEC_INT+VVEC_FRAC-1)
    {
        mo.mask(~((1<<bits)-1));
        if(mo[dim] <= d1 && mo[dim] + (1<<bits) >= d2 &&
           mo[c] <= m.u1 && mo[c] + (1<<bits) >= m.u2 &&
           mo[r] <= m.v1 && mo[r] + (1<<bits) >= m.v2)
            break;
        bits++;
    }
    return bits-VVEC_FRAC;
}

static void invalidatemerges(cube &c)
{   
    if(c.ext)
    {
        if(c.ext->va)
        {
            if(!(c.ext->va->hasmerges&(MERGE_PART | MERGE_ORIGIN))) return;
            destroyva(c.ext->va);
            c.ext->va = NULL;
        }
        if(c.ext->merged)
        {
            brightencube(c);
            c.ext->merged = 0;
            if(c.ext->merges) freemergeinfo(c);
        }
        if(c.ext->tjoints >= 0) c.ext->tjoints = -1;
    }
    if(c.children) loopi(8) invalidatemerges(c.children[i]);
}

static int invalidatedmerges = 0;

void invalidatemerges(cube &c, bool msg)
{
    if(msg && invalidatedmerges!=totalmillis)
    {
        renderprogress(0, "invalidating merged surfaces...");
        invalidatedmerges = totalmillis;
    }
    invalidatemerges(c);
}
 
void calcmerges()
{
    genmergeprogress = 0;
    genmergeinfo();
}

