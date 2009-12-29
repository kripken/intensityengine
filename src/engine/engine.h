#ifndef __ENGINE_H__
#define __ENGINE_H__

#include "cube.h"

#ifndef STANDALONE

#include "world.h"
#include "octa.h"
#include "lightmap.h"
#include "bih.h"
#include "texture.h"
#include "model.h"

// GL_ARB_multitexture
extern PFNGLACTIVETEXTUREARBPROC       glActiveTexture_;
extern PFNGLCLIENTACTIVETEXTUREARBPROC glClientActiveTexture_;
extern PFNGLMULTITEXCOORD2FARBPROC     glMultiTexCoord2f_;
extern PFNGLMULTITEXCOORD3FARBPROC     glMultiTexCoord3f_;
extern PFNGLMULTITEXCOORD4FARBPROC     glMultiTexCoord4f_;

// GL_ARB_vertex_buffer_object
extern PFNGLGENBUFFERSARBPROC       glGenBuffers_;
extern PFNGLBINDBUFFERARBPROC       glBindBuffer_;
extern PFNGLMAPBUFFERARBPROC        glMapBuffer_;
extern PFNGLUNMAPBUFFERARBPROC      glUnmapBuffer_;
extern PFNGLBUFFERDATAARBPROC       glBufferData_;
extern PFNGLBUFFERSUBDATAARBPROC    glBufferSubData_;
extern PFNGLDELETEBUFFERSARBPROC    glDeleteBuffers_;
extern PFNGLGETBUFFERSUBDATAARBPROC glGetBufferSubData_;

// GL_ARB_occlusion_query
extern PFNGLGENQUERIESARBPROC        glGenQueries_;
extern PFNGLDELETEQUERIESARBPROC     glDeleteQueries_;
extern PFNGLBEGINQUERYARBPROC        glBeginQuery_;
extern PFNGLENDQUERYARBPROC          glEndQuery_;
extern PFNGLGETQUERYIVARBPROC        glGetQueryiv_;
extern PFNGLGETQUERYOBJECTIVARBPROC  glGetQueryObjectiv_;
extern PFNGLGETQUERYOBJECTUIVARBPROC glGetQueryObjectuiv_;

// GL_EXT_framebuffer_object
extern PFNGLBINDRENDERBUFFEREXTPROC        glBindRenderbuffer_;
extern PFNGLDELETERENDERBUFFERSEXTPROC     glDeleteRenderbuffers_;
extern PFNGLGENFRAMEBUFFERSEXTPROC         glGenRenderbuffers_;
extern PFNGLRENDERBUFFERSTORAGEEXTPROC     glRenderbufferStorage_;
extern PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC  glCheckFramebufferStatus_;
extern PFNGLBINDFRAMEBUFFEREXTPROC         glBindFramebuffer_;
extern PFNGLDELETEFRAMEBUFFERSEXTPROC      glDeleteFramebuffers_;
extern PFNGLGENFRAMEBUFFERSEXTPROC         glGenFramebuffers_;
extern PFNGLFRAMEBUFFERTEXTURE2DEXTPROC    glFramebufferTexture2D_;
extern PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC glFramebufferRenderbuffer_;
extern PFNGLGENERATEMIPMAPEXTPROC          glGenerateMipmap_;

// GL_EXT_framebuffer_blit
#ifndef GL_EXT_framebuffer_blit
#define GL_READ_FRAMEBUFFER_EXT           0x8CA8
#define GL_DRAW_FRAMEBUFFER_EXT           0x8CA9
#define GL_DRAW_FRAMEBUFFER_BINDING_EXT   0x8CA6
#define GL_READ_FRAMEBUFFER_BINDING_EXT   0x8CAA
typedef void (APIENTRYP PFNGLBLITFRAMEBUFFEREXTPROC) (GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter);
#endif
extern PFNGLBLITFRAMEBUFFEREXTPROC         glBlitFramebuffer_;

// GL_EXT_draw_range_elements
extern PFNGLDRAWRANGEELEMENTSEXTPROC glDrawRangeElements_;

// GL_EXT_blend_minmax
extern PFNGLBLENDEQUATIONEXTPROC glBlendEquation_;

// GL_EXT_blend_color
extern PFNGLBLENDCOLOREXTPROC glBlendColor_;

// GL_EXT_multi_draw_arrays
extern PFNGLMULTIDRAWARRAYSEXTPROC   glMultiDrawArrays_;
extern PFNGLMULTIDRAWELEMENTSEXTPROC glMultiDrawElements_;

// GL_EXT_packed_depth_stencil
#ifndef GL_DEPTH_STENCIL_EXT
#define GL_DEPTH_STENCIL_EXT 0x84F9
#endif
#ifndef GL_DEPTH24_STENCIL8_EXT
#define GL_DEPTH24_STENCIL8_EXT 0x88F0
#endif

// GL_ARB_texture_compression
extern PFNGLCOMPRESSEDTEXIMAGE3DARBPROC    glCompressedTexImage3D_;
extern PFNGLCOMPRESSEDTEXIMAGE2DARBPROC    glCompressedTexImage2D_;
extern PFNGLCOMPRESSEDTEXIMAGE1DARBPROC    glCompressedTexImage1D_;
extern PFNGLCOMPRESSEDTEXSUBIMAGE3DARBPROC glCompressedTexSubImage3D_;
extern PFNGLCOMPRESSEDTEXSUBIMAGE2DARBPROC glCompressedTexSubImage2D_;
extern PFNGLCOMPRESSEDTEXSUBIMAGE1DARBPROC glCompressedTexSubImage1D_;
extern PFNGLGETCOMPRESSEDTEXIMAGEARBPROC   glGetCompressedTexImage_;

extern dynent *player;
extern physent *camera1;                // special ent that acts as camera, same object as player1 in FPS mode

extern int worldscale, worldsize;
extern int mapversion;
extern char *maptitle;
extern vector<ushort> texmru;
extern int xtraverts, xtravertsva;
extern int curtexnum;
extern const ivec cubecoords[8];
extern const ushort fv[6][4];
extern const uchar fvmasks[64];
extern const uchar faceedgesidx[6][4];
extern bool inbetweenframes, renderedframe;

extern SDL_Surface *screen;
extern int zpass, glowpass;

extern vector<int> entgroup;

// rendertext
struct font
{
    struct charinfo
    {
        short x, y, w, h;
    };

    char *name;
    Texture *tex;
    vector<charinfo> chars;
    short defaultw, defaulth;
    short offsetx, offsety, offsetw, offseth;
};

#define FONTH (curfont->defaulth)
#define FONTW (curfont->defaultw)
#define MINRESW 640
#define MINRESH 480

extern font *curfont;

// texture
extern int hwtexsize, hwcubetexsize, hwmaxaniso, maxtexsize;

extern Texture *textureload(const char *name, int clamp = 0, bool mipit = true, bool msg = true);
extern int texalign(void *data, int w, int bpp);
extern void cleanuptexture(Texture *t);
extern void loadalphamask(Texture *t);
extern void loadlayermasks();
extern Texture *cubemapload(const char *name, bool mipit = true, bool msg = true, bool transient = false);
extern void drawcubemap(int size, const vec &o, float yaw, float pitch, const cubemapside &side);
extern Slot &lookupmaterialslot(int slot, bool load = true);
extern Slot &lookuptexture(int slot, bool load = true);
extern void loadshaders();
extern void createtexture(int tnum, int w, int h, void *pixels, int clamp, int filter, GLenum component = GL_RGB, GLenum target = GL_TEXTURE_2D, int pw = 0, int ph = 0, int pitch = 0, bool resize = true);
extern void renderpostfx();
extern void initenvmaps();
extern void genenvmaps();
extern ushort closestenvmap(const vec &o);
extern ushort closestenvmap(int orient, int x, int y, int z, int size);
extern GLuint lookupenvmap(ushort emid);
extern GLuint lookupenvmap(Slot &slot);
extern bool reloadtexture(Texture &tex);
extern bool reloadtexture(const char *name);
extern void setuptexcompress();

// shadowmap

extern int shadowmap, shadowmapcasters;
extern bool shadowmapping;

extern bool isshadowmapcaster(const vec &o, float rad);
extern bool addshadowmapcaster(const vec &o, float xyrad, float zrad);
extern bool isshadowmapreceiver(vtxarray *va);
extern void rendershadowmap();
extern void pushshadowmap();
extern void popshadowmap();
extern void adjustshadowmatrix(const ivec &o, float scale);
extern void rendershadowmapreceivers();
extern void guessshadowdir();

// pvs
extern void clearpvs();
extern bool pvsoccluded(const ivec &bborigin, const ivec &bbsize);
extern bool waterpvsoccluded(int height);
extern void setviewcell(const vec &p);
extern void savepvs(stream *f);
extern void loadpvs(stream *f, int numpvs);
extern int getnumviewcells();

static inline bool pvsoccluded(const ivec &bborigin, int size)
{
    return pvsoccluded(bborigin, ivec(size, size, size));
}

// rendergl
extern bool hasVBO, hasDRE, hasOQ, hasTR, hasFBO, hasDS, hasTF, hasBE, hasBC, hasCM, hasNP2, hasTC, hasTE, hasMT, hasD3, hasAF, hasVP2, hasVP3, hasPP, hasMDA, hasTE3, hasTE4, hasVP, hasFP, hasGLSL, hasGM, hasNVFB, hasSGIDT, hasSGISH, hasDT, hasSH, hasNVPCF, hasRN, hasPBO, hasFBB;
extern int hasstencil;

extern bool envmapping, renderedgame;
extern glmatrixf mvmatrix, projmatrix, mvpmatrix, invmvmatrix, invmvpmatrix;
extern bvec fogcolor;

extern void gl_checkextensions();
extern void gl_init(int w, int h, int bpp, int depth, int fsaa);
extern void cleangl();
extern void rendergame(bool mainpass = false);
extern void invalidatepostfx();
extern void gl_drawframe(int w, int h);
extern void gl_drawmainmenu(int w, int h);
extern void enablepolygonoffset(GLenum type);
extern void disablepolygonoffset(GLenum type);
extern void calcspherescissor(const vec &center, float size, float &sx1, float &sy1, float &sx2, float &sy2);
extern int pushscissor(float sx1, float sy1, float sx2, float sy2);
extern void popscissor();
extern void setfogplane(const plane &p, bool flush = false);
extern void setfogplane(float scale = 0, float z = 0, bool flush = false, float fadescale = 0, float fadeoffset = 0);
extern void recomputecamera();
extern void findorientation();
extern void writecrosshairs(stream *f);

// renderextras
extern void render3dbox(vec &o, float tofloor, float toceil, float xradius, float yradius = 0);

// octa
extern cube *newcubes(uint face = F_EMPTY);
extern cubeext *newcubeext(cube &c);
extern void getcubevector(cube &c, int d, int x, int y, int z, ivec &p);
extern void setcubevector(cube &c, int d, int x, int y, int z, ivec &p);
extern int familysize(cube &c);
extern void freeocta(cube *c);
extern void discardchildren(cube &c);
extern void optiface(uchar *p, cube &c);
extern void validatec(cube *c, int size);
extern bool isvalidcube(cube &c);
extern cube &lookupcube(int tx, int ty, int tz, int tsize = 0);
extern cube &neighbourcube(int x, int y, int z, int size, int rsize, int orient);
extern void newclipplanes(cube &c);
extern void freeclipplanes(cube &c);
extern void forcemip(cube &c);
extern bool subdividecube(cube &c, bool fullcheck=true, bool brighten=true);
extern void converttovectorworld();
extern int faceverts(cube &c, int orient, int vert);
extern int faceconvexity(cube &c, int orient);
extern void calcvert(cube &c, int x, int y, int z, int size, vvec &vert, int i, bool solid = false);
extern void calcvert(cube &c, int x, int y, int z, int size, vec &vert, int i, bool solid = false);
extern int calcverts(cube &c, int x, int y, int z, int size, vvec *verts, bool *usefaces);
extern uint faceedges(cube &c, int orient);
extern bool collapsedface(uint cfe);
extern bool touchingface(cube &c, int orient);
extern bool flataxisface(cube &c, int orient);
extern int genclipplane(cube &c, int i, vec *v, plane *clip);
extern void genclipplanes(cube &c, int x, int y, int z, int size, clipplanes &p);
extern bool visibleface(cube &c, int orient, int x, int y, int z, int size, uchar mat = MAT_AIR, uchar nmat = MAT_AIR, uchar matmask = MATF_VOLUME);
extern int visibletris(cube &c, int orient, int x, int y, int z, int size);
extern int visibleorient(cube &c, int orient);
extern bool threeplaneintersect(plane &pl1, plane &pl2, plane &pl3, vec &dest);
extern void genvectorvert(const ivec &p, cube &c, ivec &v);
extern void freemergeinfo(cube &c);
extern void genmergedverts(cube &cu, int orient, const ivec &co, int size, const mergeinfo &m, vvec *vv, plane *p = NULL);
extern int calcmergedsize(int orient, const ivec &co, int size, const mergeinfo &m, const vvec *vv);
extern void invalidatemerges(cube &c, bool msg);
extern void calcmerges();

struct cubeface : mergeinfo
{
    cube *c;
};

extern int mergefaces(int orient, cubeface *m, int sz);
extern void mincubeface(cube &cu, int orient, const ivec &o, int size, const mergeinfo &orig, mergeinfo &cf);

static inline uchar octantrectangleoverlap(const ivec &c, int size, const ivec &o, const ivec &s)
{
    uchar p = 0xFF; // bitmask of possible collisions with octants. 0 bit = 0 octant, etc
    ivec v(c);
    v.add(size);
    if(v.z <= o.z)     p &= 0xF0; // not in a -ve Z octant
    if(v.z >= o.z+s.z) p &= 0x0F; // not in a +ve Z octant
    if(v.y <= o.y)     p &= 0xCC; // not in a -ve Y octant
    if(v.y >= o.y+s.y) p &= 0x33; // etc..
    if(v.x <= o.x)     p &= 0xAA;
    if(v.x >= o.x+s.x) p &= 0x55;
    return p;
}

static inline bool insideworld(const vec &o)
{
    return o.x>=0 && o.x<worldsize && o.y>=0 && o.y<worldsize && o.z>=0 && o.z<worldsize;
}

static inline bool insideworld(const ivec &o)
{
    return uint(o.x)<uint(worldsize) && uint(o.y)<uint(worldsize) && uint(o.z)<uint(worldsize);
}

// ents
extern char *entname(entity &e);
extern bool haveselent();
extern undoblock *copyundoents(undoblock *u);
extern void pasteundoents(undoblock *u);

// octaedit
extern void cancelsel();
extern void rendertexturepanel(int w, int h);
extern void addundo(undoblock *u);
extern void commitchanges(bool force = false);
extern void rendereditcursor();
extern void tryedit();

// octarender
extern void octarender();
extern void allchanged(bool load = false);
extern void clearvas(cube *c);
extern vtxarray *newva(int x, int y, int z, int size);
extern void destroyva(vtxarray *va, bool reparent = true);
extern bool readva(vtxarray *va, ushort *&edata, uchar *&vdata);
extern void updatevabb(vtxarray *va, bool force = false);
extern void updatevabbs(bool force = false);

// renderva
extern GLuint fogtex;

extern void visiblecubes(float fov, float fovy);
extern void reflectvfcP(float z, float minyaw = -M_PI, float maxyaw = M_PI, float minpitch = -M_PI, float maxpitch = M_PI);
extern void restorevfcP();
extern void createfogtex();
extern void rendergeom(float causticspass = 0, bool fogpass = false);
extern void rendermapmodels();
extern void renderreflectedgeom(bool causticspass = false, bool fogpass = false);
extern void renderreflectedmapmodels();
extern void renderoutline();
extern bool rendersky(bool explicitonly = false);

extern int isvisiblesphere(float rad, const vec &cv);
extern bool bboccluded(const ivec &bo, const ivec &br);
extern occludequery *newquery(void *owner);
extern bool checkquery(occludequery *query, bool nowait = false);
extern void resetqueries();
extern int getnumqueries();
extern void drawbb(const ivec &bo, const ivec &br, const vec &camera = camera1->o, int scale = 0, const ivec &origin = ivec(0, 0, 0));

#define startquery(query) { glBeginQuery_(GL_SAMPLES_PASSED_ARB, ((occludequery *)(query))->id); }
#define endquery(query) \
    { \
        glEndQuery_(GL_SAMPLES_PASSED_ARB); \
        extern int ati_oq_bug; \
        if(ati_oq_bug) glFlush(); \
    }

// dynlight

extern void updatedynlights();
extern int finddynlights();
extern void calcdynlightmask(vtxarray *va);
extern int setdynlights(vtxarray *va, const ivec &vaorigin);
extern bool getdynlight(int n, vec &o, float &radius, vec &color);

// material

extern int showmat;

extern int findmaterial(const char *name);
extern void genmatsurfs(cube &c, int cx, int cy, int cz, int size, vector<materialsurface> &matsurfs, uchar &vismask, uchar &clipmask);
extern void rendermatsurfs(materialsurface *matbuf, int matsurfs);
extern void rendermatgrid(materialsurface *matbuf, int matsurfs);
extern int optimizematsurfs(materialsurface *matbuf, int matsurfs);
extern void setupmaterials(int start = 0, int len = 0);
extern void rendermaterials();
extern void drawmaterial(int orient, int x, int y, int z, int csize, int rsize, float offset);
extern int visiblematerial(cube &c, int orient, int x, int y, int z, int size, uchar matmask = MATF_VOLUME);

// water
extern int refracting;
extern bool reflecting, fading, fogging;
extern float reflectz;
extern int reflectdist, vertwater, refractfog, waterrefract, waterreflect, waterfade, caustics, waterfallrefract, waterfog, lavafog;
extern bvec watercolor, waterfallcolor, lavacolor;

extern void cleanreflections();
extern void queryreflections();
extern void drawreflections();
extern void renderwater();
extern void renderlava(materialsurface &m, Texture *tex, float scale);
extern void loadcaustics(bool force = false);
extern void preloadwatershaders(bool force = false);

// glare
extern bool glaring;

extern void drawglaretex();
extern void addglare();

// depthfx
extern bool depthfxing;

extern void drawdepthfxtex();

// server
extern vector<const char *> gameargs;

extern void initserver(bool listen, bool dedicated);
extern void cleanupserver();
extern void serverslice(bool dedicated, uint timeout);

extern ENetSocket connectmaster();
extern void localclienttoserver(int chan, ENetPacket *, int cn=-1); // INTENSITY: Added cn
extern int localconnect(); // INTENSITY: Added returning of client number
extern bool serveroption(char *opt);

// serverbrowser
extern bool resolverwait(const char *name, ENetAddress *address);
extern int connectwithtimeout(ENetSocket sock, const char *hostname, const ENetAddress &address);
extern void addserver(const char *name, int port = 0, const char *password = NULL, bool keep = false);
extern void writeservercfg();

// client
extern void localdisconnect(bool cleanup = true, int cn=-1); // INTENSITY: Added client number
extern void localservertoclient(int chan, ENetPacket *packet);
extern void connectserv(const char *servername, int port, const char *serverpassword);
extern void abortconnect();
extern void clientkeepalive();

// command
extern hashtable<const char *, ident> *idents;
extern bool overrideidents, persistidents;

extern void explodelist(const char *s, vector<char *> &elems);

extern void clearoverrides();
extern void writecfg();

extern void checksleep(int millis);
extern void clearsleep(bool clearoverrides = true);

// console
extern void keypress(int code, bool isdown, int cooked);
extern int rendercommand(int x, int y, int w);
extern int renderconsole(int w, int h, int abovehud);
extern void conoutf(const char *s, ...);
extern void conoutf(int type, const char *s, ...);
extern void resetcomplete();
extern void complete(char *s);
const char *getkeyname(int code);
extern const char *addreleaseaction(const char *s);
extern void writebinds(stream *f);
extern void writecompletions(stream *f);

// main
enum
{
    NOT_INITING = 0,
    INIT_LOAD,
    INIT_RESET
};
extern int initing;

enum
{
    CHANGE_GFX   = 1<<0,
    CHANGE_SOUND = 1<<1
};
extern bool initwarning(const char *desc, int level = INIT_RESET, int type = CHANGE_GFX);

extern void pushevent(const SDL_Event &e);
extern bool interceptkey(int sym);

extern float loadprogress;
extern void renderbackground(const char *caption = NULL, Texture *mapshot = NULL, const char *mapname = NULL, const char *mapinfo = NULL, bool restore = false, bool force = false);
extern void renderprogress(float bar, const char *text, GLuint tex = 0, bool background = false);

extern void getfps(int &fps, int &bestdiff, int &worstdiff);
extern void swapbuffers();

// menu
extern void menuprocess();
extern void addchange(const char *desc, int type);
extern void clearchanges(int type);

// physics
extern void mousemove(int dx, int dy);
extern bool pointincube(const clipplanes &p, const vec &v);
extern bool overlapsdynent(const vec &o, float radius);
extern void rotatebb(vec &center, vec &radius, int yaw);
extern float shadowray(const vec &o, const vec &ray, float radius, int mode, extentity *t = NULL);

// world
enum
{
    TRIG_COLLIDE    = 1<<0,
    TRIG_TOGGLE     = 1<<1,
    TRIG_ONCE       = 0<<2,
    TRIG_MANY       = 1<<2,
    TRIG_DISAPPEAR  = 1<<3,
    TRIG_AUTO_RESET = 1<<4,
    TRIG_RUMBLE     = 1<<5,
    TRIG_LOCKED     = 1<<6
};

#define NUMTRIGGERTYPES 16

extern int triggertypes[NUMTRIGGERTYPES];

#define checktriggertype(type, flag) (triggertypes[(type) & (NUMTRIGGERTYPES-1)] & (flag))

extern vector<int> outsideents;

extern void entitiesinoctanodes();
extern void attachentities();
extern void freeoctaentities(cube &c);
extern bool pointinsel(selinfo &sel, vec &o);

extern void resetmap();
extern void startmap(const char *name);

// rendermodel
struct mapmodelinfo { string name; model *m; };

extern void findanims(const char *pattern, vector<int> &anims);
extern void loadskin(const char *dir, const char *altdir, Texture *&skin, Texture *&masks);
extern mapmodelinfo &getmminfo(int i);
extern void startmodelquery(occludequery *query);
extern void endmodelquery();
extern void preloadmodelshaders();

// renderparticles
extern void particleinit();
extern void clearparticles();
extern void clearparticleemitters();
extern void seedparticles();
extern void updateparticles();
extern void renderparticles(bool mainpass = false);
extern bool printparticles(extentity &e, char *buf);

// decal
extern void initdecals();
extern void cleardecals();
extern void renderdecals(bool mainpass = false);

// blob

enum
{
    BLOB_STATIC = 0,
    BLOB_DYNAMIC
};

extern int showblobs;

extern void initblobs(int type = -1);
extern void resetblobs();
extern void renderblob(int type, const vec &o, float radius, float fade = 1);
extern void flushblobs();

// rendersky
extern int explicitsky;
extern double skyarea;

extern void drawskybox(int farplane, bool limited);
extern bool limitsky();

// 3dgui
extern void g3d_render();
extern bool g3d_windowhit(bool on, bool act);

// menus
extern int mainmenu;

extern void clearmainmenu();
extern void g3d_mainmenu();

// sound
extern void clearmapsounds();
extern void checkmapsounds();
extern void updatesounds();

extern void initmumble();
extern void closemumble();
extern void updatemumble();

// grass
extern void generategrass();
extern void rendergrass();

// blendmap
extern int blendpaintmode;

extern bool setblendmaporigin(const ivec &o, int size);
extern bool hasblendmap();
extern uchar lookupblendmap(const vec &pos);
extern void resetblendmap();
extern void enlargeblendmap();
extern void optimizeblendmap();
extern void stoppaintblendmap();
extern void trypaintblendmap();
extern void renderblendbrush(GLuint tex, float x, float y, float w, float h);
extern void renderblendbrush();
extern bool loadblendmap(stream *f, int info);
extern void saveblendmap(stream *f);
extern uchar shouldsaveblendmap();

// recorder

namespace recorder
{
    void stop();
    void capture();
}

#endif

#endif

