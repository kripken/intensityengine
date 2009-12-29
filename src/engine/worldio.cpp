// worldio.cpp: loading & saving of maps and savegames

#include "engine.h"
#include "game.h" // INTENSITY

// INTENSITY
#include "intensity.h"
#include "world_system.h"
#include "message_system.h"
#include "client_system.h"


void backup(char *name, char *backupname)
{
    string backupfile;
    copystring(backupfile, findfile(backupname, "wb"));
    remove(backupfile);
    rename(findfile(name, "wb"), backupfile);
}

string ogzname, bakname, cfgname, picname;

VARP(savebak, 0, 2, 2);

void cutogz(char *s)
{
    char *ogzp = strstr(s, ".ogz");
    if(ogzp) *ogzp = '\0';
}

void getmapfilenames(const char *fname, const char *cname, char *pakname, char *mapname, char *cfgname)
{
    if(!cname) cname = fname;
    string name;
    copystring(name, cname, 100);
    cutogz(name);
    char *slash = strpbrk(name, "/\\");
    if(slash)
    {
        copystring(pakname, name, slash-name+1);
        copystring(cfgname, slash+1);
    }
    else
    {
        copystring(pakname, "base");
        copystring(cfgname, name);
    }
    if(strpbrk(fname, "/\\")) copystring(mapname, fname);
    else formatstring(mapname)("base/%s", fname);
    cutogz(mapname);
}

void setmapfilenames(const char *fname, const char *cname = 0)
{
    string pakname, mapname, mcfgname;
    getmapfilenames(fname, cname, pakname, mapname, mcfgname);

    formatstring(ogzname)("packages/%s.ogz", mapname);
    if(savebak==1) formatstring(bakname)("packages/%s.BAK", mapname);
    else formatstring(bakname)("packages/%s_%d.BAK", mapname, totalmillis);
    formatstring(cfgname)("packages/%s/%s.cfg", pakname, mcfgname);
    formatstring(picname)("packages/%s.jpg", mapname);

    path(ogzname);
    path(bakname);
    path(cfgname);
    path(picname);
}

void mapcfgname()
{
    const char *mname = game::getclientmap();
    if(!*mname) mname = "untitled";

    string pakname, mapname, mcfgname;
    getmapfilenames(mname, NULL, pakname, mapname, mcfgname);
    defformatstring(cfgname)("packages/%s/%s.js", pakname, mcfgname); // INTENSITY: Switched to .js TODO: More files
    path(cfgname);
    result(cfgname);
}

COMMAND(mapcfgname, "");

enum { OCTSAV_CHILDREN = 0, OCTSAV_EMPTY, OCTSAV_SOLID, OCTSAV_NORMAL, OCTSAV_LODCUBE };

void savec(cube *c, stream *f, bool nolms)
{
    loopi(8)
    {
        if(c[i].children && (!c[i].ext || !c[i].ext->surfaces))
        {
            f->putchar(OCTSAV_CHILDREN);
            savec(c[i].children, f, nolms);
        }
        else
        {
            int oflags = 0;
            if(c[i].ext && c[i].ext->merged) oflags |= 0x80;
            if(c[i].children) f->putchar(oflags | OCTSAV_LODCUBE);
            else if(isempty(c[i])) f->putchar(oflags | OCTSAV_EMPTY);
            else if(isentirelysolid(c[i])) f->putchar(oflags | OCTSAV_SOLID);
            else
            {
                f->putchar(oflags | OCTSAV_NORMAL);
                f->write(c[i].edges, 12);
            }
            loopj(6) f->putlil<ushort>(c[i].texture[j]);
            uchar mask = 0;
            if(c[i].ext)
            {
                if(c[i].ext->material != MAT_AIR) mask |= 0x80;
                if(c[i].ext->normals && !nolms)
                {
                    mask |= 0x40;
                    loopj(6) if(c[i].ext->normals[j].normals[0] != bvec(128, 128, 128)) mask |= 1 << j;
                }
            }
            // save surface info for lighting
            if(!c[i].ext || !c[i].ext->surfaces || nolms)
            {
                f->putchar(mask);
                if(c[i].ext)
                {
                    if(c[i].ext->material != MAT_AIR) f->putchar(c[i].ext->material);
                    if(c[i].ext->normals && !nolms) loopj(6) if(mask & (1 << j))
                    {
                        loopk(sizeof(surfaceinfo)) f->putchar(0);
                        f->write(&c[i].ext->normals[j], sizeof(surfacenormals));
                    } 
                }
            }
            else
            {
                int numsurfs = 6;
                loopj(6) 
                {
                    surfaceinfo &surface = c[i].ext->surfaces[j];
                    if(surface.lmid >= LMID_RESERVED || surface.layer!=LAYER_TOP) 
                    {
                        mask |= 1 << j;
                        if(surface.layer&LAYER_BLEND) numsurfs++;
                    }
                }
                f->putchar(mask);
                if(c[i].ext->material != MAT_AIR) f->putchar(c[i].ext->material);
                loopj(numsurfs) if(j >= 6 || mask & (1 << j))
                {
                    surfaceinfo tmp = c[i].ext->surfaces[j];
                    lilswap(&tmp.x, 2);
                    f->write(&tmp, sizeof(surfaceinfo));
                    if(j < 6 && c[i].ext->normals) f->write(&c[i].ext->normals[j], sizeof(surfacenormals));
                }
            }
            if(c[i].ext && c[i].ext->merged)
            {
                f->putchar(c[i].ext->merged | (c[i].ext->mergeorigin ? 0x80 : 0));
                if(c[i].ext->mergeorigin)
                {
                    f->putchar(c[i].ext->mergeorigin);
                    int index = 0;
                    loopj(6) if(c[i].ext->mergeorigin&(1<<j))
                    {
                        mergeinfo tmp = c[i].ext->merges[index++];
                        lilswap(&tmp.u1, 4);
                        f->write(&tmp, sizeof(mergeinfo));
                    }
                }
            }
            if(c[i].children) savec(c[i].children, f, nolms);
        }
    }
}

cube *loadchildren(stream *f);

void loadc(stream *f, cube &c)
{
    bool haschildren = false;
    int octsav = f->getchar();
    switch(octsav&0x7)
    {
        case OCTSAV_CHILDREN:
            c.children = loadchildren(f);
            return;

        case OCTSAV_LODCUBE: haschildren = true;    break;
        case OCTSAV_EMPTY:  emptyfaces(c);          break;
        case OCTSAV_SOLID:  solidfaces(c);          break;
        case OCTSAV_NORMAL: f->read(c.edges, 12); break;

        default:
            fatal("garbage in map");
    }
    loopi(6) c.texture[i] = mapversion<14 ? f->getchar() : f->getlil<ushort>();
    if(mapversion < 7) f->seek(3, SEEK_CUR);
    else
    {
        uchar mask = f->getchar();
        if(mask & 0x80) 
        {
            int mat = f->getchar();
            if(mapversion < 27)
            {
                static uchar matconv[] = { MAT_AIR, MAT_WATER, MAT_CLIP, MAT_GLASS|MAT_CLIP, MAT_NOCLIP, MAT_LAVA|MAT_DEATH, MAT_GAMECLIP, MAT_DEATH };
                mat = size_t(mat) < sizeof(matconv)/sizeof(matconv[0]) ? matconv[mat] : MAT_AIR;
            }
            ext(c).material = mat;
        }
        if(mask & 0x3F)
        {
            uchar lit = 0, bright = 0;
            static surfaceinfo surfaces[12];
            memset(surfaces, 0, 6*sizeof(surfaceinfo));
            if(mask & 0x40) newnormals(c);
            int numsurfs = 6;
            loopi(numsurfs)
            {
                if(i >= 6 || mask & (1 << i))
                {
                    f->read(&surfaces[i], sizeof(surfaceinfo));
                    lilswap(&surfaces[i].x, 2);
                    if(mapversion < 10) ++surfaces[i].lmid;
                    if(mapversion < 18)
                    {
                        if(surfaces[i].lmid >= LMID_AMBIENT1) ++surfaces[i].lmid;
                        if(surfaces[i].lmid >= LMID_BRIGHT1) ++surfaces[i].lmid;
                    }
                    if(mapversion < 19)
                    {
                        if(surfaces[i].lmid >= LMID_DARK) surfaces[i].lmid += 2;
                    }
                    if(i < 6)
                    {
                        if(mask & 0x40) f->read(&c.ext->normals[i], sizeof(surfacenormals));
                        if(surfaces[i].layer != LAYER_TOP) lit |= 1 << i;
                        else if(surfaces[i].lmid == LMID_BRIGHT) bright |= 1 << i;
                        else if(surfaces[i].lmid != LMID_AMBIENT) lit |= 1 << i;
                        if(surfaces[i].layer&LAYER_BLEND) numsurfs++;
                    }
                }
                else surfaces[i].lmid = LMID_AMBIENT;
            }
            if(lit) newsurfaces(c, surfaces, numsurfs);
            else if(bright) brightencube(c);
        }
        if(mapversion >= 20)
        {
            if(octsav&0x80)
            {
                int merged = f->getchar();
                ext(c).merged = merged&0x3F;
                if(merged&0x80)
                {
                    c.ext->mergeorigin = f->getchar();
                    int nummerges = 0;
                    loopi(6) if(c.ext->mergeorigin&(1<<i)) nummerges++;
                    if(nummerges)
                    {
                        c.ext->merges = new mergeinfo[nummerges];
                        loopi(nummerges)
                        {
                            mergeinfo *m = &c.ext->merges[i];
                            f->read(m, sizeof(mergeinfo));
                            lilswap(&m->u1, 4);
                            if(mapversion <= 25)
                            {
                                int uorigin = m->u1 & 0xE000, vorigin = m->v1 & 0xE000;
                                m->u1 = (m->u1 - uorigin) << 2;
                                m->u2 = (m->u2 - uorigin) << 2;
                                m->v1 = (m->v1 - vorigin) << 2;
                                m->v2 = (m->v2 - vorigin) << 2;
                            }
                        }
                    }
                }
            }    
        }                
    }
    c.children = (haschildren ? loadchildren(f) : NULL);
}

cube *loadchildren(stream *f)
{
    cube *c = newcubes();
    loopi(8) loadc(f, c[i]);
    // TODO: remip c from children here
    return c;
}

VAR(dbgvars, 0, 0, 1);

bool save_world(const char *mname, bool nolms)
{
    if(!*mname) mname = game::getclientmap();
    setmapfilenames(*mname ? mname : "untitled");
    if(savebak) backup(ogzname, bakname);
    stream *f = opengzfile(ogzname, "wb");
    if(!f) { conoutf(CON_WARN, "could not write map to %s", ogzname); return false; }
    octaheader hdr;
    memcpy(hdr.magic, "OCTA", 4);
    hdr.version = MAPVERSION;
    hdr.headersize = sizeof(hdr);
    hdr.worldsize = worldsize;
    hdr.numents = 0;
//    const vector<extentity *> &ents = entities::getents(); // INTENSITY: No ents in .ogz
//    loopv(ents) if(ents[i]->type!=ET_EMPTY || nolms) hdr.numents++; // INTENSITY: No ents in .ogz
    hdr.numpvs = nolms ? 0 : getnumviewcells();
    hdr.lightmaps = nolms ? 0 : lightmaps.length();
    hdr.blendmap = shouldsaveblendmap();
    hdr.numvars = 0;
    enumerate(*idents, ident, id, 
    {
        if((id.type == ID_VAR || id.type == ID_FVAR || id.type == ID_SVAR) && id.flags&IDF_OVERRIDE && !(id.flags&IDF_READONLY) && id.override!=NO_OVERRIDE) hdr.numvars++;
    });
    lilswap(&hdr.version, 8);
    f->write(&hdr, sizeof(hdr));
   
    enumerate(*idents, ident, id, 
    {
        if((id.type!=ID_VAR && id.type!=ID_FVAR && id.type!=ID_SVAR) || !(id.flags&IDF_OVERRIDE) || id.flags&IDF_READONLY || id.override==NO_OVERRIDE) continue;
        f->putchar(id.type);
        f->putlil<ushort>(strlen(id.name));
        f->write(id.name, strlen(id.name));
        switch(id.type)
        {
            case ID_VAR:
                if(dbgvars) conoutf(CON_DEBUG, "wrote var %s: %d", id.name, *id.storage.i);
                f->putlil<int>(*id.storage.i);
                break;

            case ID_FVAR:
                if(dbgvars) conoutf(CON_DEBUG, "wrote fvar %s: %f", id.name, *id.storage.f);
                f->putlil<float>(*id.storage.f);
                break;

            case ID_SVAR:
                if(dbgvars) conoutf(CON_DEBUG, "wrote svar %s: %s", id.name, *id.storage.s);
                f->putlil<ushort>(strlen(*id.storage.s));
                f->write(*id.storage.s, strlen(*id.storage.s));
                break;
        }
    });

    if(dbgvars) conoutf(CON_DEBUG, "wrote %d vars", hdr.numvars);

    f->putchar((int)strlen(game::gameident()));
    f->write(game::gameident(), (int)strlen(game::gameident())+1);
    f->putlil<ushort>(entities::extraentinfosize());
    vector<char> extras;
    game::writegamedata(extras);
    f->putlil<ushort>(extras.length());
    f->write(extras.getbuf(), extras.length());
    
    f->putlil<ushort>(texmru.length());
    loopv(texmru) f->putlil<ushort>(texmru[i]);
#if 0 // INTENSITY: No ents in .ogz
    char *ebuf = new char[entities::extraentinfosize()];
    loopv(ents)
    {
        if(ents[i]->type!=ET_EMPTY || nolms)
        {
            entity tmp = *ents[i];
            lilswap(&tmp.o.x, 3);
            lilswap(&tmp.attr1, 5);
            f->write(&tmp, sizeof(entity));
            entities::writeent(*ents[i], ebuf);
            if(entities::extraentinfosize()) f->write(ebuf, entities::extraentinfosize());
        }
    }
    delete[] ebuf;
#endif // 0

    savec(worldroot, f, nolms);
    if(!nolms) 
    {
        loopv(lightmaps)
        {
            LightMap &lm = lightmaps[i];
            f->putchar(lm.type | (lm.unlitx>=0 ? 0x80 : 0));
            if(lm.unlitx>=0)
            {
                f->putlil<ushort>(ushort(lm.unlitx));
                f->putlil<ushort>(ushort(lm.unlity));
            }
            f->write(lm.data, lm.bpp*LM_PACKW*LM_PACKH);
        }
        if(getnumviewcells()>0) savepvs(f);
    }
    if(shouldsaveblendmap()) saveblendmap(f);

    delete f;
    conoutf("wrote map file %s", ogzname);

    return true;
}

static uint mapcrc = 0;

uint getmapcrc() { return mapcrc; }

static void swapXZ(cube *c)
{	
	loopi(8) 
	{
		swap(c[i].faces[0],   c[i].faces[2]);
		swap(c[i].texture[0], c[i].texture[4]);
		swap(c[i].texture[1], c[i].texture[5]);
		if(c[i].ext && c[i].ext->surfaces)
		{
			swap(c[i].ext->surfaces[0], c[i].ext->surfaces[4]);
			swap(c[i].ext->surfaces[1], c[i].ext->surfaces[5]);
		}
		if(c[i].children) swapXZ(c[i].children);
	}
}

static void fixoversizedcubes(cube *c, int size)
{
    if(size <= VVEC_INT_MASK+1) return;
    loopi(8)
    {
        if(!c[i].children) subdividecube(c[i], true, false);
        fixoversizedcubes(c[i].children, size>>1);
    }
}

bool finish_load_world(); // INTENSITY: Added this, and use it inside load_world

const char *_saved_mname = NULL; // INTENSITY
const char *_saved_cname = NULL; // INTENSITY
octaheader *saved_hdr = NULL; // INTENSITY

bool load_world(const char *mname, const char *cname)        // still supports all map formats that have existed since the earliest cube betas!
{
    WorldSystem::loadingWorld = true; // INTENSITY
    LogicSystem::init(); // INTENSITY: Start our game data system, wipe all existing LogicEntities, and add the player

#if 0 // INTENSITY
    int loadingstart = SDL_GetTicks();
#endif
    setmapfilenames(mname, cname);

    _saved_mname = mname; // INTENSITY
    _saved_cname = cname; // INTENSITY

    stream *f = opengzfile(ogzname, "rb");
    if(!f) { conoutf(CON_ERROR, "could not read map %s", ogzname); return false; }
    saved_hdr = new octaheader; // INTENSITY
    octaheader& hdr = *saved_hdr; // INTENSITY
    if(f->read(&hdr, 7*sizeof(int))!=7*sizeof(int)) { conoutf(CON_ERROR, "map %s has malformatted header", ogzname); delete f; return false; }
    lilswap(&hdr.version, 6);
    if(strncmp(hdr.magic, "OCTA", 4)!=0 || hdr.worldsize <= 0|| hdr.numents < 0) { conoutf(CON_ERROR, "map %s has malformatted header", ogzname); delete f; return false; }
    if(hdr.version>MAPVERSION) { conoutf(CON_ERROR, "map %s requires a newer version of Cube 2: Sauerbraten", ogzname); delete f; return false; }
    compatheader chdr;
    if(hdr.version <= 28)
    {
        if(f->read(&chdr.lightprecision, sizeof(chdr) - 7*sizeof(int)) != sizeof(chdr) - 7*sizeof(int)) { conoutf(CON_ERROR, "map %s has malformatted header", ogzname); delete f; return false; }
    }
    else if(f->read(&hdr.blendmap, sizeof(hdr) - 7*sizeof(int)) != sizeof(hdr) - 7*sizeof(int)) { conoutf(CON_ERROR, "map %s has malformatted header", ogzname); delete f; return false; }

    resetmap();
    Texture *mapshot = textureload(picname, 3, true, false);
    renderbackground("loading...", mapshot, mname, game::getmapinfo());

    setvar("mapversion", hdr.version, true, false);

    if(hdr.version <= 28)
    {
        lilswap(&chdr.lightprecision, 3);
        if(hdr.version<=20) conoutf(CON_WARN, "loading older / less efficient map format, may benefit from \"calclight 2\", then \"savecurrentmap\"");
        if(chdr.lightprecision) setvar("lightprecision", chdr.lightprecision);
        if(chdr.lighterror) setvar("lighterror", chdr.lighterror);
        if(chdr.bumperror) setvar("bumperror", chdr.bumperror);
        setvar("lightlod", chdr.lightlod);
        if(chdr.ambient) setvar("ambient", chdr.ambient);
        setvar("skylight", (int(chdr.skylight[0])<<16) | (int(chdr.skylight[1])<<8) | int(chdr.skylight[2]));
        setvar("watercolour", (int(chdr.watercolour[0])<<16) | (int(chdr.watercolour[1])<<8) | int(chdr.watercolour[2]), true);
        setvar("waterfallcolour", (int(chdr.waterfallcolour[0])<<16) | (int(chdr.waterfallcolour[1])<<8) | int(chdr.waterfallcolour[2]));
        setvar("lavacolour", (int(chdr.lavacolour[0])<<16) | (int(chdr.lavacolour[1])<<8) | int(chdr.lavacolour[2]));
        setvar("fullbright", 0, true);
        if(chdr.lerpsubdivsize || chdr.lerpangle) setvar("lerpangle", chdr.lerpangle);
        if(chdr.lerpsubdivsize)
        {
            setvar("lerpsubdiv", chdr.lerpsubdiv);
            setvar("lerpsubdivsize", chdr.lerpsubdivsize);
        }
        setsvar("maptitle", chdr.maptitle);
        hdr.blendmap = chdr.blendmap;
        hdr.numvars = 0; 
    }
    else lilswap(&hdr.blendmap, 2);
 
    loopi(hdr.numvars)
    {
        int type = f->getchar(), ilen = f->getlil<ushort>();
        string name;
        f->read(name, min(ilen, MAXSTRLEN-1));
        name[min(ilen, MAXSTRLEN-1)] = '\0';
        if(ilen >= MAXSTRLEN) f->seek(ilen - (MAXSTRLEN-1), SEEK_CUR);
        ident *id = getident(name);
        bool exists = id && id->type == type;
        switch(type)
        {
            case ID_VAR:
            {
                int val = f->getlil<int>();
                if(exists && id->minval <= id->maxval) setvar(name, val);
                if(dbgvars) conoutf(CON_DEBUG, "read var %s: %d", name, val);
                break;
            }
 
            case ID_FVAR:
            {
                float val = f->getlil<float>();
                if(exists && id->minvalf <= id->maxvalf) setfvar(name, val);
                if(dbgvars) conoutf(CON_DEBUG, "read fvar %s: %f", name, val);
                break;
            }
    
            case ID_SVAR:
            {
                int slen = f->getlil<ushort>();
                string val;
                f->read(val, min(slen, MAXSTRLEN-1));
                val[min(slen, MAXSTRLEN-1)] = '\0';
                if(slen >= MAXSTRLEN) f->seek(slen - (MAXSTRLEN-1), SEEK_CUR);
                if(exists) setsvar(name, val);
                if(dbgvars) conoutf(CON_DEBUG, "read svar %s: %s", name, val);
                break;
            }
        }
    }
    if(dbgvars) conoutf(CON_DEBUG, "read %d vars", hdr.numvars);

    string gametype;
    copystring(gametype, "fps");
    bool samegame = true;
    int eif = 0;
    if(hdr.version>=16)
    {
        int len = f->getchar();
        f->read(gametype, len+1);
    }
    if(strcmp(gametype, game::gameident())!=0)
    {
        samegame = false;
        conoutf(CON_WARN, "WARNING: loading map from %s game, ignoring entities except for lights/mapmodels)", gametype);
    }
    if(hdr.version>=16)
    {
        eif = f->getlil<ushort>();
        int extrasize = f->getlil<ushort>();
        vector<char> extras;
        loopj(extrasize) extras.add(f->getchar());
        if(samegame) game::readgamedata(extras);
    }
    
    renderprogress(0, "clearing world...");

    texmru.setsize(0);
    if(hdr.version<14)
    {
        uchar oldtl[256];
        f->read(oldtl, sizeof(oldtl));
        loopi(256) texmru.add(oldtl[i]);
    }
    else
    {
        ushort nummru = f->getlil<ushort>();
        loopi(nummru) texmru.add(f->getlil<ushort>());
    }

    freeocta(worldroot);
    worldroot = NULL;

    setvar("mapsize", hdr.worldsize, true, false);
    int worldscale = 0;
    while(1<<worldscale < hdr.worldsize) worldscale++;
    setvar("mapscale", worldscale, true, false);

    renderprogress(0, "loading entities...");

    vector<extentity *> &ents = entities::getents();
    int einfosize = entities::extraentinfosize();
    char *ebuf = einfosize > 0 ? new char[einfosize] : NULL;
    loopi(min(hdr.numents, MAXENTS))
    {
//        extentity &e = *entities::newentity();
//        ents.add(&e);
        extentity e; // INTENSITY: Do *NOT* actually load entities from .ogz files - we use our own system.
                     // But, read the data from the file so we can move on (might be a sauer .ogz)
                     // So 'e' here is just a dummy

        f->read(&e, sizeof(entity));
        lilswap(&e.o.x, 3);
        lilswap(&e.attr1, 5);
        e.spawned = false;
        e.inoctanode = false;
        if(samegame)
        {
            if(einfosize > 0) f->read(ebuf, einfosize);
            entities::readent(e, ebuf); 
        }
        else f->seek(eif, SEEK_CUR);
        if(hdr.version <= 14 && e.type >= ET_MAPMODEL && e.type <= 16)
        {
            if(e.type == 16) e.type = ET_MAPMODEL;
            else e.type++;
        }
        if(hdr.version <= 20 && e.type >= ET_ENVMAP) e.type++;
        if(hdr.version <= 21 && e.type >= ET_PARTICLES) e.type++;
        if(hdr.version <= 22 && e.type >= ET_SOUND) e.type++;
        if(hdr.version <= 23 && e.type >= ET_SPOTLIGHT) e.type++;
        if(!samegame)
        {
            if(e.type>=ET_GAMESPECIFIC || hdr.version<=14)
            {
                entities::deleteentity(ents.pop());
                continue;
            }
        }
        if(!insideworld(e.o))
        {
            if(e.type != ET_LIGHT && e.type != ET_SPOTLIGHT)
            {
                conoutf(CON_WARN, "warning: ent outside of world: enttype[%s] index %d (%f, %f, %f)", entities::entname(e.type), i, e.o.x, e.o.y, e.o.z);
            }
        }
        if(hdr.version <= 14 && e.type == ET_MAPMODEL)
        {
            e.o.z += e.attr3;
            if(e.attr4) conoutf(CON_WARN, "warning: mapmodel ent (index %d) uses texture slot %d", i, e.attr4);
            e.attr3 = e.attr4 = 0;
        }

        // INTENSITY: Print ent out, useful for copy-paste importing sauer maps
        static int uniqueId = 0;
        printf("[%d, \"", uniqueId);
        if (e.type == ET_LIGHT) printf("Light");
        else if (e.type == ET_MAPMODEL) printf("Mapmodel");
        else if (e.type == ET_PLAYERSTART) printf("WorldMarker");
        else if (e.type == ET_ENVMAP) printf("EnvironmentMap***");
        else if (e.type == ET_PARTICLES) printf("ParticleEffect");
        else if (e.type == ET_SOUND) printf("SoundEffect***");
        else if (e.type == ET_SPOTLIGHT) printf("SpotLight***");
        printf("\", {");
            printf("\"position\":\"[%f|%f|%f]\", ", e.o.x, e.o.y, e.o.z);
            printf("\"attr1\":\"%d\", ", e.attr1);
            printf("\"attr2\":\"%d\", ", e.attr2);
            printf("\"attr3\":\"%d\", ", e.attr3);
            printf("\"attr4\":\"%d\", ", e.attr4);
            printf("\"_persistent\":\"true\"");
        printf("}], \r\n");
        uniqueId++;
        // INTENSITY: end Print ent out
    }
    if(ebuf) delete[] ebuf;

    if(hdr.numents > MAXENTS) 
    {
        conoutf(CON_WARN, "warning: map has %d entities", hdr.numents);
        f->seek((hdr.numents-MAXENTS)*(sizeof(entity) + einfosize), SEEK_CUR);
    }

    renderprogress(0, "loading octree...");
    worldroot = loadchildren(f);

	if(hdr.version <= 11)
		swapXZ(worldroot);

    if(hdr.version <= 8)
        converttovectorworld();

    if(hdr.version <= 25 && hdr.worldsize > VVEC_INT_MASK+1)
        fixoversizedcubes(worldroot, hdr.worldsize>>1);

    renderprogress(0, "validating...");
    validatec(worldroot, hdr.worldsize>>1);

#ifdef CLIENT // INTENSITY: Server doesn't need lightmaps, pvs and blendmap (and current code for server wouldn't clean
              //            them up if we did read them, so would have a leak)
    if(hdr.version >= 7) loopi(hdr.lightmaps)
    {
        renderprogress(i/(float)hdr.lightmaps, "loading lightmaps...");
        LightMap &lm = lightmaps.add();
        if(hdr.version >= 17)
        {
            int type = f->getchar();
            lm.type = type&0x7F;
            if(hdr.version >= 20 && type&0x80)
            {
                lm.unlitx = f->getlil<ushort>();
                lm.unlity = f->getlil<ushort>();
            }
        }
        if(lm.type&LM_ALPHA && (lm.type&LM_TYPE)!=LM_BUMPMAP1) lm.bpp = 4;
        lm.data = new uchar[lm.bpp*LM_PACKW*LM_PACKH];
        f->read(lm.data, lm.bpp * LM_PACKW * LM_PACKH);
        lm.finalize();
    }

    if(hdr.version >= 25 && hdr.numpvs > 0) loadpvs(f, hdr.numpvs);
    if(hdr.version >= 28 && hdr.blendmap) loadblendmap(f, hdr.blendmap);
#endif // INTENSITY

//    mapcrc = f->getcrc(); // INTENSITY: We use our own signatures
    delete f;

#if 0 // INTENSITY
    conoutf("read map %s (%.1f seconds)", ogzname, (SDL_GetTicks()-loadingstart)/1000.0f);
#endif

    clearmainmenu();

    overrideidents = true;
    execfile("data/default_map_settings.cfg", false);
#if 0 // INTENSITY: Use our scripting script instead
    execfile(cfgname, false);
#else
    WorldSystem::runMapScript();
#endif
    overrideidents = false;
   
#ifdef CLIENT // INTENSITY: Stop, finish loading later when we have all the entities
    renderprogress(0, "requesting entities...");
    Logging::log(Logging::DEBUG, "Requesting active entities...\r\n");
    MessageSystem::send_ActiveEntitiesRequest(ClientSystem::currScenarioCode); // Ask for the NPCs and other players, which are not part of the map proper
#else // SERVER
    Logging::log(Logging::DEBUG, "Finishing loading of the world...\r\n");
    finish_load_world();
#endif

    return true;
}

bool finish_load_world() // INTENSITY: Second half, after all entities received
{
    renderprogress(0, "finalizing world..."); // INTENSITY

    const char *mname = _saved_mname; // INTENSITY
    const char *cname = _saved_cname; // INTENSITY
    octaheader& hdr = *saved_hdr; // INTENSITY

    extern void fixlightmapnormals();
    if(hdr.version <= 25) fixlightmapnormals();

#if 0 // INTENSITY: We use our own preloading system
    vector<int> mapmodels;
    loopv(ents)
    {
        extentity &e = *ents[i];
        if(e.type==ET_MAPMODEL && e.attr2 >= 0)
        {
            if(mapmodels.find(e.attr2) < 0) mapmodels.add(e.attr2);
        }
    }

    loopv(mapmodels)
    {
        loadprogress = float(i+1)/mapmodels.length();
        int mmindex = mapmodels[i];
        mapmodelinfo &mmi = getmminfo(mmindex);
        if(!&mmi) conoutf(CON_WARN, "could not find map model: %d", mmindex);
        else if(!loadmodel(NULL, mmindex, true)) conoutf(CON_WARN, "could not load model: %s", mmi.name);
    }
#endif // INTENSITY

    loadprogress = 0;

    game::preload();
    flushpreloadedmodels();

    entitiesinoctanodes();
    attachentities();
    initlights();
    allchanged(true);

//    if(maptitle[0]) conoutf(CON_ECHO, "%s", maptitle); // INTENSITY

    startmap(cname ? cname : mname);
    
    Logging::log(Logging::DEBUG, "load_world complete.\r\n"); // INTENSITY
    WorldSystem::loadingWorld = false; // INTENSITY

    delete saved_hdr; // INTENSITY

    printf("\r\n\r\n[[MAP LOADING]] - Success.\r\n"); // INTENSITY

    return true;
}

void savecurrentmap() { save_world(game::getclientmap()); }
void savemap(char *mname) { save_world(mname); }

COMMAND(savemap, "s");
COMMAND(savecurrentmap, "");

void writeobj(char *name)
{
    defformatstring(fname)("%s.obj", name);
    stream *f = openfile(path(fname), "w"); 
    if(!f) return;
    f->printf("# obj file of Cube 2: Sauerbraten level\n");
    extern vector<vtxarray *> valist;
    loopv(valist)
    {
        vtxarray &va = *valist[i];
        ushort *edata = NULL;
        uchar *vdata = NULL;
        if(!readva(&va, edata, vdata)) continue;
        int vtxsize = VTXSIZE;
        uchar *vert = vdata;
        loopj(va.verts) 
        {
            vec v;
            if(floatvtx) (v = *(vec *)vert).div(1<<VVEC_FRAC); 
            else v = ((vvec *)vert)->tovec(va.o).add(0x8000>>VVEC_FRAC);
            if(v.x != floor(v.x)) f->printf("v %.3f ", v.x); else f->printf("v %d ", int(v.x));
            if(v.y != floor(v.y)) f->printf("%.3f ", v.y); else f->printf("%d ", int(v.y));
            if(v.z != floor(v.z)) f->printf("%.3f\n", v.z); else f->printf("%d\n", int(v.z));
            vert += vtxsize;
        }
        ushort *tri = edata;
        loopi(va.tris)
        {
            f->printf("f");
            for(int k = 0; k<3; k++) f->printf(" %d", tri[k]-va.verts-va.voffset);
            tri += 3;
            f->printf("\n");
        }
        delete[] edata;
        delete[] vdata;
    }
    delete f;
}  
    
COMMAND(writeobj, "s"); 

