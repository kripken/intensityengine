#include "engine.h"

extern int outline;

void boxs(int orient, vec o, const vec &s)
{
    int   d = dimension(orient),
          dc= dimcoord(orient);

    float f = !outline ? 0 : (dc>0 ? 0.2f : -0.2f);
    o[D[d]] += float(dc) * s[D[d]] + f,

    glBegin(GL_POLYGON);

    glVertex3fv(o.v); o[R[d]] += s[R[d]];
    glVertex3fv(o.v); o[C[d]] += s[C[d]];
    glVertex3fv(o.v); o[R[d]] -= s[R[d]];
    glVertex3fv(o.v);

    glEnd();
    xtraverts += 4;
}

void boxs3D(const vec &o, vec s, int g)
{
    s.mul(g);
    loopi(6)
        boxs(i, o, s);    
}

void boxsgrid(int orient, vec o, vec s, int g)
{
    int   d = dimension(orient),
          dc= dimcoord(orient);
    float ox = o[R[d]],
          oy = o[C[d]],
          xs = s[R[d]],
          ys = s[C[d]],
          f = !outline ? 0 : (dc>0 ? 0.2f : -0.2f);    

    o[D[d]] += dc * s[D[d]]*g + f;

    glBegin(GL_LINES);
    loop(x, xs) {
        o[R[d]] += g;
        glVertex3fv(o.v);
        o[C[d]] += ys*g;
        glVertex3fv(o.v);
        o[C[d]] = oy;
    }
    loop(y, ys) {
        o[C[d]] += g;
        o[R[d]] = ox;
        glVertex3fv(o.v);
        o[R[d]] += xs*g;
        glVertex3fv(o.v);
    }
    glEnd();
    xtraverts += 2*int(xs+ys);
}

selinfo sel, lastsel; // INTENSITY: WAS: selinfo sel = { 0 }, lastsel; // but that fails on gcc 4.4

int orient = 0;
int gridsize = 8;
ivec cor, lastcor;
ivec cur, lastcur;

extern int entediting;
bool editmode = false;
bool havesel = false;
bool hmapsel = false;
int horient  = 0;

extern int entmoving;

VARF(dragging, 0, 0, 1,
    if(!dragging || cor[0]<0) return;
    lastcur = cur;
    lastcor = cor;
    sel.grid = gridsize;
    sel.orient = orient;
);

VARF(moving, 0, 0, 1,
    if(!moving) return;
    vec v(cur.v); v.add(1);
    moving = pointinsel(sel, v);
    if(moving) havesel = false; // tell cursorupdate to create handle
);

VARF(gridpower, 3-VVEC_FRAC, 3, VVEC_INT-1,
{
    if(dragging) return;
    gridsize = 1<<gridpower;
    if(gridsize>=worldsize) gridsize = worldsize/2;
    cancelsel();
});

VAR(passthroughsel, 0, 0, 1);
VAR(editing, 1, 0, 0);
VAR(selectcorners, 0, 0, 1);
VARF(hmapedit, 0, 0, 1, horient = sel.orient);

void forcenextundo() { lastsel.orient = -1; }

void cubecancel()
{
    havesel = false;
    moving = dragging = hmapedit = passthroughsel = 0;
    forcenextundo();
}

extern void entcancel();

void cancelsel()
{
    cubecancel();
    entcancel();
}

void toggleedit(bool force)
{
    if(player->state!=CS_ALIVE && player->state!=CS_DEAD && player->state!=CS_EDITING) return; // do not allow dead players to edit to avoid state confusion
    if(!editmode && !game::allowedittoggle()) return;         // not in most multiplayer modes
    if(!(editmode = !editmode))
    {
        player->state = player->editstate;
        player->o.z -= player->eyeheight;       // entinmap wants feet pos
        entinmap(player);                       // find spawn closest to current floating pos
    }
    else
    {
        game::resetgamestate();
        player->editstate = player->state;
        player->state = CS_EDITING;
    }
    cancelsel();
    stoppaintblendmap();
    keyrepeat(editmode);
    editing = entediting = editmode;
    extern int fullbright;
    if(fullbright) initlights();
    if(!force) game::edittoggled(editmode);
}

bool noedit(bool view, bool msg)
{
    if(!editmode) { if(msg) conoutf(CON_ERROR, "operation only allowed in edit mode"); return true; }
    if(view || haveselent()) return false;
    float r = 1.0f;
    vec o, s;
    o = sel.o.v;
    s = sel.s.v;
    s.mul(float(sel.grid) / 2.0f);
    o.add(s);
    r = float(max(s.x, max(s.y, s.z)));
    bool viewable = (isvisiblesphere(r, o) != VFC_NOT_VISIBLE);
    if(!viewable && msg) conoutf(CON_ERROR, "selection not in view");
    return !viewable;
}

extern void createheightmap();

void reorient()
{
    sel.cx = 0;
    sel.cy = 0;
    sel.cxs = sel.s[R[dimension(orient)]]*2;
    sel.cys = sel.s[C[dimension(orient)]]*2;
    sel.orient = orient;
}

void selextend()
{
    if(noedit(true)) return;    
    loopi(3)
    {
        if(cur[i]<sel.o[i])
        {
            sel.s[i] += (sel.o[i]-cur[i])/sel.grid;
            sel.o[i] = cur[i];
        }
        else if(cur[i]>=sel.o[i]+sel.s[i]*sel.grid)
        {
            sel.s[i] = (cur[i]-sel.o[i])/sel.grid+1;
        }
    }
}

ICOMMAND(edittoggle, "", (), toggleedit(false));
COMMAND(entcancel, "");
COMMAND(cubecancel, "");
COMMAND(cancelsel, "");
COMMAND(reorient, "");
COMMAND(selextend, "");

///////// selection support /////////////

cube &blockcube(int x, int y, int z, const block3 &b, int rgrid) // looks up a world cube, based on coordinates mapped by the block
{
    ivec s(dimension(b.orient), x*b.grid, y*b.grid, dimcoord(b.orient)*(b.s[dimension(b.orient)]-1)*b.grid);

    return neighbourcube(b.o.x+s.x, b.o.y+s.y, b.o.z+s.z, -z*b.grid, rgrid, b.orient);
}

#define loopxy(b)        loop(y,(b).s[C[dimension((b).orient)]]) loop(x,(b).s[R[dimension((b).orient)]])
#define loopxyz(b, r, f) { loop(z,(b).s[D[dimension((b).orient)]]) loopxy((b)) { cube &c = blockcube(x,y,z,b,r); f; } }
#define loopselxyz(f)    { makeundo(); loopxyz(sel, sel.grid, f); changed(sel); }
#define selcube(x, y, z) blockcube(x, y, z, sel, sel.grid)

////////////// cursor ///////////////

int selchildcount=0;

ICOMMAND(havesel, "", (), intret(havesel ? selchildcount : 0));

void countselchild(cube *c, const ivec &cor, int size)
{
    ivec ss(sel.s);
    ss.mul(sel.grid);
    loopoctabox(cor, size, sel.o, ss)
    {
        ivec o(i, cor.x, cor.y, cor.z, size);
        if(c[i].children) countselchild(c[i].children, o, size/2);
        else selchildcount++;
    }
}

void normalizelookupcube(int x, int y, int z)
{
    if(lusize>gridsize)
    {
        lu.x += (x-lu.x)/gridsize*gridsize;
        lu.y += (y-lu.y)/gridsize*gridsize;
        lu.z += (z-lu.z)/gridsize*gridsize;
    }
    else if(gridsize>lusize)
    {
        lu.x &= ~(gridsize-1);
        lu.y &= ~(gridsize-1);
        lu.z &= ~(gridsize-1);
    }
    lusize = gridsize;
}

void updateselection()
{
    sel.o.x = min(lastcur.x, cur.x);
    sel.o.y = min(lastcur.y, cur.y);
    sel.o.z = min(lastcur.z, cur.z);
    sel.s.x = abs(lastcur.x-cur.x)/sel.grid+1;
    sel.s.y = abs(lastcur.y-cur.y)/sel.grid+1;
    sel.s.z = abs(lastcur.z-cur.z)/sel.grid+1;
}

void editmoveplane(const vec &o, const vec &ray, int d, float off, vec &handle, vec &dest, bool first)
{
    plane pl(d, off);
    float dist = 0.0f;

    if(pl.rayintersect(camera1->o, ray, dist)) // INTENSITY: So can drag in thirdperson
    {        
        dest = ray;
        dest.mul(dist);
        dest.add(camera1->o); // INTENSITY: So can drag in thirdperson
        if(first)
        {
            handle = dest;
            handle.sub(o);
        }
        dest.sub(handle);
    }
}

inline bool isheightmap(int orient, int d, bool empty, cube *c);
extern void entdrag(const vec &ray);
extern bool hoveringonent(int ent, int orient);
extern void renderentselection(const vec &o, const vec &ray, bool entmoving);
extern float rayent(const vec &o, const vec &ray, float radius, int mode, int size, int &orient, int &ent);

VAR(gridlookup, 0, 0, 1);
VAR(passthroughcube, 0, 1, 1);

void rendereditcursor() // INTENSITY: Replaced all player->o with camera1->o, so can edit in thirdperson
{
    if(sel.grid == 0) sel.grid = gridsize;

    int d   = dimension(sel.orient),
        od  = dimension(orient),
        odc = dimcoord(orient);

    bool hidecursor = g3d_windowhit(true, false) || blendpaintmode, hovering = false;
    hmapsel = false;
           
    if(moving)
    {       
        ivec e;
        static vec v, handle;
        editmoveplane(sel.o.tovec(), camdir, od, sel.o[D[od]]+odc*sel.grid*sel.s[D[od]], handle, v, !havesel);
        if(!havesel)
        {
            v.add(handle);
            (e = handle).mask(~(sel.grid-1));
            v.sub(handle = e.v);
            havesel = true;
        }
        (e = v).mask(~(sel.grid-1));
        sel.o[R[od]] = e[R[od]];
        sel.o[C[od]] = e[C[od]];
    }
    else 
    if(entmoving)
    {
        entdrag(camdir);       
    }
    else
    {  
        ivec w;
        float sdist = 0, wdist = 0, t;
        int entorient = 0, ent = -1;
       
        wdist = rayent(camera1->o, camdir, 1e16f, 
                       (editmode && showmat ? RAY_EDITMAT : 0)   // select cubes first
                       | (!dragging && entediting ? RAY_ENTS : 0)
                       | RAY_SKIPFIRST 
                       | (passthroughcube==1 ? RAY_PASS : 0), gridsize, entorient, ent);
     
        if((havesel || dragging) && !passthroughsel && !hmapedit)     // now try selecting the selection
            if(rayrectintersect(sel.o.tovec(), vec(sel.s.tovec()).mul(sel.grid), camera1->o, camdir, sdist, orient))
            {   // and choose the nearest of the two
                if(sdist < wdist) 
                {
                    wdist = sdist;
                    ent   = -1;
                }
            }
       
        if((hovering = hoveringonent(hidecursor ? -1 : ent, entorient)))
        {
           if(!havesel) 
           {
               selchildcount = 0;
               sel.s = ivec(0, 0, 0);
           }
        }
        else 
        {
            vec w = vec(camdir).mul(wdist+0.05f).add(camera1->o);
            if(!insideworld(w))
            {
                loopi(3) wdist = min(wdist, ((camdir[i] > 0 ? worldsize : 0) - camera1->o[i]) / camdir[i]);
                w = vec(camdir).mul(wdist-0.05f).add(camera1->o);
                if(!insideworld(w))
                {
                    wdist = 0;
                    loopi(3) w[i] = clamp(camera1->o[i], 0.0f, float(worldsize));
                }
            }
            cube *c = &lookupcube(int(w.x), int(w.y), int(w.z));            
            if(gridlookup && !dragging && !moving && !havesel && hmapedit!=1) gridsize = lusize;
            int mag = lusize / gridsize;
            normalizelookupcube(int(w.x), int(w.y), int(w.z));
            if(sdist == 0 || sdist > wdist) rayrectintersect(lu.tovec(), vec(gridsize), camera1->o, camdir, t=0, orient); // just getting orient     
            cur = lu;
            cor = vec(w).mul(2).div(gridsize);
            od = dimension(orient);
            d = dimension(sel.orient);
            
            if(hmapedit==1 && dimcoord(horient) == (camdir[dimension(horient)]<0))
            {
                hmapsel = isheightmap(horient, dimension(horient), false, c);     
                if(hmapsel)
                    od = dimension(orient = horient);
            }

            if(dragging) 
            {
                updateselection();
                sel.cx   = min(cor[R[d]], lastcor[R[d]]);
                sel.cy   = min(cor[C[d]], lastcor[C[d]]);
                sel.cxs  = max(cor[R[d]], lastcor[R[d]]);
                sel.cys  = max(cor[C[d]], lastcor[C[d]]);

                if(!selectcorners)
                {
                    sel.cx &= ~1;
                    sel.cy &= ~1;
                    sel.cxs &= ~1;
                    sel.cys &= ~1;
                    sel.cxs -= sel.cx-2;
                    sel.cys -= sel.cy-2;
                }
                else
                {
                    sel.cxs -= sel.cx-1;
                    sel.cys -= sel.cy-1;
                }

                sel.cx  &= 1;
                sel.cy  &= 1;
                havesel = true;
            }
            else if(!havesel)
            {
                sel.o = lu;
                sel.s.x = sel.s.y = sel.s.z = 1;
                sel.cx = sel.cy = 0;
                sel.cxs = sel.cys = 2;
                sel.grid = gridsize;
                sel.orient = orient;
                d = od;
            }

            sel.corner = (cor[R[d]]-(lu[R[d]]*2)/gridsize)+(cor[C[d]]-(lu[C[d]]*2)/gridsize)*2;
            selchildcount = 0;
            countselchild(worldroot, ivec(0, 0, 0), worldsize/2);
            if(mag>1 && selchildcount==1) selchildcount = -mag;
        }
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    
    // cursors    

    renderentselection(camera1->o, camdir, entmoving!=0);

    enablepolygonoffset(GL_POLYGON_OFFSET_LINE);

    if(!moving && !hovering && !hidecursor)
    {
        if(hmapedit==1)
            glColor3ub(0, hmapsel ? 255 : 40, 0);
        else
            glColor3ub(120,120,120);
        boxs(orient, lu.tovec(), vec(lusize));
    }

    // selections
    if(havesel)
    {
        d = dimension(sel.orient);
        glColor3ub(50,50,50);   // grid
        boxsgrid(sel.orient, sel.o.tovec(), sel.s.tovec(), sel.grid);
        glColor3ub(200,0,0);    // 0 reference
        boxs3D(sel.o.tovec().sub(0.5f*min(gridsize*0.25f, 2.0f)), vec(min(gridsize*0.25f, 2.0f)), 1);
        glColor3ub(200,200,200);// 2D selection box
        vec co(sel.o.v), cs(sel.s.v);
        co[R[d]] += 0.5f*(sel.cx*gridsize);
        co[C[d]] += 0.5f*(sel.cy*gridsize);
        cs[R[d]]  = 0.5f*(sel.cxs*gridsize);
        cs[C[d]]  = 0.5f*(sel.cys*gridsize);       
        cs[D[d]] *= gridsize;
        boxs(sel.orient, co, cs);
        if(hmapedit==1)         // 3D selection box
            glColor3ub(0,120,0);
        else 
            glColor3ub(0,0,120);
        boxs3D(sel.o.tovec(), sel.s.tovec(), sel.grid);
    }
   
    disablepolygonoffset(GL_POLYGON_OFFSET_LINE);

    glDisable(GL_BLEND);
}

void tryedit()
{
    extern int hidehud;
    if(!editmode || hidehud || mainmenu) return;
    if(blendpaintmode) trypaintblendmap();
}

//////////// ready changes to vertex arrays ////////////

static bool haschanged = false;

void readychanges(block3 &b, cube *c, const ivec &cor, int size)
{
    loopoctabox(cor, size, b.o, b.s)
    {
        ivec o(i, cor.x, cor.y, cor.z, size);
        if(c[i].ext)
        {
            if(c[i].ext->va)             // removes va s so that octarender will recreate
            {
                int hasmerges = c[i].ext->va->hasmerges;
                destroyva(c[i].ext->va);
                c[i].ext->va = NULL;
                if(hasmerges) invalidatemerges(c[i], true); 
            }
            freeoctaentities(c[i]);
            c[i].ext->tjoints = -1;
        }
        if(c[i].children)
        {
            if(size<=(8>>VVEC_FRAC))
            {
                solidfaces(c[i]);
                discardchildren(c[i]);
                brightencube(c[i]);
            }
            else readychanges(b, c[i].children, o, size/2);
        }
        else brightencube(c[i]);
    }
}

void commitchanges(bool force)
{
    if(!force && !haschanged) return;
    haschanged = false;

    extern vector<vtxarray *> valist;
    int oldlen = valist.length();
    entitiesinoctanodes();
    inbetweenframes = false;
    octarender();
    inbetweenframes = true;
    setupmaterials(oldlen);
    invalidatepostfx();
    updatevabbs();
    resetblobs();
}

void changed(const block3 &sel, bool commit = true)
{
    if(sel.s.iszero()) return;
    block3 b = sel;
    loopi(3) b.s[i] *= b.grid;
    b.grid = 1;
    loopi(3)                    // the changed blocks are the selected cubes
    {
        b.o[i] -= 1;
        b.s[i] += 2;
        readychanges(b, worldroot, ivec(0, 0, 0), worldsize/2);
        b.o[i] += 1;
        b.s[i] -= 2;
    }
    haschanged = true;

    if(commit) commitchanges();
}

//////////// copy and undo /////////////
cube copycube(cube &src)
{
    cube c = src;
    c.ext = NULL; // src cube is responsible for va destruction
    if(src.children)
    {
        c.children = newcubes(F_EMPTY);
        loopi(8) c.children[i] = copycube(src.children[i]);
    }
    else if(src.ext && src.ext->material!=MAT_AIR) ext(c).material = src.ext->material;
    return c;
}

void pastecube(cube &src, cube &dest)
{
    discardchildren(dest);
    dest = copycube(src);
}

void blockcopy(const block3 &s, int rgrid, block3 *b)
{
    *b = s;
    cube *q = b->c();
    loopxyz(s, rgrid, *q++ = copycube(c));
}

block3 *blockcopy(const block3 &s, int rgrid)
{
    block3 *b = (block3 *)new uchar[sizeof(block3)+sizeof(cube)*s.size()];
    blockcopy(s, rgrid, b);
    return b;
}

void freeblock(block3 *b, bool alloced = true)
{
    cube *q = b->c();
    loopi(b->size()) discardchildren(*q++);
    if(alloced) delete[] b;
}

void selgridmap(selinfo &sel, int *g)                           // generates a map of the cube sizes at each grid point
{
    loopxyz(sel, -sel.grid, (*g++ = lusize, (void)c));
}

void freeundo(undoblock *u)
{
    if(!u->numents) freeblock(u->block(), false);
    delete[] (uchar *)u;
}

void pasteundo(undoblock *u)
{
    if(u->numents) pasteundoents(u);
    else
    {
        block3 *b = u->block();
        cube *s = b->c();
        int *g = u->gridmap();
        loopxyz(*b, *g++, pastecube(*s++, c));
    }
}

static inline int undosize(undoblock *u)
{
    if(u->numents) return u->numents*sizeof(undoent);
    else
    {
        block3 *b = u->block();
        cube *q = b->c();
        int size = b->size(), total = size*sizeof(int);
        loopj(size) total += familysize(*q++)*sizeof(cube);
        return total;
    }
}

struct undolist
{
    undoblock *first, *last;

    undolist() : first(NULL), last(NULL) {}

    bool empty() { return !first; }

    void add(undoblock *u)
    {
        u->next = NULL;
        u->prev = last;
        if(!first) first = last = u;
        else
        {
            last->next = u;
            last = u;
        }
    }

    undoblock *popfirst()
    {
        undoblock *u = first;
        first = first->next;
        if(first) first->prev = NULL;
        else last = NULL;
        return u;
    }

    undoblock *poplast()
    {
        undoblock *u = last;
        last = last->prev;
        if(last) last->next = NULL;
        else first = NULL;
        return u;
    }
};

undolist undos, redos;
VARP(undomegs, 0, 5, 100);                              // bounded by n megs
int totalundos = 0;

void pruneundos(int maxremain)                          // bound memory
{
    while(totalundos > maxremain && !undos.empty())
    {
        undoblock *u = undos.popfirst();
        totalundos -= u->size;
        freeundo(u);
    }
    //conoutf(CON_DEBUG, "undo: %d of %d(%%%d)", totalundos, undomegs<<20, totalundos*100/(undomegs<<20));
    while(!redos.empty()) 
    {
        undoblock *u = redos.popfirst();
        totalundos -= u->size;
        freeundo(u);
    }
}

void clearundos() { pruneundos(0); }

COMMAND(clearundos, "");

undoblock *newundocube(selinfo &s)
{
    int ssize = s.size(),
        selgridsize = ssize*sizeof(int),
        blocksize = sizeof(block3)+ssize*sizeof(cube);
    undoblock *u = (undoblock *)new uchar[sizeof(undoblock) + blocksize + selgridsize];
    u->numents = 0;
    block3 *b = (block3 *)(u + 1);
    blockcopy(s, -s.grid, b);
    int *g = (int *)((uchar *)b + blocksize);
    selgridmap(s, g);
    return u;
}

void addundo(undoblock *u)
{
    u->size = undosize(u);
    u->timestamp = totalmillis;
    undos.add(u);
    totalundos += u->size;
    pruneundos(undomegs<<20);
}

VARP(nompedit, 0, 1, 1);

void makeundoex(selinfo &s)
{
    if(nompedit && multiplayer(false)) return;
    undoblock *u = newundocube(s);
    addundo(u);
}

void makeundo()                        // stores state of selected cubes before editing
{
    if(lastsel==sel || sel.s.iszero()) return;
    lastsel=sel;
    makeundoex(sel);
}

void swapundo(undolist &a, undolist &b, const char *s)
{
    if(noedit() || (nompedit && multiplayer())) return;
    if(a.empty()) { conoutf(CON_WARN, "nothing more to %s", s); return; }	
	int ts = a.last->timestamp;
    selinfo l = sel;
	while(!a.empty() && ts==a.last->timestamp)
	{
		undoblock *u = a.poplast(), *r;
        if(u->numents) r = copyundoents(u);
		else
		{
            block3 *ub = u->block();
			l.o = ub->o;
			l.s = ub->s;
			l.grid = ub->grid;
			l.orient = ub->orient;
            r = newundocube(l);
		}
        r->size = u->size;
        r->timestamp = totalmillis;
        b.add(r);
		pasteundo(u);
		if(!u->numents) changed(l, false);
		freeundo(u);
	}
    commitchanges();
    if(!hmapsel) 
    {
        sel = l;
        reorient();
    }
    forcenextundo();
}

void editundo() { swapundo(undos, redos, "undo"); }
void editredo() { swapundo(redos, undos, "redo"); }

editinfo *localedit = NULL;

void freeeditinfo(editinfo *&e)
{
    if(!e) return;
    if(e->copy) freeblock(e->copy);
    delete e;
    e = NULL;
}

// guard against subdivision
#define protectsel(f) { undoblock *_u = newundocube(sel); f; pasteundo(_u); freeundo(_u); }

void mpcopy(editinfo *&e, selinfo &sel, bool local)
{
    if(local) game::edittrigger(sel, EDIT_COPY);
    if(e==NULL) e = new editinfo;
    if(e->copy) freeblock(e->copy);
    e->copy = NULL;
    protectsel(e->copy = blockcopy(block3(sel), sel.grid));
    changed(sel);
}

void mppaste(editinfo *&e, selinfo &sel, bool local)
{
    if(e==NULL) return;
    if(local) game::edittrigger(sel, EDIT_PASTE);
    if(e->copy)
    {
        sel.s = e->copy->s;
        int o = sel.orient;
        sel.orient = e->copy->orient;
        cube *s = e->copy->c();
        loopselxyz(if (!isempty(*s) || s->children) pastecube(*s, c); s++); // 'transparent'. old opaque by 'delcube; paste'
        sel.orient = o;
    }
}

void copy()
{
    if(noedit(true)) return;
    mpcopy(localedit, sel, true);
}

void pastehilite()
{
    if(!localedit) return;
	sel.s = localedit->copy->s;
    reorient();
    havesel = true;
}

void paste()
{
    if(noedit()) return;
    mppaste(localedit, sel, true);
}

COMMAND(copy, "");
COMMAND(pastehilite, "");
COMMAND(paste, "");
COMMANDN(undo, editundo, "");
COMMANDN(redo, editredo, "");

///////////// height maps ////////////////

#define MAXBRUSH    64
#define MAXBRUSHC   63
#define MAXBRUSH2   32
int brush[MAXBRUSH][MAXBRUSH];
VAR(brushx, 0, MAXBRUSH2, MAXBRUSH);
VAR(brushy, 0, MAXBRUSH2, MAXBRUSH);
bool paintbrush = 0;
int brushmaxx = 0, brushminx = MAXBRUSH;
int brushmaxy = 0, brushminy = MAXBRUSH;

void clearbrush()
{
    memset(brush, 0, sizeof brush);
    brushmaxx = brushmaxy = 0;
    brushminx = brushminy = MAXBRUSH;
    paintbrush = false;
}

void brushvert(int *x, int *y, int *v)
{
    *x += MAXBRUSH2 - brushx + 1; // +1 for automatic padding
    *y += MAXBRUSH2 - brushy + 1;
    if(*x<0 || *y<0 || *x>=MAXBRUSH || *y>=MAXBRUSH) return;
    brush[*x][*y] = clamp(*v, 0, 8);
    paintbrush = paintbrush || (brush[*x][*y] > 0);
    brushmaxx = min(MAXBRUSH-1, max(brushmaxx, *x+1));
    brushmaxy = min(MAXBRUSH-1, max(brushmaxy, *y+1));
    brushminx = max(0,          min(brushminx, *x-1));
    brushminy = max(0,          min(brushminy, *y-1));
}

vector<int> htextures;

COMMAND(clearbrush, "");
COMMAND(brushvert, "iii");
ICOMMAND(hmapcancel, "", (), htextures.setsizenodelete(0); );
ICOMMAND(hmapselect, "", (), 
    int t = lookupcube(cur.x, cur.y, cur.z).texture[orient];
    int i = htextures.find(t);
    if(i<0)
        htextures.add(t);
    else
        htextures.remove(i);
);

inline bool ishtexture(int t)
{    
    loopv(htextures)
        if(t == htextures[i])
            return false;
    return true;
}

VARP(bypassheightmapcheck, 0, 0, 1);    // temp

inline bool isheightmap(int o, int d, bool empty, cube *c) 
{
    return havesel ||
           (empty && isempty(*c)) ||
           ishtexture(c->texture[o]);
}

namespace hmap 
{
#   define PAINTED     1
#   define NOTHMAP     2
#   define MAPPED      16
    uchar  flags[MAXBRUSH][MAXBRUSH];
    cube   *cmap[MAXBRUSHC][MAXBRUSHC][4];
    int    mapz[MAXBRUSHC][MAXBRUSHC];    
    int    map [MAXBRUSH][MAXBRUSH];
    
    selinfo changes;    
    bool selecting;
    int d, dc, dr, dcr, biasup, br, hws, fg;
    int gx, gy, gz, mx, my, mz, nx, ny, nz, bmx, bmy, bnx, bny;
    uint fs;
    selinfo hundo;
    
    cube *getcube(ivec t, int f) 
    {
        t[d] += dcr*f*gridsize;    
        if(t[d] > nz || t[d] < mz) return NULL;
        cube *c = &lookupcube(t.x, t.y, t.z, gridsize);
        if(c->children) forcemip(*c);
        discardchildren(*c);    
        if(!isheightmap(sel.orient, d, true, c)) return NULL;        
        if     (t.x < changes.o.x) changes.o.x = t.x;
        else if(t.x > changes.s.x) changes.s.x = t.x;
        if     (t.y < changes.o.y) changes.o.y = t.y;
        else if(t.y > changes.s.y) changes.s.y = t.y;
        if     (t.z < changes.o.z) changes.o.z = t.z;
        else if(t.z > changes.s.z) changes.s.z = t.z;
        return c;
    }

    uint getface(cube *c, int d)
    {
        return  0x0f0f0f0f & ((dc ? c->faces[d] : 0x88888888 - c->faces[d]) >> fs);
    }

    void pushside(cube &c, int d, int x, int y, int z)
    {
        ivec a;
        getcubevector(c, d, x, y, z, a);
        a[R[d]] = 8 - a[R[d]];
        setcubevector(c, d, x, y, z, a);
    }

    void addpoint(int x, int y, int z, int v)
    {
        if(!(flags[x][y] & MAPPED))
          map[x][y] = v + (z*8);
      flags[x][y] |= MAPPED;
    }
    
    void select(int x, int y, int z)
    {        
        if((NOTHMAP & flags[x][y]) || (PAINTED & flags[x][y])) return;
        ivec t(d, x+gx, y+gy, dc ? z : hws-z);
        t.shl(gridpower);

        // selections may damage; must makeundo before
        hundo.o = t;
        hundo.o[D[d]] -= dcr*gridsize*2;
        makeundoex(hundo);
        
        cube **c = cmap[x][y];     
        loopk(4) c[k] = NULL;
        c[1] = getcube(t, 0);
        if(!c[1] || !isempty(*c[1])) 
        {   // try up             
            c[2] = c[1];
            c[1] = getcube(t, 1);
            if(!c[1] || isempty(*c[1])) {
                c[0] = c[1], c[1] = c[2], c[2] = NULL;
            }else
                z++, t[d]+=fg;
        }
        else // drop down
        { 
            z--; 
            t[d]-= fg; 
            c[0] = c[1];
            c[1] = getcube(t, 0);            
        }
        
        if(!c[1] || isempty(*c[1])) { flags[x][y] |= NOTHMAP; return; }

        flags[x][y] |= PAINTED;
        mapz [x][y]  = z;
        
        if(!c[0]) c[0] = getcube(t, 1);
        if(!c[2]) c[2] = getcube(t, -1);
        c[3] = getcube(t, -2);
        c[2] = !c[2] || isempty(*c[2]) ? NULL : c[2];
        c[3] = !c[3] || isempty(*c[3]) ? NULL : c[3];
        
        uint face = getface(c[1], d);          
        if(face == 0x08080808 && (!c[0] || !isempty(*c[0]))) { flags[x][y] |= NOTHMAP; return; }              
        if(c[1]->faces[R[d]] == F_SOLID)   // was single
            face += 0x08080808;      
        else                               // was pair
            face += c[2] ? getface(c[2], d) : 0x08080808;
        face += 0x08080808;                // c[3]        
        uchar *f = (uchar*)&face;
        addpoint(x,   y,   z, f[0]);
        addpoint(x+1, y,   z, f[1]);
        addpoint(x,   y+1, z, f[2]);
        addpoint(x+1, y+1, z, f[3]);
                
        if(selecting) // continue to adjacent cubes
        {        
            if(x>bmx) select(x-1, y, z);
            if(x<bnx) select(x+1, y, z);
            if(y>bmy) select(x, y-1, z);
            if(y<bny) select(x, y+1, z);
        }
    }       

    void ripple(int x, int y, int z, bool force)
    {
        if(force) select(x, y, z);
        if((NOTHMAP & flags[x][y]) || !(PAINTED & flags[x][y])) return;

        bool changed = false;
        int *o[4], best, par, q = 0;
        loopi(2) loopj(2) o[i+j*2] = &map[x+i][y+j];
        #define pullhmap(I, LT, GT, M, N, A) do { \
            best = I; \
            loopi(4) if(*o[i] LT best) best = *o[q = i] - M; \
            par = (best&(~7)) + N; \
            /* dual layer for extra smoothness */ \
            if(*o[q^3] GT par && !(*o[q^1] LT par || *o[q^2] LT par)) { \
                if(*o[q^3] GT par A 8 || *o[q^1] != par || *o[q^2] != par) { \
                    *o[q^3] = (*o[q^3] GT par A 8 ? par A 8 : *o[q^3]); \
                    *o[q^1] = *o[q^2] = par; \
                    changed = true; \
                } \
            /* single layer */ \
            } else { \
                loopj(4) if(*o[j] GT par) { \
                    *o[j] = par; \
                    changed = true; \
                } \
            } \
        } while(0)
        
        if(biasup)
            pullhmap(0, >, <, 1, 0, -);
        else
            pullhmap(worldsize, <, >, 0, 8, +);     
   
        cube **c  = cmap[x][y];
        int e[2][2];
        int notempty = 0;

        loopk(4) if(c[k]) {
            loopi(2) loopj(2) {
                e[i][j] = min(8, map[x+i][y+j] - (mapz[x][y]+3-k)*8);
                notempty |= e[i][j] > 0;         
            }
            if(notempty) 
            {
                c[k]->texture[sel.orient] = c[1]->texture[sel.orient];
                solidfaces(*c[k]);
                loopi(2) loopj(2)
                {
                    int f = e[i][j];
                    if(f<0 || (f==0 && e[1-i][j]==0 && e[i][1-j]==0))
                    {
                        f=0;
                        pushside(*c[k], d, i, j, 0);
                        pushside(*c[k], d, i, j, 1);
                    }
                    edgeset(cubeedge(*c[k], d, i, j), dc, dc ? f : 8-f);
                }
            }
            else 
                emptyfaces(*c[k]);
        }

        if(!changed) return;
        if(x>mx) ripple(x-1, y, mapz[x][y], true);
        if(x<nx) ripple(x+1, y, mapz[x][y], true);
        if(y>my) ripple(x, y-1, mapz[x][y], true);
        if(y<ny) ripple(x, y+1, mapz[x][y], true);    
               
#define DIAGONAL_RIPPLE(a,b,exp) if(exp) { \
            if(flags[x a][ y] & PAINTED) \
                ripple(x a, y b, mapz[x a][y], true); \
            else if(flags[x][y b] & PAINTED) \
                ripple(x a, y b, mapz[x][y b], true); \
        }
        
        DIAGONAL_RIPPLE(-1, -1, (x>mx && y>my)); // do diagonals because adjacents
        DIAGONAL_RIPPLE(-1, +1, (x>mx && y<ny)); //    won't unless changed
        DIAGONAL_RIPPLE(+1, +1, (x<nx && y<ny));
        DIAGONAL_RIPPLE(+1, -1, (x<nx && y>my));
    }

#define loopbrush(i) for(int x=bmx; x<=bnx+i; x++) for(int y=bmy; y<=bny+i; y++)

    void paint()
    {
        loopbrush(1)
            map[x][y] -= dr * brush[x][y];        
    }

    void smooth()
    {
        int sum, div;
        loopbrush(-2)
        {
            sum = 0;
            div = 9;
            loopi(3) loopj(3)
                if(flags[x+i][y+j] & MAPPED)
                    sum += map[x+i][y+j];
                else div--;
            if(div)
                map[x+1][y+1] = sum / div;
        }
    }

    void rippleandset()
    {              
        loopbrush(0)
            ripple(x, y, gz, false);        
    }

    void run(int dir, int mode) 
    {                 
        d  = dimension(sel.orient);
        dc = dimcoord(sel.orient);
        dcr= dc ? 1 : -1;
        dr = dir>0 ? 1 : -1;
        br = dir>0 ? 0x08080808 : 0;
     //   biasup = mode == dir<0;
        biasup = dir<0;
        bool paintme = paintbrush;
        int cx = (sel.corner&1 ? 0 : -1);
        int cy = (sel.corner&2 ? 0 : -1);
        hws= (worldsize>>gridpower);
        gx = (cur[R[d]] >> gridpower) + cx - MAXBRUSH2;
        gy = (cur[C[d]] >> gridpower) + cy - MAXBRUSH2;
        gz = (cur[D[d]] >> gridpower);
        fs = dc ? 4 : 0;  
        fg = dc ? gridsize : -gridsize;
        mx = max(0, -gx); // ripple range
        my = max(0, -gy);
        nx = min(MAXBRUSH-1, hws-gx) - 1; 
        ny = min(MAXBRUSH-1, hws-gy) - 1; 
        if(havesel)
        {   // selection range
            bmx = mx = max(mx, (sel.o[R[d]]>>gridpower)-gx);
            bmy = my = max(my, (sel.o[C[d]]>>gridpower)-gy);
            bnx = nx = min(nx, (sel.s[R[d]]+(sel.o[R[d]]>>gridpower))-gx-1);
            bny = ny = min(ny, (sel.s[C[d]]+(sel.o[C[d]]>>gridpower))-gy-1);
        }
        if(havesel && mode<0) // -ve means smooth selection
            paintme = false;
        else
        {   // brush range
            bmx = max(mx, brushminx);
            bmy = max(my, brushminy);
            bnx = min(nx, brushmaxx-1);
            bny = min(ny, brushmaxy-1);   
        }
        nz = worldsize-gridsize;
        mz = 0;
        hundo.s = ivec(d,1,1,5);
        hundo.orient = sel.orient;
        hundo.grid = gridsize;
        forcenextundo();
                    
        changes.grid = gridsize;
        changes.s = changes.o = cur;
        memset(map, 0, sizeof map);
        memset(flags, 0, sizeof flags);
        
        selecting = true;
        select(clamp(MAXBRUSH2-cx, bmx, bnx),
               clamp(MAXBRUSH2-cy, bmy, bny),
               dc ? gz : hws - gz);
        selecting = false;
        if(paintme)
            paint();
        else 
            smooth();
        rippleandset();                       // pull up points to cubify, and set
        changes.s.sub(changes.o).shr(gridpower).add(1);
        changed(changes);
    }
}

void edithmap(int dir, int mode) {    
    if((nompedit && multiplayer()) || !hmapsel || gridsize < 8) return;    
    hmap::run(dir, mode);        
}

///////////// main cube edit ////////////////

int bounded(int n) { return n<0 ? 0 : (n>8 ? 8 : n); }

void pushedge(uchar &edge, int dir, int dc)
{
    int ne = bounded(edgeget(edge, dc)+dir);
    edge = edgeset(edge, dc, ne);
    int oe = edgeget(edge, 1-dc);
    if((dir<0 && dc && oe>ne) || (dir>0 && dc==0 && oe<ne)) edge = edgeset(edge, 1-dc, ne);
}

void linkedpush(cube &c, int d, int x, int y, int dc, int dir)
{
    ivec v, p;
    getcubevector(c, d, x, y, dc, v);

    loopi(2) loopj(2)
    {
        getcubevector(c, d, i, j, dc, p);
        if(v==p)
            pushedge(cubeedge(c, d, i, j), dir, dc);
    }
}

static uchar getmaterial(cube &c)
{
    if(c.children)
    {
        uchar mat = getmaterial(c.children[7]);
        loopi(7) if(mat != getmaterial(c.children[i])) return MAT_AIR;
        return mat;
    }
    return c.ext ? c.ext->material : MAT_AIR;
}

VAR(invalidcubeguard, 0, 1, 1);

void mpeditface(int dir, int mode, selinfo &sel, bool local)
{
    if(mode==1 && (sel.cx || sel.cy || sel.cxs&1 || sel.cys&1)) mode = 0;
    int d = dimension(sel.orient);
    int dc = dimcoord(sel.orient);
    int seldir = dc ? -dir : dir;

    if(local)
        game::edittrigger(sel, EDIT_FACE, dir, mode);

    if(mode==1)
    {
        int h = sel.o[d]+dc*sel.grid;
        if(((dir>0) == dc && h<=0) || ((dir<0) == dc && h>=worldsize)) return;
        if(dir<0) sel.o[d] += sel.grid * seldir;
    }

    if(dc) sel.o[d] += sel.us(d)-sel.grid;
    sel.s[d] = 1;

    loopselxyz(
        if(c.children) solidfaces(c);
        uchar mat = getmaterial(c);
        discardchildren(c);
        if(mat!=MAT_AIR) ext(c).material = mat;
        if(mode==1) // fill command
        {
            if(dir<0)
            {
                solidfaces(c);
                cube &o = blockcube(x, y, 1, sel, -sel.grid);
                loopi(6)
                    c.texture[i] = o.texture[i];
            }
            else
                emptyfaces(c);
        }
        else
        {
            uint bak = c.faces[d];
            uchar *p = (uchar *)&c.faces[d];

            if(mode==2)
                linkedpush(c, d, sel.corner&1, sel.corner>>1, dc, seldir); // corner command
            else
            {
                loop(mx,2) loop(my,2)                                       // pull/push edges command
                {
                    if(x==0 && mx==0 && sel.cx) continue;
                    if(y==0 && my==0 && sel.cy) continue;
                    if(x==sel.s[R[d]]-1 && mx==1 && (sel.cx+sel.cxs)&1) continue;
                    if(y==sel.s[C[d]]-1 && my==1 && (sel.cy+sel.cys)&1) continue;
                    if(p[mx+my*2] != ((uchar *)&bak)[mx+my*2]) continue;

                    linkedpush(c, d, mx, my, dc, seldir);
                }
            }

            optiface(p, c);
            if(invalidcubeguard==1 && !isvalidcube(c))
            {
                uint newbak = c.faces[d];
                uchar *m = (uchar *)&bak;
                uchar *n = (uchar *)&newbak;
                loopk(4) if(n[k] != m[k]) // tries to find partial edit that is valid
                {
                    c.faces[d] = bak;
                    c.edges[d*4+k] = n[k];
                    if(isvalidcube(c))
                        m[k] = n[k];
                }
                c.faces[d] = bak;
            }
        }
    );
    if (mode==1 && dir>0)
        sel.o[d] += sel.grid * seldir;
}

void editface(int *dir, int *mode)
{
    if(noedit(moving!=0)) return;
    if(hmapedit!=1)
        mpeditface(*dir, *mode, sel, true);        
    else 
        edithmap(*dir, *mode);       
}

VAR(selectionsurf, 0, 0, 1);

void pushsel(int *dir)
{
    if(noedit(moving!=0)) return;
    int d = dimension(orient);
    int s = dimcoord(orient) ? -*dir : *dir;
    sel.o[d] += s*sel.grid;
    if(selectionsurf==1) player->o[d] += s*sel.grid;
}

void mpdelcube(selinfo &sel, bool local)
{
    if(local) game::edittrigger(sel, EDIT_DELCUBE);
    loopselxyz(discardchildren(c); emptyfaces(c));
}

void delcube() 
{
    if(noedit()) return;
    mpdelcube(sel, true);
}

COMMAND(pushsel, "i");
COMMAND(editface, "ii");
COMMAND(delcube, "");

/////////// texture editing //////////////////

int curtexindex = -1, lasttex = 0, lasttexmillis = -1;
int texpaneltimer = 0;
vector<ushort> texmru;

void tofronttex()                                       // maintain most recently used of the texture lists when applying texture
{
    int c = curtexindex;
    if(c>=0)
    {
        texmru.insert(0, texmru.remove(c));
        curtexindex = -1;
    }
}

selinfo repsel;
int reptex = -1;

void edittexcube(cube &c, int tex, int orient, bool &findrep)
{
    if(orient<0) loopi(6) c.texture[i] = tex;
    else
    {
        int i = visibleorient(c, orient);
        if(findrep)
        {
            if(reptex < 0) reptex = c.texture[i];
            else if(reptex != c.texture[i]) findrep = false;
        }
        c.texture[i] = tex;
    }
    if(c.children) loopi(8) edittexcube(c.children[i], tex, orient, findrep);
}

extern int curtexnum;
VAR(allfaces, 0, 0, 1);

void mpedittex(int tex, int allfaces, selinfo &sel, bool local)
{
    if(local)
    {
        game::edittrigger(sel, EDIT_TEX, tex, allfaces);
        if(allfaces || !(repsel == sel)) reptex = -1;
        repsel = sel;
    }
    bool findrep = local && !allfaces && reptex < 0;
    loopselxyz(edittexcube(c, tex, allfaces ? -1 : sel.orient, findrep));
}

void filltexlist()
{
    if(texmru.length()!=curtexnum)
    {
        loopv(texmru) if(texmru[i]>=curtexnum) texmru.remove(i--);
        loopi(curtexnum) if(texmru.find(i)<0) texmru.add(i);
    }
}

void edittex(int i, bool save = true)
{
    lasttex = i;
    lasttexmillis = totalmillis;
    if(save) 
    {
        loopvj(texmru) if(texmru[j]==lasttex) { curtexindex = j; break; }
    }
    mpedittex(i, allfaces, sel, true);
}

void edittex_(int *dir)
{
    if(noedit()) return;
    filltexlist();
    texpaneltimer = 5000;
    if(!(lastsel==sel)) tofronttex();
    curtexindex = clamp(curtexindex<0 ? 0 : curtexindex+*dir, 0, curtexnum-1);
    edittex(texmru[curtexindex], false);
}

void gettex()
{
    if(noedit()) return;
    filltexlist();
    loopxyz(sel, sel.grid, curtexindex = c.texture[sel.orient]);
    loopi(curtexnum) if(texmru[i]==curtexindex)
    {
        curtexindex = i;
        tofronttex();
        return;
    }
}

void getcurtex()
{
    if(noedit()) return;
    filltexlist();
    int index = curtexindex < 0 ? 0 : curtexindex;
    if(!texmru.inrange(index)) return;
    intret(texmru[index]);
}

void getseltex()
{
    if(noedit()) return;
    cube &c = lookupcube(sel.o.x, sel.o.y, sel.o.z, -sel.grid);
    if(c.children || isempty(c)) return;
    intret(c.texture[sel.orient]);
}

void gettexname(int *tex, int *subslot)
{
    if(noedit() || *tex<0) return;
    Slot &slot = lookuptexture(*tex);
    if(!slot.sts.inrange(*subslot)) return;
    result(slot.sts[*subslot].name);
}

COMMANDN(edittex, edittex_, "i");
COMMAND(gettex, "");
COMMAND(getcurtex, "");
COMMAND(getseltex, "");
COMMAND(gettexname, "ii");

void replacetexcube(cube &c, int oldtex, int newtex)
{
    loopi(6) if(c.texture[i] == oldtex) c.texture[i] = newtex;
    if(c.children) loopi(8) replacetexcube(c.children[i], oldtex, newtex);
}

void mpreplacetex(int oldtex, int newtex, selinfo &sel, bool local)
{
    if(local) game::edittrigger(sel, EDIT_REPLACE, oldtex, newtex);
    loopi(8) replacetexcube(worldroot[i], oldtex, newtex);
    allchanged();
}

void replace()
{
    if(noedit()) return;
    if(reptex < 0) { conoutf(CON_ERROR, "can only replace after a texture edit"); return; }
    mpreplacetex(reptex, lasttex, sel, true);
}

COMMAND(replace, "");

////////// flip and rotate ///////////////
uint dflip(uint face) { return face==F_EMPTY ? face : 0x88888888 - (((face&0xF0F0F0F0)>>4) | ((face&0x0F0F0F0F)<<4)); }
uint cflip(uint face) { return ((face&0xFF00FF00)>>8) | ((face&0x00FF00FF)<<8); }
uint rflip(uint face) { return ((face&0xFFFF0000)>>16)| ((face&0x0000FFFF)<<16); }
uint mflip(uint face) { return (face&0xFF0000FF) | ((face&0x00FF0000)>>8) | ((face&0x0000FF00)<<8); }

void flipcube(cube &c, int d)
{
    swap(c.texture[d*2], c.texture[d*2+1]);
    c.faces[D[d]] = dflip(c.faces[D[d]]);
    c.faces[C[d]] = cflip(c.faces[C[d]]);
    c.faces[R[d]] = rflip(c.faces[R[d]]);
    if(c.children)
    {
        loopi(8) if(i&octadim(d)) swap(c.children[i], c.children[i-octadim(d)]);
        loopi(8) flipcube(c.children[i], d);
    }
}

void rotatequad(cube &a, cube &b, cube &c, cube &d)
{
    cube t = a; a = b; b = c; c = d; d = t;
}

void rotatecube(cube &c, int d)   // rotates cube clockwise. see pics in cvs for help.
{
    c.faces[D[d]] = cflip (mflip(c.faces[D[d]]));
    c.faces[C[d]] = dflip (mflip(c.faces[C[d]]));
    c.faces[R[d]] = rflip (mflip(c.faces[R[d]]));
    swap(c.faces[R[d]], c.faces[C[d]]);

    swap(c.texture[2*R[d]], c.texture[2*C[d]+1]);
    swap(c.texture[2*C[d]], c.texture[2*R[d]+1]);
    swap(c.texture[2*C[d]], c.texture[2*C[d]+1]);

    if(c.children)
    {
        int row = octadim(R[d]);
        int col = octadim(C[d]);
        for(int i=0; i<=octadim(d); i+=octadim(d)) rotatequad
        (
            c.children[i+row],
            c.children[i],
            c.children[i+col],
            c.children[i+col+row]
        );
        loopi(8) rotatecube(c.children[i], d);
    }
}

void mpflip(selinfo &sel, bool local)
{
    if(local) game::edittrigger(sel, EDIT_FLIP);
    int zs = sel.s[dimension(sel.orient)];
    makeundo();
    loopxy(sel)
    {
        loop(z,zs) flipcube(selcube(x, y, z), dimension(sel.orient));
        loop(z,zs/2)
        {
            cube &a = selcube(x, y, z);
            cube &b = selcube(x, y, zs-z-1);
            swap(a, b);
        }
    }
    changed(sel);
}

void flip()
{
    if(noedit()) return;
    mpflip(sel, true);
}

void mprotate(int cw, selinfo &sel, bool local)
{
    if(local) game::edittrigger(sel, EDIT_ROTATE, cw);
    int d = dimension(sel.orient);
    if(!dimcoord(sel.orient)) cw = -cw;
    int m = sel.s[C[d]] < sel.s[R[d]] ? C[d] : R[d];
    int ss = sel.s[m] = max(sel.s[R[d]], sel.s[C[d]]);
    makeundo();
    loop(z,sel.s[D[d]]) loopi(cw>0 ? 1 : 3)
    {
        loopxy(sel) rotatecube(selcube(x,y,z), d);
        loop(y,ss/2) loop(x,ss-1-y*2) rotatequad
        (
            selcube(ss-1-y, x+y, z),
            selcube(x+y, y, z),
            selcube(y, ss-1-x-y, z),
            selcube(ss-1-x-y, ss-1-y, z)
        );
    }
    changed(sel);
}

void rotate(int *cw)
{
    if(noedit()) return;
    mprotate(*cw, sel, true);
}

COMMAND(flip, "");
COMMAND(rotate, "i");

void setmat(cube &c, uchar mat, uchar matmask, uchar filtermat, uchar filtermask)
{
    if(c.children)
        loopi(8) setmat(c.children[i], mat, matmask, filtermat, filtermask);
    else if(((c.ext ? c.ext->material : MAT_AIR)&filtermask) == filtermat)
    {
        if(mat!=MAT_AIR) 
        {
            cubeext &e = ext(c);
            e.material &= matmask;
            e.material |= mat;
        }
        else if(c.ext) c.ext->material = MAT_AIR;
    }
}

void mpeditmat(int matid, int filter, selinfo &sel, bool local)
{
    if(local) game::edittrigger(sel, EDIT_MAT, matid, filter);

    uchar matmask = matid&MATF_VOLUME ? 0 : (matid&MATF_CLIP ? ~MATF_CLIP : ~matid), 
          filtermat = filter < 0 ? 0 : filter,
          filtermask = filter < 0 ? 0 : (filter&MATF_VOLUME ? MATF_VOLUME : (filter&MATF_CLIP ? MATF_CLIP : filter));
    if(isclipped(matid&MATF_VOLUME)) matid |= MAT_CLIP;
    if(isdeadly(matid&MATF_VOLUME)) matid |= MAT_DEATH;
    if(matid < 0 && filter >= 0)
    {
        matid = 0;
        matmask = filtermask;
        if(isclipped(filter&MATF_VOLUME)) matmask &= ~MATF_CLIP; 
        if(isdeadly(filter&MATF_VOLUME)) matmask &= ~MAT_DEATH; 
    }
    loopselxyz(setmat(c, matid, matmask, filtermat, filtermask));
}

void editmat(char *name, char *filtername)
{
    if(noedit()) return;
    int filter = -1;
    if(filtername[0])
    {
        filter = findmaterial(filtername);
        if(filter < 0) { conoutf(CON_ERROR, "unknown material \"%s\"", filtername); return; }
    }
    int id = -1;
    if(name[0] || filter < 0)
    {
        id = findmaterial(name);
        if(id<0) { conoutf(CON_ERROR, "unknown material \"%s\"", name); return; }
    }
    mpeditmat(id, filter, sel, true);
}

COMMAND(editmat, "ss");

#define TEXGUI_WIDTH 10
#define TEXGUI_HEIGHT 7
extern int menudistance, menuautoclose;

VARP(thumbtime, 0, 50, 1000);

static int lastthumbnail = 0;

VARP(texgui2d, 0, 1, 1);

struct texturegui : g3d_callback 
{
    bool menuon;
    vec menupos;
    int menustart, menutab;
   
    texturegui() : menustart(-1) {} 

    void gui(g3d_gui &g, bool firstpass)
    {
        int origtab = menutab, numtabs = max((curtexnum + TEXGUI_WIDTH*TEXGUI_HEIGHT - 1)/(TEXGUI_WIDTH*TEXGUI_HEIGHT), 1);
        g.start(menustart, 0.04f, &menutab);
        loopi(numtabs)
        {   
            g.tab(!i ? "Textures" : NULL, 0xAAFFAA);
            if(i+1 != origtab) continue; //don't load textures on non-visible tabs!
            loopj(TEXGUI_HEIGHT) 
            {
                g.pushlist();
                loopk(TEXGUI_WIDTH) 
                {
                    int ti = (i*TEXGUI_HEIGHT+j)*TEXGUI_WIDTH+k;
                    if(ti<curtexnum) 
                    {
                        Texture *tex = notexture, *glowtex = NULL, *layertex = NULL;
                        Slot &slot = lookuptexture(ti, false);
                        if(slot.sts.empty()) continue;
                        else if(slot.loaded) 
                        {
                            tex = slot.sts[0].t;
                            if(slot.texmask&(1<<TEX_GLOW)) { loopv(slot.sts) if(slot.sts[i].type==TEX_GLOW) { glowtex = slot.sts[i].t; break; } }
                            if(slot.layer)
                            {
                                Slot &layer = lookuptexture(slot.layer);
                                if(!layer.sts.empty()) layertex = layer.sts[0].t;
                            }
                        }
                        else if(slot.thumbnail) tex = slot.thumbnail;
                        else if(totalmillis-lastthumbnail>=thumbtime) { tex = loadthumbnail(slot); lastthumbnail = totalmillis; }
                        if(g.texture(tex, 1.0, slot.rotation, slot.xoffset, slot.yoffset, glowtex, slot.glowcolor, layertex)&G3D_UP && (slot.loaded || tex!=notexture)) 
                            edittex(ti);
                    }
                    else
                        g.texture(notexture, 1.0); //create an empty space
                }
                g.poplist();
            }
        }
        g.end();
    }

    void showtextures(bool on)
    {
        if(on != menuon && (menuon = on)) 
        { 
            if(menustart <= lasttexmillis) menutab = 1+clamp(lasttex, 0, curtexnum-1)/(TEXGUI_WIDTH*TEXGUI_HEIGHT);
            menupos = menuinfrontofplayer(); 
            menustart = starttime(); 
        }
    }

    void show()
    {   
        if(!menuon) return;
        filltexlist();
        extern int usegui2d;
        if(!editmode || ((!texgui2d || !usegui2d) && camera1->o.dist(menupos) > menuautoclose)) menuon = false;
        else g3d_addgui(this, menupos, texgui2d ? GUI_2D : 0);
    }
} gui;

void g3d_texturemenu() 
{ 
    gui.show(); 
}

void showtexgui(int *n) 
{ 
    if(!editmode) { conoutf(CON_ERROR, "operation only allowed in edit mode"); return; }
    gui.showtextures(*n==0 ? !gui.menuon : *n==1); 
}

// 0/noargs = toggle, 1 = on, other = off - will autoclose if too far away or exit editmode
COMMAND(showtexgui, "i");

void rendertexturepanel(int w, int h)
{
    if((texpaneltimer -= curtime)>0 && editmode)
    {
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glPushMatrix();
        glScalef(h/1800.0f, h/1800.0f, 1);
        int y = 50, gap = 10;

        static Shader *rgbonlyshader = NULL;
        if(!rgbonlyshader) rgbonlyshader = lookupshaderbyname("rgbonly");
        
        rgbonlyshader->set();

        loopi(7)
        {
            int s = (i == 3 ? 285 : 220), ti = curtexindex+i-3;
            if(ti>=0 && ti<curtexnum)
            {
                Slot &slot = lookuptexture(texmru[ti]);
                Texture *tex = slot.sts[0].t, *glowtex = NULL, *layertex = NULL;
                if(slot.texmask&(1<<TEX_GLOW))
                {
                    loopvj(slot.sts) if(slot.sts[j].type==TEX_GLOW) { glowtex = slot.sts[j].t; break; }
                }
                if(slot.layer)
                {
                    Slot &layer = lookuptexture(slot.layer);
                    layertex = layer.sts[0].t;
                }
                float sx = min(1.0f, tex->xs/(float)tex->ys), sy = min(1.0f, tex->ys/(float)tex->xs);
                int x = w*1800/h-s-50, r = s;
                float tc[4][2] = { { 0, 0 }, { 1, 0 }, { 1, 1 }, { 0, 1 } };
                float xoff = slot.xoffset, yoff = slot.yoffset;
                if(slot.rotation)
                {
                    if((slot.rotation&5) == 1) { swap(xoff, yoff); loopk(4) swap(tc[k][0], tc[k][1]); }
                    if(slot.rotation >= 2 && slot.rotation <= 4) { xoff *= -1; loopk(4) tc[k][0] *= -1; }
                    if(slot.rotation <= 2 || slot.rotation == 5) { yoff *= -1; loopk(4) tc[k][1] *= -1; }
                }
                loopk(4) { tc[k][0] = tc[k][0]/sx - xoff/tex->xs; tc[k][1] = tc[k][1]/sy - yoff/tex->ys; }
                glBindTexture(GL_TEXTURE_2D, tex->id);
                loopj(glowtex ? 3 : 2)
                {
                    if(j < 2) glColor4f(j, j, j, texpaneltimer/1000.0f);
                    else
                    {
                        glBindTexture(GL_TEXTURE_2D, glowtex->id);
                        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
                        glColor4f(slot.glowcolor.x, slot.glowcolor.y, slot.glowcolor.z, texpaneltimer/1000.0f);
                    }
                    glBegin(GL_QUADS);
                    glTexCoord2fv(tc[0]); glVertex2f(x,   y);
                    glTexCoord2fv(tc[1]); glVertex2f(x+r, y);
                    glTexCoord2fv(tc[2]); glVertex2f(x+r, y+r);
                    glTexCoord2fv(tc[3]); glVertex2f(x,   y+r);
                    glEnd();
                    xtraverts += 4;
                    if(j==1 && layertex)
                    {
                        glBindTexture(GL_TEXTURE_2D, layertex->id);
                        glBegin(GL_QUADS);
                        glTexCoord2fv(tc[0]); glVertex2f(x+r/2, y+r/2);
                        glTexCoord2fv(tc[1]); glVertex2f(x+r,   y+r/2);
                        glTexCoord2fv(tc[2]); glVertex2f(x+r,   y+r);
                        glTexCoord2fv(tc[3]); glVertex2f(x+r/2, y+r);
                        glEnd();
                        xtraverts += 4;
                    }
                    if(!j)
                    {
                        r -= 10;
                        x += 5;
                        y += 5;
                    }
                    else if(j == 2) glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                }
            }
            y += s+gap;
        }

        defaultshader->set();

        glPopMatrix();
    }
}
