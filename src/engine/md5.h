struct md5;

md5 *loadingmd5 = NULL;

string md5dir;

struct md5joint
{
    vec pos;
    quat orient;
};

struct md5weight
{
    int joint;
    float bias;
    vec pos;
};  

struct md5vert
{
    float u, v;
    ushort start, count;
};

struct md5hierarchy
{
    string name;
    int parent, flags, start;
};

struct md5adjustment
{
    float yaw, pitch, roll;
    vec translate;

    md5adjustment(float yaw, float pitch, float roll, const vec &translate) : yaw(yaw), pitch(pitch), roll(roll), translate(translate) {}
};

vector<md5adjustment> md5adjustments;

struct md5 : skelmodel
{
    md5(const char *name) : skelmodel(name) {}

    int type() const { return MDL_MD5; }

    struct md5mesh : skelmesh
    {
        md5weight *weightinfo;
        int numweights;
        md5vert *vertinfo;

        md5mesh() : weightinfo(NULL), numweights(0), vertinfo(NULL)
        {
        }

        ~md5mesh()
        {
            cleanup();
        }

        void cleanup()
        {
            DELETEA(weightinfo);
            DELETEA(vertinfo);
        }

        void buildverts(vector<md5joint> &joints)
        {
            loopi(numverts)
            {
                md5vert &v = vertinfo[i];
                vec pos(0, 0, 0);
                loopk(v.count)
                {
                    md5weight &w = weightinfo[v.start+k];
                    md5joint &j = joints[w.joint];
                    pos.add(j.orient.rotate(w.pos).add(j.pos).mul(w.bias));
                }
                vert &vv = verts[i];
                vv.pos = pos;
                vv.u = v.u;
                vv.v = v.v;

                blendcombo c;
                int sorted = 0;
                loopj(v.count)
                {
                    md5weight &w = weightinfo[v.start+j];
                    sorted = c.addweight(sorted, w.bias, w.joint); 
                }
                c.finalize(sorted);
                vv.blend = addblendcombo(c);
            }
        }

        void load(stream *f, char *buf, size_t bufsize)
        {
            md5weight w;
            md5vert v;
            tri t;
            int index;

            while(f->getline(buf, bufsize) && buf[0]!='}')
            {
                if(strstr(buf, "// meshes:"))
                {
                    char *start = strchr(buf, ':')+1;
                    if(*start==' ') start++; 
                    char *end = start + strlen(start)-1;
                    while(end >= start && isspace(*end)) end--;
                    name = newstring(start, end+1-start);
                }
                else if(strstr(buf, "shader"))
                {
                    char *start = strchr(buf, '"'), *end = start ? strchr(start+1, '"') : NULL;
                    if(start && end) 
                    {
                        char *texname = newstring(start+1, end-(start+1));
                        part *p = loadingmd5->parts.last();
                        p->initskins(notexture, notexture, group->meshes.length());
                        skin &s = p->skins.last();
                        s.tex = textureload(makerelpath(md5dir, texname), 0, true, false);
                        delete[] texname;
                    }
                }
                else if(sscanf(buf, " numverts %d", &numverts)==1)
                {
                    numverts = max(numverts, 0);        
                    if(numverts)
                    {
                        vertinfo = new md5vert[numverts];
                        verts = new vert[numverts];
                    }
                }
                else if(sscanf(buf, " numtris %d", &numtris)==1)
                {
                    numtris = max(numtris, 0);
                    if(numtris) tris = new tri[numtris];
                }
                else if(sscanf(buf, " numweights %d", &numweights)==1)
                {
                    numweights = max(numweights, 0);
                    if(numweights) weightinfo = new md5weight[numweights];
                }
                else if(sscanf(buf, " vert %d ( %f %f ) %hu %hu", &index, &v.u, &v.v, &v.start, &v.count)==5)
                {
                    if(index>=0 && index<numverts) vertinfo[index] = v;
                }
                else if(sscanf(buf, " tri %d %hu %hu %hu", &index, &t.vert[0], &t.vert[1], &t.vert[2])==4)
                {
                    if(index>=0 && index<numtris) tris[index] = t;
                }
                else if(sscanf(buf, " weight %d %d %f ( %f %f %f ) ", &index, &w.joint, &w.bias, &w.pos.x, &w.pos.y, &w.pos.z)==6)
                {
                    w.pos.y = -w.pos.y;
                    if(index>=0 && index<numweights) weightinfo[index] = w;
                }
            }
        }
    };

    struct md5meshgroup : skelmeshgroup
    {
        md5meshgroup() 
        {
        }

        bool loadmd5mesh(const char *filename, float smooth)
        {
            stream *f = openfile(filename, "r");
            if(!f) return false;

            char buf[512];
            vector<md5joint> basejoints;
            while(f->getline(buf, sizeof(buf)))
            {
                int tmp;
                if(sscanf(buf, " MD5Version %d", &tmp)==1)
                {
                    if(tmp!=10) { delete f; return false; }
                }
                else if(sscanf(buf, " numJoints %d", &tmp)==1)
                {
                    if(tmp<1) { delete f; return false; }
                    if(skel->numbones>0) continue;
                    skel->numbones = tmp;
                    skel->bones = new boneinfo[skel->numbones];
                }
                else if(sscanf(buf, " numMeshes %d", &tmp)==1)
                {
                    if(tmp<1) { delete f; return false; }
                }
                else if(strstr(buf, "joints {"))
                {
                    string name;
                    int parent;
                    md5joint j;
                    while(f->getline(buf, sizeof(buf)) && buf[0]!='}')
                    {
                        if(sscanf(buf, " %s %d ( %f %f %f ) ( %f %f %f )",
                            name, &parent, &j.pos.x, &j.pos.y, &j.pos.z,
                            &j.orient.x, &j.orient.y, &j.orient.z)==8)
                        {
                            j.pos.y = -j.pos.y;
                            j.orient.x = -j.orient.x;
                            j.orient.z = -j.orient.z;
                            if(basejoints.length()<skel->numbones) 
                            {
                                if(!skel->bones[basejoints.length()].name) 
                                {
                                    char *start = strchr(name, '"'), *end = start ? strchr(start+1, '"') : NULL;
                                    skel->bones[basejoints.length()].name = start && end ? newstring(start+1, end-(start+1)) : newstring(name);
                                }
                                skel->bones[basejoints.length()].parent = parent;
                            }
                            j.orient.restorew();
                            basejoints.add(j);
                        }
                    }
                    if(basejoints.length()!=skel->numbones) { delete f; return false; }
                }
                else if(strstr(buf, "mesh {"))
                {
                    md5mesh *m = new md5mesh;
                    m->group = this;
                    meshes.add(m);
                    m->load(f, buf, sizeof(buf));
                    if(!m->numtris || !m->numverts)
                    {
                        conoutf("empty mesh in %s", filename);
                        meshes.removeobj(m);
                        delete m;
                    }
                }
            }
        
            if(skel->shared <= 1) 
            {
                skel->linkchildren();
                loopv(basejoints) skel->bones[i].base = dualquat(basejoints[i].orient, basejoints[i].pos);
            }

            loopv(meshes)
            {
                md5mesh &m = *(md5mesh *)meshes[i];
                m.buildverts(basejoints);
                if(smooth <= 1) m.smoothnorms(smooth);
                else m.buildnorms();
                m.cleanup();
            }
            
            sortblendcombos();

            delete f;
            return true;
        }

        skelanimspec *loadmd5anim(const char *filename)
        {
            skelanimspec *sa = skel->findskelanim(filename);
            if(sa) return sa;

            stream *f = openfile(filename, "r");
            if(!f) return NULL;

            vector<md5hierarchy> hierarchy;
            vector<md5joint> basejoints;
            int animdatalen = 0, animframes = 0;
            float *animdata = NULL;
            dualquat *animbones = NULL;
            char buf[512];
            while(f->getline(buf, sizeof(buf)))
            {
                int tmp;
                if(sscanf(buf, " MD5Version %d", &tmp)==1)
                {
                    if(tmp!=10) { delete f; return NULL; }
                }
                else if(sscanf(buf, " numJoints %d", &tmp)==1)
                {
                    if(tmp!=skel->numbones) { delete f; return NULL; }
                }
                else if(sscanf(buf, " numFrames %d", &animframes)==1)
                {
                    if(animframes<1) { delete f; return NULL; }
                }
                else if(sscanf(buf, " frameRate %d", &tmp)==1);
                else if(sscanf(buf, " numAnimatedComponents %d", &animdatalen)==1)
                {
                    if(animdatalen>0) animdata = new float[animdatalen];
                }
                else if(strstr(buf, "bounds {"))
                {
                    while(f->getline(buf, sizeof(buf)) && buf[0]!='}');
                }
                else if(strstr(buf, "hierarchy {"))
                {
                    while(f->getline(buf, sizeof(buf)) && buf[0]!='}')
                    {
                        md5hierarchy h;
                        if(sscanf(buf, " %s %d %d %d", h.name, &h.parent, &h.flags, &h.start)==4)
                            hierarchy.add(h);
                    }
                }
                else if(strstr(buf, "baseframe {"))
                {
                    while(f->getline(buf, sizeof(buf)) && buf[0]!='}')
                    {
                        md5joint j;
                        if(sscanf(buf, " ( %f %f %f ) ( %f %f %f )", &j.pos.x, &j.pos.y, &j.pos.z, &j.orient.x, &j.orient.y, &j.orient.z)==6)
                        {
                            j.pos.y = -j.pos.y;
                            j.orient.x = -j.orient.x;
                            j.orient.z = -j.orient.z;
                            j.orient.restorew();
                            basejoints.add(j);
                        }
                    }
                    if(basejoints.length()!=skel->numbones) { delete f; return NULL; }
                    animbones = new dualquat[(skel->numframes+animframes)*skel->numbones];
                    if(skel->bones)
                    {
                        memcpy(animbones, skel->framebones, skel->numframes*skel->numbones*sizeof(dualquat));
                        delete[] skel->framebones;
                    }
                    skel->framebones = animbones;
                    animbones += skel->numframes*skel->numbones;

                    sa = &skel->addskelanim(filename);
                    sa->frame = skel->numframes;
                    sa->range = animframes;

                    skel->numframes += animframes;
                }
                else if(sscanf(buf, " frame %d", &tmp)==1)
                {
                    for(int numdata = 0; f->getline(buf, sizeof(buf)) && buf[0]!='}';)
                    {
                        for(char *src = buf, *next = src; numdata < animdatalen; numdata++, src = next)
                        {
                            animdata[numdata] = strtod(src, &next);
                            if(next <= src) break;
                        }
                    }
                    dualquat *frame = &animbones[tmp*skel->numbones];
                    loopv(basejoints)
                    {
                        md5hierarchy &h = hierarchy[i];
                        md5joint j = basejoints[i];
                        if(h.start < animdatalen && h.flags)
                        {
                            float *jdata = &animdata[h.start];
                            if(h.flags&1) j.pos.x = *jdata++;
                            if(h.flags&2) j.pos.y = -*jdata++;
                            if(h.flags&4) j.pos.z = *jdata++;
                            if(h.flags&8) j.orient.x = -*jdata++;
                            if(h.flags&16) j.orient.y = *jdata++;
                            if(h.flags&32) j.orient.z = -*jdata++;
                            j.orient.restorew();
                            /*if(memcmp(&j, &basejoints[i], sizeof(j))) usedjoints[i] = 1; */
                        }
                        frame[i] = dualquat(j.orient, j.pos);
                        frame[i].fixantipodal(skel->framebones[i]);
#if 0
                        if(h.parent<0) frame[i] = dualquat(j.orient, j.pos); 
                        else (frame[i] = frame[h.parent]).mul(dualquat(j.orient, j.pos));
#endif
                    }
                    loopv(md5adjustments)
                    {
                        if(md5adjustments[i].yaw) frame[i].mulorient(quat(vec(0, 0, 1), md5adjustments[i].yaw*RAD));
                        if(md5adjustments[i].pitch) frame[i].mulorient(quat(vec(0, -1, 0), md5adjustments[i].pitch*RAD));
                        if(md5adjustments[i].roll) frame[i].mulorient(quat(vec(-1, 0, 0), md5adjustments[i].roll*RAD));
                        if(!md5adjustments[i].translate.iszero()) frame[i].translate(md5adjustments[i].translate);
                    }
                }    
            }

            DELETEA(animdata);
            delete f;

#if 0
            vector<dualquat> invbase;
            loopi(numbones) 
            {
                dualquat &d = invbase.add(basebones[i]);
#if 1
                d.translate(vec(translate).neg());
                d.scale(1.0f/scale);
#endif
                d.invert();
            }
            loopi(animframes) 
            {
                dualquat *frame = &animbones[i*numbones];
                loopj(numbones) 
                {
                    dualquat &d = frame[j];
#if 1
                    d.mul(invbase[j]);
#endif
                    d.scale(scale);
                    d.translate(translate);
#if 0
                    d.mul(invbase[j]);
#endif
                }
            }
#endif

            return sa;
        }

        bool load(const char *meshfile, float smooth)
        {
            name = newstring(meshfile);

            if(!loadmd5mesh(meshfile, smooth)) return false;
            
            return true;
        }
    };            

    meshgroup *loadmeshes(char *name, va_list args)
    {
        md5meshgroup *group = new md5meshgroup;
        group->shareskeleton(va_arg(args, char *));
        if(!group->load(name, va_arg(args, double))) { delete group; return NULL; }
        return group;
    }

    bool loaddefaultparts()
    {
        skelpart &mdl = *new skelpart;
        parts.add(&mdl);
        mdl.model = this;
        mdl.index = 0;
        mdl.pitchscale = mdl.pitchoffset = mdl.pitchmin = mdl.pitchmax = 0;
        md5adjustments.setsizenodelete(0);
        const char *fname = loadname + strlen(loadname);
        do --fname; while(fname >= loadname && *fname!='/' && *fname!='\\');
        fname++;
        defformatstring(meshname)("packages/models/%s/%s.md5mesh", loadname, fname);
        mdl.meshes = sharemeshes(path(meshname), NULL, 0);
        if(!mdl.meshes) return false;
        mdl.initanimparts();
        mdl.initskins();
        defformatstring(animname)("packages/models/%s/%s.md5anim", loadname, fname);
        ((md5meshgroup *)mdl.meshes)->loadmd5anim(path(animname));
        return true;
    }

    bool load()
    {
        if(loaded) return true;
        formatstring(md5dir)("packages/models/%s", loadname);
        defformatstring(cfgname)("packages/models/%s/md5.js", loadname); // INTENSITY

        loadingmd5 = this;
        persistidents = false;
        if(ScriptEngineManager::runFile(path(cfgname), false) && parts.length()) // INTENSITY: execfile(cfgname, false) && parts.length()) // configured md5, will call the md5* commands below
        {
            persistidents = true;
            loadingmd5 = NULL;
            loopv(parts) if(!parts[i]->meshes) return false;
        }
        else // md5 without configuration, try default tris and skin 
        {
            persistidents = true;
            if(!loaddefaultparts()) 
            {
                loadingmd5 = NULL;
                return false;
            }
            loadingmd5 = NULL;
        }
        scale /= 4;
        parts[0]->translate = translate;
        loopv(parts) 
        {
            skelpart *p = (skelpart *)parts[i];
            p->endanimparts();
            p->meshes->shared++;
        }
        preloadshaders();
        return loaded = true;
    }
};

void setmd5dir(char *name)
{
    if(!loadingmd5) { conoutf("not loading an md5"); return; }
    formatstring(md5dir)("packages/models/%s", name);
}
    
void md5load(char *meshfile, char *skelname, float *smooth)
{
    if(!loadingmd5) { conoutf("not loading an md5"); return; }
    defformatstring(filename)("%s/%s", md5dir, meshfile);
    md5::skelpart &mdl = *new md5::skelpart;
    loadingmd5->parts.add(&mdl);
    mdl.model = loadingmd5;
    mdl.index = loadingmd5->parts.length()-1;
    mdl.pitchscale = mdl.pitchoffset = mdl.pitchmin = mdl.pitchmax = 0;
    md5adjustments.setsizenodelete(0);
    mdl.meshes = loadingmd5->sharemeshes(path(filename), skelname[0] ? skelname : NULL, double(*smooth > 0 ? cos(clamp(*smooth, 0.0f, 90.0f)*RAD) : 2));
    if(!mdl.meshes) conoutf("could not load %s", filename); // ignore failure
    else 
    {
        mdl.initanimparts();
        mdl.initskins();
    }
}

void md5tag(char *name, char *tagname)
{
    if(!loadingmd5 || loadingmd5->parts.empty()) { conoutf("not loading an md5"); return; }
    md5::part &mdl = *loadingmd5->parts.last();
    int i = mdl.meshes ? ((md5::skelmeshgroup *)mdl.meshes)->skel->findbone(name) : -1;
    if(i >= 0)
    {
        ((md5::skelmeshgroup *)mdl.meshes)->skel->addtag(tagname, i);
        return;
    }
    conoutf("could not find bone %s for tag %s", name, tagname);
}
        
void md5pitch(char *name, float *pitchscale, float *pitchoffset, float *pitchmin, float *pitchmax)
{
    if(!loadingmd5 || loadingmd5->parts.empty()) { conoutf("not loading an md5"); return; }
    md5::part &mdl = *loadingmd5->parts.last();

    if(name[0])
    {
        int i = mdl.meshes ? ((md5::skelmeshgroup *)mdl.meshes)->skel->findbone(name) : -1;
        if(i>=0)
        {
            md5::boneinfo &b = ((md5::skelmeshgroup *)mdl.meshes)->skel->bones[i];
            b.pitchscale = *pitchscale;
            b.pitchoffset = *pitchoffset;
            if(*pitchmin || *pitchmax)
            {
                b.pitchmin = *pitchmin;
                b.pitchmax = *pitchmax;
            }
            else
            {
                b.pitchmin = -360*b.pitchscale;
                b.pitchmax = 360*b.pitchscale;
            }
            return;
        }
        conoutf("could not find bone %s to pitch", name);
        return;
    }

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

void md5adjust(char *name, float *yaw, float *pitch, float *roll, float *tx, float *ty, float *tz)
{
    if(!loadingmd5 || loadingmd5->parts.empty()) { conoutf("not loading an md5"); return; }
    md5::part &mdl = *loadingmd5->parts.last();

    if(!name[0]) return;
    int i = mdl.meshes ? ((md5::skelmeshgroup *)mdl.meshes)->skel->findbone(name) : -1;
    if(i < 0) {  conoutf("could not find bone %s to adjust", name); return; }
    while(!md5adjustments.inrange(i)) md5adjustments.add(md5adjustment(0, 0, 0, vec(0, 0, 0)));
    md5adjustments[i] = md5adjustment(*yaw, *pitch, *roll, vec(*tx/4, *ty/4, *tz/4));
}
 
#define loopmd5meshes(meshname, m, body) \
    if(!loadingmd5 || loadingmd5->parts.empty()) { conoutf("not loading an md5"); return; } \
    md5::part &mdl = *loadingmd5->parts.last(); \
    if(!mdl.meshes) return; \
    loopv(mdl.meshes->meshes) \
    { \
        md5::skelmesh &m = *(md5::skelmesh *)mdl.meshes->meshes[i]; \
        if(!strcmp(meshname, "*") || (m.name && !strcmp(m.name, meshname))) \
        { \
            body; \
        } \
    }

#define loopmd5skins(meshname, s, body) loopmd5meshes(meshname, m, { md5::skin &s = mdl.skins[i]; body; })

void md5skin(char *meshname, char *tex, char *masks, float *envmapmax, float *envmapmin)
{
    loopmd5skins(meshname, s,
        s.tex = textureload(makerelpath(md5dir, tex), 0, true, false);
        if(*masks)
        {
            s.masks = textureload(makerelpath(md5dir, masks, NULL, "<ffmask:25>"), 0, true, false);
            s.envmapmax = *envmapmax;
            s.envmapmin = *envmapmin;
        }
    );
}

void md5spec(char *meshname, int *percent)
{
    float spec = 1.0f;
    if(*percent>0) spec = *percent/100.0f;
    else if(*percent<0) spec = 0.0f;
    loopmd5skins(meshname, s, s.spec = spec);
}

void md5ambient(char *meshname, int *percent)
{
    float ambient = 0.3f;
    if(*percent>0) ambient = *percent/100.0f;
    else if(*percent<0) ambient = 0.0f;
    loopmd5skins(meshname, s, s.ambient = ambient);
}

void md5glow(char *meshname, int *percent)
{
    float glow = 3.0f;
    if(*percent>0) glow = *percent/100.0f;
    else if(*percent<0) glow = 0.0f;
    loopmd5skins(meshname, s, s.glow = glow);
}

void md5glare(char *meshname, float *specglare, float *glowglare)
{
    loopmd5skins(meshname, s, { s.specglare = *specglare; s.glowglare = *glowglare; });
}

void md5alphatest(char *meshname, float *cutoff)
{
    loopmd5skins(meshname, s, s.alphatest = max(0.0f, min(1.0f, *cutoff)));
}

void md5alphablend(char *meshname, int *blend)
{
    loopmd5skins(meshname, s, s.alphablend = *blend!=0);
}

void md5cullface(char *meshname, int *cullface)
{
    loopmd5skins(meshname, s, s.cullface = *cullface!=0);
}

void md5envmap(char *meshname, char *envmap)
{
    Texture *tex = cubemapload(envmap);
    loopmd5skins(meshname, s, s.envmap = tex);
}

void md5bumpmap(char *meshname, char *normalmap, char *skin)
{
    Texture *normalmaptex = NULL, *skintex = NULL;
    normalmaptex = textureload(makerelpath(md5dir, normalmap, "<noff>"), 0, true, false);
    if(skin[0]) skintex = textureload(makerelpath(md5dir, skin, "<noff>"), 0, true, false);
    loopmd5skins(meshname, s, { s.unlittex = skintex; s.normalmap = normalmaptex; m.calctangents(); });
}

void md5fullbright(char *meshname, float *fullbright)
{
    loopmd5skins(meshname, s, s.fullbright = *fullbright);
}

void md5shader(char *meshname, char *shader)
{
    loopmd5skins(meshname, s, s.shader = lookupshaderbyname(shader));
}

void md5scroll(char *meshname, float *scrollu, float *scrollv)
{
    loopmd5skins(meshname, s, { s.scrollu = *scrollu; s.scrollv = *scrollv; });
}

void md5anim(char *anim, char *animfile, float *speed, int *priority)
{
    if(!loadingmd5 || loadingmd5->parts.empty()) { conoutf("not loading an md5"); return; }

    vector<int> anims;
    findanims(anim, anims);
    if(anims.empty()) conoutf("could not find animation %s", anim);
    else 
    {
        md5::part *p = loadingmd5->parts.last();
        if(!p->meshes) return;
        defformatstring(filename)("%s/%s", md5dir, animfile);
        md5::skelanimspec *sa = ((md5::md5meshgroup *)p->meshes)->loadmd5anim(path(filename));
        if(!sa) conoutf("could not load md5anim file %s", filename);
        else loopv(anims)
        {
            loadingmd5->parts.last()->setanim(p->numanimparts-1, anims[i], sa->frame, sa->range, *speed, *priority);
        }
    }
}

void md5animpart(char *maskstr)
{
    if(!loadingmd5 || loadingmd5->parts.empty()) { conoutf("not loading an md5"); return; }

    md5::skelpart *p = (md5::skelpart *)loadingmd5->parts.last();

    vector<char *> bonestrs;
    explodelist(maskstr, bonestrs);
    vector<ushort> bonemask;
    loopv(bonestrs)
    {
        char *bonestr = bonestrs[i];
        int bone = p->meshes ? ((md5::skelmeshgroup *)p->meshes)->skel->findbone(bonestr[0]=='!' ? bonestr+1 : bonestr) : -1;
        if(bone<0) { conoutf("could not find bone %s for anim part mask [%s]", bonestr, maskstr); bonestrs.deletecontentsa(); return; }
        bonemask.add(bone | (bonestr[0]=='!' ? BONEMASK_NOT : 0));
    }
    bonestrs.deletecontentsa();
    bonemask.sort(bonemaskcmp);
    if(bonemask.length()) bonemask.add(BONEMASK_END);

    if(!p->addanimpart(bonemask.getbuf())) conoutf("too many animation parts");
}

void md5link(int *parent, int *child, char *tagname)
{
    if(!loadingmd5) { conoutf("not loading an md5"); return; }
    if(!loadingmd5->parts.inrange(*parent) || !loadingmd5->parts.inrange(*child)) { conoutf("no models loaded to link"); return; }
    if(!loadingmd5->parts[*parent]->link(loadingmd5->parts[*child], tagname)) conoutf("could not link model %s", loadingmd5->loadname);
}

void md5noclip(char *meshname, int *noclip)
{
    loopmd5meshes(meshname, m, m.noclip = *noclip!=0);
}

COMMANDN(md5dir, setmd5dir, "s");
COMMAND(md5load, "ssf");
COMMAND(md5tag, "ss");
COMMAND(md5pitch, "sffff");
COMMAND(md5adjust, "sffffff");
COMMAND(md5skin, "sssff");
COMMAND(md5spec, "si");
COMMAND(md5ambient, "si");
COMMAND(md5glow, "si");
COMMAND(md5glare, "sff");
COMMAND(md5alphatest, "sf");
COMMAND(md5alphablend, "si");
COMMAND(md5cullface, "si");
COMMAND(md5envmap, "ss");
COMMAND(md5bumpmap, "sss");
COMMAND(md5fullbright, "sf");
COMMAND(md5shader, "ss");
COMMAND(md5scroll, "sff");
COMMAND(md5animpart, "s");
COMMAND(md5anim, "ssfi");
COMMAND(md5link, "iis");
COMMAND(md5noclip, "si");            
