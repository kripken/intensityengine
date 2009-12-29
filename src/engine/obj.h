struct obj;

obj *loadingobj = 0;

string objdir;

struct obj : vertmodel
{
    obj(const char *name) : vertmodel(name) {}

    int type() const { return MDL_OBJ; }

    struct objmeshgroup : vertmeshgroup
    {
        void parsevert(char *s, vector<vec> &out)
        {
            vec &v = out.add(vec(0, 0, 0));
            while(isalpha(*s)) s++;
            loopi(3)
            {
                v[i] = strtod(s, &s);
                while(isspace(*s)) s++;
                if(!*s) break;
            }
        }

        bool load(char *filename)
        {
            int len = strlen(filename);
            if(len < 4 || strcasecmp(&filename[len-4], ".obj")) return false;

            stream *file = openfile(filename, "rb");
            if(!file) return false;

            name = newstring(filename);

            numframes = 1;

            vector<vec> attrib[3];
            char buf[512];

            hashtable<ivec, int> verthash;
            vector<vert> verts;
            vector<tcvert> tcverts;
            vector<tri> tris;

            #define STARTMESH do { \
                vertmesh &m = *new vertmesh; \
                m.group = this; \
                m.name = meshname[0] ? newstring(meshname) : NULL; \
                meshes.add(&m); \
                curmesh = &m; \
                verthash.clear(); \
                verts.setsizenodelete(0); \
                tcverts.setsizenodelete(0); \
                tris.setsizenodelete(0); \
            } while(0)

            #define FLUSHMESH do { \
                curmesh->numverts = verts.length(); \
                if(verts.length()) \
                { \
                    curmesh->verts = new vert[verts.length()]; \
                    memcpy(curmesh->verts, verts.getbuf(), verts.length()*sizeof(vert)); \
                    curmesh->tcverts = new tcvert[verts.length()]; \
                    memcpy(curmesh->tcverts, tcverts.getbuf(), tcverts.length()*sizeof(tcvert)); \
                } \
                curmesh->numtris = tris.length(); \
                if(tris.length()) \
                { \
                    curmesh->tris = new tri[tris.length()]; \
                    memcpy(curmesh->tris, tris.getbuf(), tris.length()*sizeof(tri)); \
                } \
                if(attrib[2].empty()) curmesh->buildnorms(); \
            } while(0)

            string meshname = "";
            vertmesh *curmesh = NULL;
            while(file->getline(buf, sizeof(buf)))
            {
                char *c = buf;
                while(isspace(*c)) c++;
                switch(*c)
                {
                    case '#': continue;
                    case 'v':
                        if(isspace(c[1])) parsevert(c, attrib[0]);
                        else if(c[1]=='t') parsevert(c, attrib[1]);
                        else if(c[1]=='n') parsevert(c, attrib[2]);
                        break;
                    case 'g':
                    {
                        while(isalpha(*c)) c++;
                        while(isspace(*c)) c++;
                        char *name = c;
                        size_t namelen = strlen(name);
                        while(namelen > 0 && isspace(name[namelen-1])) namelen--;
                        copystring(meshname, name, min(namelen+1, sizeof(meshname)));

                        if(curmesh) FLUSHMESH;
                        curmesh = NULL;
                        break;
                    }
                    case 'f':
                    {
                        if(!curmesh) STARTMESH;
                        int v0 = -1, v1 = -1;
                        while(isalpha(*c)) c++;
                        for(;;)
                        {
                            while(isspace(*c)) c++;
                            if(!*c) break; 
                            ivec vkey(-1, -1, -1);
                            loopi(3)
                            {
                                vkey[i] = strtol(c, &c, 10);
                                if(vkey[i] < 0) vkey[i] = attrib[i].length() - vkey[i];
                                else vkey[i]--;
                                if(!attrib[i].inrange(vkey[i])) vkey[i] = -1;
                                if(*c!='/') break;
                                c++;
                            }
                            int *index = verthash.access(vkey);
                            if(!index)
                            {
                                index = &verthash[vkey];
                                *index = verts.length();
                                vert &v = verts.add();
                                v.pos = vkey.x < 0 ? vec(0, 0, 0) : attrib[0][vkey.x];
                                v.pos = vec(v.pos.z, -v.pos.x, v.pos.y);
                                v.norm = vkey.z < 0 ? vec(0, 0, 0) : attrib[2][vkey.z];
                                v.norm = vec(v.norm.z, -v.norm.x, v.norm.y);
                                tcvert &tcv = tcverts.add();
                                if(vkey.y < 0) tcv.u = tcv.v = 0;
                                else { tcv.u = attrib[1][vkey.y].x; tcv.v = 1-attrib[1][vkey.y].y; }
                            }
                            if(v0 < 0) v0 = *index;
                            else if(v1 < 0) v1 = *index;
                            else
                            {
                                tri &t = tris.add();
                                t.vert[0] = ushort(*index);
                                t.vert[1] = ushort(v1);
                                t.vert[2] = ushort(v0);
                                v1 = *index;
                            }
                        }
                        break;
                    }
                }
            }

            if(curmesh) FLUSHMESH;

            delete file;

            return true;
        }
    };

    meshgroup *loadmeshes(char *name, va_list args)
    {
        objmeshgroup *group = new objmeshgroup;
        if(!group->load(name)) { delete group; return NULL; }
        return group;
    }

    bool loaddefaultparts()
    {
        part &mdl = *new part;
        parts.add(&mdl);
        mdl.model = this;
        mdl.index = 0;
        const char *pname = parentdir(loadname);
        defformatstring(name1)("packages/models/%s/tris.obj", loadname);
        mdl.meshes = sharemeshes(path(name1));
        if(!mdl.meshes)
        {
            defformatstring(name2)("packages/models/%s/tris.obj", pname);    // try obj in parent folder (vert sharing)
            mdl.meshes = sharemeshes(path(name2));
            if(!mdl.meshes) return false;
        }
        Texture *tex, *masks;
        loadskin(loadname, pname, tex, masks);
        mdl.initskins(tex, masks);
        if(tex==notexture) conoutf("could not load model skin for %s", name1);
        return true;
    }

    bool load()
    { 
        if(loaded) return true;
        formatstring(objdir)("packages/models/%s", loadname);
        defformatstring(cfgname)("packages/models/%s/obj.js", loadname); // INTENSITY

        loadingobj = this;
        persistidents = false;
        if(ScriptEngineManager::runFile(path(cfgname), false) && parts.length()) // INTENSITY configured obj, will call the obj* commands below
        {
            persistidents = true;
            loadingobj = NULL;
            loopv(parts) if(!parts[i]->meshes) return false;
        }
        else // obj without configuration, try default tris and skin
        {
            persistidents = true;
            loadingobj = NULL;
            if(!loaddefaultparts()) return false;
        }
        scale /= 4;
        translate.y = -translate.y;
        parts[0]->translate = translate;
        loopv(parts) parts[i]->meshes->shared++;
        preloadshaders();
        return loaded = true;
    }
};

void objload(char *model)
{
    if(!loadingobj) { conoutf("not loading an obj"); return; }
    defformatstring(filename)("%s/%s", objdir, model);
    obj::part &mdl = *new obj::part;
    loadingobj->parts.add(&mdl);
    mdl.model = loadingobj;
    mdl.index = loadingobj->parts.length()-1;
    if(mdl.index) mdl.pitchscale = mdl.pitchoffset = mdl.pitchmin = mdl.pitchmax = 0;
    mdl.meshes = loadingobj->sharemeshes(path(filename));
    if(!mdl.meshes) conoutf("could not load %s", filename); // ignore failure
    else mdl.initskins();
}

void objpitch(float *pitchscale, float *pitchoffset, float *pitchmin, float *pitchmax)
{
    if(!loadingobj || loadingobj->parts.empty()) { conoutf("not loading an obj"); return; }
    obj::part &mdl = *loadingobj->parts.last();

    mdl.pitchscale = *pitchscale;
    mdl.pitchoffset = *pitchoffset;
    if(*pitchmin || *pitchmax)
    {
        mdl.pitchmin = *pitchmin;
        mdl.pitchmax = *pitchmax;
    }
    else
    {
        mdl.pitchmin = -360*mdl.pitchscale;
        mdl.pitchmax = 360*mdl.pitchscale;
    }
}

#define loopobjmeshes(meshname, m, body) \
    if(!loadingobj || loadingobj->parts.empty()) { conoutf("not loading an obj"); return; } \
    obj::part &mdl = *loadingobj->parts.last(); \
    if(!mdl.meshes) return; \
    loopv(mdl.meshes->meshes) \
    { \
        obj::vertmesh &m = *(obj::vertmesh *)mdl.meshes->meshes[i]; \
        if(!strcmp(meshname, "*") || (m.name && !strcmp(m.name, meshname))) \
        { \
            body; \
        } \
    }

#define loopobjskins(meshname, s, body) loopobjmeshes(meshname, m, { obj::skin &s = mdl.skins[i]; body; })

void objskin(char *meshname, char *tex, char *masks, float *envmapmax, float *envmapmin)
{
    loopobjskins(meshname, s,
        s.tex = textureload(makerelpath(objdir, tex), 0, true, false);
        if(*masks)
        {
            s.masks = textureload(makerelpath(objdir, masks, NULL, "<ffmask:25>"), 0, true, false);
            s.envmapmax = *envmapmax;
            s.envmapmin = *envmapmin;
        }
    );
}

void objspec(char *meshname, int *percent)
{
    float spec = 1.0f;
    if(*percent>0) spec = *percent/100.0f;
    else if(*percent<0) spec = 0.0f;
    loopobjskins(meshname, s, s.spec = spec);
}

void objambient(char *meshname, int *percent)
{
    float ambient = 0.3f;
    if(*percent>0) ambient = *percent/100.0f;
    else if(*percent<0) ambient = 0.0f;
    loopobjskins(meshname, s, s.ambient = ambient);
}

void objglow(char *meshname, int *percent)
{
    float glow = 3.0f;
    if(*percent>0) glow = *percent/100.0f;
    else if(*percent<0) glow = 0.0f;
    loopobjskins(meshname, s, s.glow = glow);
}

void objglare(char *meshname, float *specglare, float *glowglare)
{
    loopobjskins(meshname, s, { s.specglare = *specglare; s.glowglare = *glowglare; });
}

void objalphatest(char *meshname, float *cutoff)
{
    loopobjskins(meshname, s, s.alphatest = max(0.0f, min(1.0f, *cutoff)));
}

void objalphablend(char *meshname, int *blend)
{
    loopobjskins(meshname, s, s.alphablend = *blend!=0);
}

void objcullface(char *meshname, int *cullface)
{
    loopobjskins(meshname, s, s.cullface = *cullface!=0);
}

void objenvmap(char *meshname, char *envmap)
{
    Texture *tex = cubemapload(envmap);
    loopobjskins(meshname, s, s.envmap = tex);
}

void objbumpmap(char *meshname, char *normalmap, char *skin)
{
    Texture *normalmaptex = NULL, *skintex = NULL;
    normalmaptex = textureload(makerelpath(objdir, normalmap, "<noff>"), 0, true, false);
    if(skin[0]) skintex = textureload(makerelpath(objdir, skin, "<noff>"), 0, true, false);
    loopobjskins(meshname, s, { s.unlittex = skintex; s.normalmap = normalmaptex; m.calctangents(); });
}

void objfullbright(char *meshname, float *fullbright)
{
    loopobjskins(meshname, s, s.fullbright = *fullbright);
}

void objshader(char *meshname, char *shader)
{
    loopobjskins(meshname, s, s.shader = lookupshaderbyname(shader));
}

void objscroll(char *meshname, float *scrollu, float *scrollv)
{
    loopobjskins(meshname, s, { s.scrollu = *scrollu; s.scrollv = *scrollv; });
}

void objnoclip(char *meshname, int *noclip)
{
    loopobjmeshes(meshname, m, m.noclip = *noclip!=0);
}

COMMAND(objload, "s");
COMMAND(objpitch, "ffff");
COMMAND(objskin, "sssff");
COMMAND(objspec, "si");
COMMAND(objambient, "si");
COMMAND(objglow, "si");
COMMAND(objglare, "sff");
COMMAND(objalphatest, "sf");
COMMAND(objalphablend, "si");
COMMAND(objcullface, "si");
COMMAND(objenvmap, "ss");
COMMAND(objbumpmap, "sss");
COMMAND(objfullbright, "sf");
COMMAND(objshader, "ss");
COMMAND(objscroll, "sff");
COMMAND(objnoclip, "si");

