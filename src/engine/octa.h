// 6-directional octree heightfield map format

enum
{
    LAYER_TOP = 0,
    LAYER_BOTTOM,

    LAYER_BLEND = 1<<1
};

struct elementset
{
    ushort texture;
    uchar lmid, layer;
    ushort envmap;
    ushort length[6];
    ushort minvert[6], maxvert[6];
};

enum
{
    EMID_NONE = 0,
    EMID_CUSTOM,
    EMID_SKY,
    EMID_RESERVED
};

struct materialsurface
{
    ivec o;
    ushort csize, rsize;
    union
    {
        short index;
        short depth;
    };
    uchar material, orient;
    union
    {
        entity *light;
        ushort envmap;
        uchar ends;
    };
};

struct surfaceinfo
{
    uchar texcoords[8];
    uchar w, h;
    ushort x, y;
    uchar lmid, layer;
};

struct surfacenormals
{
    bvec normals[4];
};

struct grasstri
{
    vec v[4];
    int numv;
    vec4 tcu, tcv;
    plane surface, e[4];
    vec center;
    float radius;
    ushort texture, lmid;
};

struct occludequery
{
    void *owner;
    GLuint id;
    int fragments;
};

struct vtxarray;

struct octaentities
{
    vector<int> mapmodels;
    vector<int> other;
    occludequery *query;
    octaentities *next, *rnext;
    int distance;
    ivec o;
    int size;
    ivec bbmin, bbmax;

    octaentities(const ivec &o, int size) : query(0), o(o), size(size), bbmin(o), bbmax(o)
    {
        bbmin.add(size);
    }
};

enum
{
    OCCLUDE_NOTHING = 0,
    OCCLUDE_GEOM,
    OCCLUDE_BB,
    OCCLUDE_PARENT
};

enum
{
    MERGE_ORIGIN = 1<<0,
    MERGE_PART   = 1<<1,
    MERGE_USE    = 1<<2
};

struct vtxarray
{
    vtxarray *parent;
    vector<vtxarray *> children;
    vtxarray *next, *rnext; // linked list of visible VOBs
    vertex *vdata;           // vertex data
    ushort voffset;          // offset into vertex data
    ushort *edata, *skydata; // vertex indices
    GLuint vbuf, ebuf, skybuf; // VBOs
    ushort minvert, maxvert; // DRE info
    elementset *eslist;      // List of element indices sets (range) per texture
    materialsurface *matbuf; // buffer of material surfaces
    int verts, tris, texs, blends, texmask, sky, explicitsky, skyfaces, skyclip, matsurfs, distance;
    double skyarea;
    ivec o;
    int size;                // location and size of cube.
    ivec geommin, geommax;   // BB of geom
    ivec shadowmapmin, shadowmapmax; // BB of shadowmapped surfaces
    ivec matmin, matmax;     // BB of any materials
    ivec bbmin, bbmax;       // BB of everything including children
    uchar curvfc, occluded;
    occludequery *query, *rquery;
    vector<octaentities *> *mapmodels;
    vector<grasstri> *grasstris;
    int hasmerges;
    uint dynlightmask;
    bool shadowed;
};

struct cube;

struct clipplanes
{
    vec o, r;
    int size;
    plane p[12];
    cube *owner;
};

struct mergeinfo
{
    ushort u1, u2, v1, v2;
};

struct tjoint
{
    int next;
    ushort offset;
    uchar edge;
};

struct cubeext
{
    uchar material;          // empty-space material
    uchar visible;           // visible faces of the cube
    uchar merged;            // merged faces of the cube
    uchar mergeorigin;       // whether this face describes a larger merged face
    vtxarray *va;            // vertex array for children, or NULL
    clipplanes *clip;        // collision planes
    surfaceinfo *surfaces;   // lighting info for each surface
    surfacenormals *normals; // per-vertex normals for each surface
    octaentities *ents;      // list of map entites totally inside cube
    mergeinfo *merges;       // bounds of merged surfaces
    int tjoints;             // linked list of t-joints
};  

struct cube
{
    cube *children;          // points to 8 cube structures which are its children, or NULL. -Z first, then -Y, -X
    union
    {
        uchar edges[12];     // edges of the cube, each uchar is 2 4bit values denoting the range.
                             // see documentation jpgs for more info.
        uint faces[3];       // 4 edges of each dimension together representing 2 perpendicular faces
    };
    union
    {
        ushort texture[6];       // one for each face. same order as orient.
        struct
        {
            uchar clipmask, vismask;
            uchar vismasks[8];
        };
    };
    cubeext *ext;            // extended info for the cube
};

static inline cubeext &ext(cube &c)
{
    extern cubeext *newcubeext(cube &c);
    return *(c.ext ? c.ext : newcubeext(c));
}

struct block3
{
    ivec o, s;
    int grid, orient;
    block3() {}
    block3(const selinfo &sel) : o(sel.o), s(sel.s), grid(sel.grid), orient(sel.orient) {}
    cube *c()           { return (cube *)(this+1); }
    int size()    const { return s.x*s.y*s.z; }
};

struct editinfo
{
    block3 *copy;
    editinfo() : copy(NULL) {}
};

struct undoent   { int i; entity e; };
struct undoblock // undo header, all data sits in payload
{
    undoblock *prev, *next;
    int size, timestamp, numents; // if numents is 0, is a cube undo record, otherwise an entity undo record

    block3 *block() { return (block3 *)(this + 1); }
    int *gridmap()
    {
        block3 *ub = block();
        return (int *)(ub->c() + ub->size());
    }
    undoent *ents() { return (undoent *)(this + 1); }
};

extern cube *worldroot;             // the world data. only a ptr to 8 cubes (ie: like cube.children above)
extern ivec lu;
extern int lusize;
extern int wtris, wverts, vtris, vverts, glde, gbatches, rplanes;
extern int allocnodes, allocva, selchildcount;

const uint F_EMPTY = 0;             // all edges in the range (0,0)
const uint F_SOLID = 0x80808080;    // all edges in the range (0,8)

#define isempty(c) ((c).faces[0]==F_EMPTY)
#define isentirelysolid(c) ((c).faces[0]==F_SOLID && (c).faces[1]==F_SOLID && (c).faces[2]==F_SOLID)
#define setfaces(c, face) { (c).faces[0] = (c).faces[1] = (c).faces[2] = face; }
#define solidfaces(c) setfaces(c, F_SOLID)
#define emptyfaces(c) setfaces(c, F_EMPTY)

#define edgemake(a, b) ((b)<<4|a)
#define edgeget(edge, coord) ((coord) ? (edge)>>4 : (edge)&0xF)
#define edgeset(edge, coord, val) ((edge) = ((coord) ? ((edge)&0xF)|((val)<<4) : ((edge)&0xF0)|(val)))

#define cubeedge(c, d, x, y) ((c).edges[(((d)<<2)+((y)<<1)+(x))])

#define octadim(d)          (1<<(d))                    // creates mask for bit of given dimension
#define octacoord(d, i)     (((i)&octadim(d))>>(d))
#define oppositeocta(d, i)  ((i)^octadim(D[d]))
#define octaindex(d,x,y,z)  (octadim(D[d])*(z)+octadim(C[d])*(y)+octadim(R[d])*(x))
#define octastep(x, y, z, scale) (((((z)>>(scale))&1)<<2) | ((((y)>>(scale))&1)<<1) | (((x)>>(scale))&1))

#define loopoctabox(c, size, o, s) uchar possible = octantrectangleoverlap(c, size, o, s); loopi(8) if(possible&(1<<i))

enum
{
    O_LEFT = 0,
    O_RIGHT,
    O_BACK,
    O_FRONT,
    O_BOTTOM,
    O_TOP
};

#define dimension(orient) ((orient)>>1)
#define dimcoord(orient)  ((orient)&1)
#define opposite(orient)  ((orient)^1)

enum
{
    VFC_FULL_VISIBLE = 0,
    VFC_PART_VISIBLE,
    VFC_FOGGED,
    VFC_NOT_VISIBLE,
    PVS_FULL_VISIBLE,
    PVS_PART_VISIBLE,
    PVS_FOGGED
};

