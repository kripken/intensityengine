VARFP(lightmodels, 0, 1, 1, preloadmodelshaders());
VARFP(envmapmodels, 0, 1, 1, preloadmodelshaders());
VARFP(glowmodels, 0, 1, 1, preloadmodelshaders());
VARFP(bumpmodels, 0, 1, 1, preloadmodelshaders());
VARP(fullbrightmodels, 0, 0, 200);

struct animmodel : model
{
    struct animspec
    {
        int frame, range;
        float speed;
        int priority;
    };

    struct animpos
    {
        int fr1, fr2;
        float t;

        void setframes(const animinfo &info)
        {
            if(info.range<=1) 
            {
                fr1 = 0;
                t = 0;
            }
            else
            {
                int time = info.anim&ANIM_SETTIME ? info.basetime : lastmillis-info.basetime;
                fr1 = (int)(time/info.speed); // round to full frames
                t = (time-fr1*info.speed)/info.speed; // progress of the frame, value from 0.0f to 1.0f
            }
            if(info.anim&ANIM_LOOP)
            {
                fr1 = fr1%info.range+info.frame;
                fr2 = fr1+1;
                if(fr2>=info.frame+info.range) fr2 = info.frame;
            }
            else
            {
                fr1 = min(fr1, info.range-1)+info.frame;
                fr2 = min(fr1+1, info.frame+info.range-1);
            }
            if(info.anim&ANIM_REVERSE)
            {
                fr1 = (info.frame+info.range-1)-(fr1-info.frame);
                fr2 = (info.frame+info.range-1)-(fr2-info.frame);
            }

            assert(fr1 >= 0); // INTENSITY: Guard against an old bug of ours, see setAnimation in engine_additions.cpp
        }

        bool operator==(const animpos &a) const { return fr1==a.fr1 && fr2==a.fr2 && (fr1==fr2 || t==a.t); }
        bool operator!=(const animpos &a) const { return fr1!=a.fr1 || fr2!=a.fr2 || (fr1!=fr2 && t!=a.t); }
    };

    struct part;

    struct animstate
    {
        part *owner;
        int anim;
        animpos cur, prev;
        float interp;

        bool operator==(const animstate &a) const { return cur==a.cur && (interp<1 ? interp==a.interp && prev==a.prev : a.interp>=1); }
        bool operator!=(const animstate &a) const { return cur!=a.cur || (interp<1 ? interp!=a.interp || prev!=a.prev : a.interp<1); }
    };

    struct linkedpart;
    struct mesh;

    struct skin
    {
        part *owner;
        Texture *tex, *masks, *envmap, *unlittex, *normalmap;
        Shader *shader;
        float spec, ambient, glow, specglare, glowglare, fullbright, envmapmin, envmapmax, scrollu, scrollv, alphatest;
        bool alphablend, cullface;

        skin() : owner(0), tex(notexture), masks(notexture), envmap(NULL), unlittex(NULL), normalmap(NULL), shader(NULL), spec(1.0f), ambient(0.3f), glow(3.0f), specglare(1), glowglare(1), fullbright(0), envmapmin(0), envmapmax(0), scrollu(0), scrollv(0), alphatest(0.9f), alphablend(true), cullface(true) {}

        bool multitextured() { return enableglow; }
        bool envmapped() { return hasCM && envmapmax>0 && envmapmodels && (renderpath!=R_FIXEDFUNCTION || maxtmus >= (fogging ? 4 : 3)); }
        bool bumpmapped() { return renderpath!=R_FIXEDFUNCTION && normalmap && bumpmodels; }
        bool normals() { return renderpath!=R_FIXEDFUNCTION || (lightmodels && !fullbright) || envmapped() || bumpmapped(); }
        bool tangents() { return bumpmapped(); }

        void setuptmus(const animstate *as, bool masked)
        {
            if(fullbright)
            {
                if(enablelighting) { glDisable(GL_LIGHTING); enablelighting = false; }
            }
            else if(lightmodels && !enablelighting)
            {
                glEnable(GL_LIGHTING); 
                enablelighting = true;
                if(!enablerescale) { glEnable(hasRN ? GL_RESCALE_NORMAL_EXT : GL_NORMALIZE); enablerescale = true; }
            }
            int needsfog = -1;
            if(fogging)
            {
                needsfog = masked ? 2 : 1;
                if(fogtmu!=needsfog && fogtmu>=0) disablefog(true);
            }
            if(masked!=enableglow) lasttex = lastmasks = NULL;
            float mincolor = as->anim&ANIM_FULLBRIGHT ? fullbrightmodels/100.0f : 0,
                  r = max(lightcolor.x, mincolor), g = max(lightcolor.y, mincolor), b = max(lightcolor.z, mincolor);
            if(masked)
            {
                if(enableoverbright) disableoverbright();
                if(!enableglow) setuptmu(0, "K , C @ T", envmaptmu>=0 && envmapmax>0 ? "Ca * Ta" : NULL);
                int glowscale = glow>2 ? 4 : (glow>1 || mincolor>1 ? 2 : 1);
                float envmap = envmaptmu>=0 && envmapmax>0 ? 0.2f*envmapmax + 0.8f*envmapmin : 1;
                colortmu(0, glow/glowscale, glow/glowscale, glow/glowscale);
                if(fullbright) glColor4f(fullbright/glowscale, fullbright/glowscale, fullbright/glowscale, envmap);
                else if(lightmodels)
                {
                    GLfloat material[4] = { 1.0f/glowscale, 1.0f/glowscale, 1.0f/glowscale, envmap };
                    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, material);
                }
                else glColor4f(r/glowscale, g/glowscale, b/glowscale, envmap);

                glActiveTexture_(GL_TEXTURE1_ARB);
                if(!enableglow || (!enableenvmap && envmaptmu>=0 && envmapmax>0) || transparent<1)
                {
                    if(!enableglow) glEnable(GL_TEXTURE_2D);
                    if(!(envmaptmu>=0 && envmapmax>0) && transparent<1) colortmu(1, 0, 0, 0, transparent);
                    setuptmu(1, "P * T", envmaptmu>=0 && envmapmax>0 ? "= Pa" : (transparent<1 ? "Ta * Ka" : "= Ta"));
                }
                scaletmu(1, glowscale);

                if(envmaptmu>=0 && envmapmax>0 && transparent<1)
                {
                    glActiveTexture_(GL_TEXTURE0_ARB+envmaptmu);
                    colortmu(envmaptmu, 0, 0, 0, transparent);
                }

                if(needsfog<0) glActiveTexture_(GL_TEXTURE0_ARB);

                enableglow = true;
            }
            else
            {
                if(enableglow) disableglow();
                int colorscale = 1;
                if(mincolor>1 && maxtmus>=1)
                {
                    colorscale = 2;
                    if(!enableoverbright) { setuptmu(0, "C * T x 2"); enableoverbright = true; }
                }
                else if(enableoverbright) disableoverbright();
                if(fullbright) glColor4f(fullbright/colorscale, fullbright/colorscale, fullbright, transparent);
                else if(lightmodels)
                {
                    GLfloat material[4] = { 1.0f/colorscale, 1.0f/colorscale, 1.0f/colorscale, transparent };
                    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, material);
                }
                else glColor4f(r/colorscale, g/colorscale, b/colorscale, transparent);
            }
            if(needsfog>=0)
            {
                if(needsfog!=fogtmu)
                {
                    fogtmu = needsfog;
                    glActiveTexture_(GL_TEXTURE0_ARB+fogtmu);
                    glEnable(GL_TEXTURE_1D);
                    glEnable(GL_TEXTURE_GEN_S);
                    glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
                    setuptmu(fogtmu, "K , P @ Ta", masked && envmaptmu>=0 && envmapmax>0 ? "Ka , Pa @ Ta" : "= Pa");
                    colortmu(fogtmu, watercolor[0]/255.0f, watercolor[1]/255.0f, watercolor[2]/255.0f, 0);
                    if(!fogtex) createfogtex();
                    glBindTexture(GL_TEXTURE_1D, fogtex);
                }
                else glActiveTexture_(GL_TEXTURE0_ARB+fogtmu);
                if(!enablefog) { glEnable(GL_TEXTURE_1D); enablefog = true; }
                GLfloat s[4] = { -refractfogplane.x/waterfog, -refractfogplane.y/waterfog, -refractfogplane.z/waterfog, -refractfogplane.offset/waterfog };
                glTexGenfv(GL_S, GL_OBJECT_PLANE, s);
                glActiveTexture_(GL_TEXTURE0_ARB);
            }
            if(lightmodels && !fullbright)
            {
                float ambientk = min(max(ambient, mincolor)*0.75f, 1.0f),
                      diffusek = 1-ambientk;
                GLfloat ambientcol[4] = { r*ambientk, g*ambientk, b*ambientk, 1 },
                        diffusecol[4] = { r*diffusek, g*diffusek, b*diffusek, 1 };
                float ambientmax = max(ambientcol[0], max(ambientcol[1], ambientcol[2])),
                      diffusemax = max(diffusecol[0], max(diffusecol[1], diffusecol[2]));
                if(ambientmax>1e-3f) loopk(3) ambientcol[k] *= min(1.5f, 1.0f/min(ambientmax, 1.0f));
                if(diffusemax>1e-3f) loopk(3) diffusecol[k] *= min(1.5f, 1.0f/min(diffusemax, 1.0f));
                glLightfv(GL_LIGHT0, GL_AMBIENT, ambientcol);
                glLightfv(GL_LIGHT0, GL_DIFFUSE, diffusecol);
            }
        }

        void setshaderparams(mesh *m, const animstate *as, bool masked)
        {
            if(fullbright)
            {
                glColor4f(fullbright/2, fullbright/2, fullbright/2, transparent);
                setenvparamf("lightscale", SHPARAM_VERTEX, 2, 0, 2, glow);
                setenvparamf("lightscale", SHPARAM_PIXEL, 2, 0, 2, glow);
            }
            else
            {
                float mincolor = as->anim&ANIM_FULLBRIGHT ? fullbrightmodels/100.0f : 0,
                      minshade = max(ambient, mincolor);
                glColor4f(max(lightcolor.x, mincolor), 
                          max(lightcolor.y, mincolor),
                          max(lightcolor.z, mincolor),
                          transparent);
                setenvparamf("lightscale", SHPARAM_VERTEX, 2, spec, minshade, glow);
                setenvparamf("lightscale", SHPARAM_PIXEL, 2, spec, minshade, glow);
            }
            setenvparamf("millis", SHPARAM_VERTEX, 5, lastmillis/1000.0f, lastmillis/1000.0f, lastmillis/1000.0f);
            if(envmaptmu>=0 && envmapmax>0) setenvparamf("envmapscale", bumpmapped() ? SHPARAM_PIXEL : SHPARAM_VERTEX, 3, envmapmin-envmapmax, envmapmax);
            if(glaring) setenvparamf("glarescale", SHPARAM_PIXEL, 4, 16*specglare, 4*glowglare);
        }

        Shader *loadshader(bool shouldenvmap, bool masked)
        {
            #define DOMODELSHADER(name, body) \
                do { \
                    static Shader *name##shader = NULL; \
                    if(!name##shader) name##shader = useshaderbyname(#name); \
                    body; \
                } while(0)
            #define LOADMODELSHADER(name) DOMODELSHADER(name, return name##shader)
            #define SETMODELSHADER(m, name) DOMODELSHADER(name, (m)->setshader(name##shader))
            if(shader) return shader;
            else if(bumpmapped())
            {
                if(shouldenvmap)
                {
                    if(lightmodels && !fullbright && (masked || spec>=0.01f)) LOADMODELSHADER(bumpenvmapmodel);
                    else LOADMODELSHADER(bumpenvmapnospecmodel);
                }
                else if(masked && lightmodels && !fullbright) LOADMODELSHADER(bumpmasksmodel);
                else if(masked && glowmodels) LOADMODELSHADER(bumpmasksnospecmodel);
                else if(spec>=0.01f && lightmodels && !fullbright) LOADMODELSHADER(bumpmodel);
                else LOADMODELSHADER(bumpnospecmodel);
            }
            else if(shouldenvmap)
            {
                if(lightmodels && !fullbright && (masked || spec>=0.01f)) LOADMODELSHADER(envmapmodel);
                else LOADMODELSHADER(envmapnospecmodel);
            }
            else if(masked && lightmodels && !fullbright) LOADMODELSHADER(masksmodel);
            else if(masked && glowmodels) LOADMODELSHADER(masksnospecmodel);
            else if(spec>=0.01f && lightmodels && !fullbright) LOADMODELSHADER(stdmodel);
            else LOADMODELSHADER(nospecmodel);
        }

        void preloadshader()
        {
            bool shouldenvmap = envmapped();
            loadshader(shouldenvmap, masks!=notexture && !(masks->type&Texture::STUB) && (lightmodels || glowmodels || shouldenvmap));
        }
 
        void setshader(mesh *m, const animstate *as, bool masked)
        {
            m->setshader(loadshader(envmaptmu>=0 && envmapmax>0, masked));
        }

        void bind(mesh *b, const animstate *as)
        {
            if(!cullface && enablecullface) { glDisable(GL_CULL_FACE); enablecullface = false; }
            else if(cullface && !enablecullface) { glEnable(GL_CULL_FACE); enablecullface = true; }

            if(as->anim&ANIM_NOSKIN)
            {
                if(enablealphatest) { glDisable(GL_ALPHA_TEST); enablealphatest = false; }
                if(enablealphablend) { glDisable(GL_BLEND); enablealphablend = false; }
                if(enableglow) disableglow();
                if(enableenvmap) disableenvmap();
                if(enablelighting) { glDisable(GL_LIGHTING); enablelighting = false; }
                if(enablerescale) { glDisable(hasRN ? GL_RESCALE_NORMAL_EXT : GL_NORMALIZE); enablerescale = false; }
                if(enablefog) disablefog(true);
                if(shadowmapping) SETMODELSHADER(b, shadowmapcaster);
                else /*if(as->anim&ANIM_SHADOW)*/ SETMODELSHADER(b, notexturemodel);
                return;
            }
            Texture *s = bumpmapped() && unlittex ? unlittex : tex, 
                    *m = masks->type&Texture::STUB ? notexture : masks, 
                    *n = bumpmapped() ? normalmap : NULL;
            if((renderpath==R_FIXEDFUNCTION || !lightmodels) &&
               (!glowmodels || (renderpath==R_FIXEDFUNCTION && fogging && maxtmus<=2)) &&
               (!envmapmodels || envmaptmu<0 || envmapmax<=0))
                m = notexture;
            if(renderpath==R_FIXEDFUNCTION) setuptmus(as, m!=notexture);
            else
            {
                setshaderparams(b, as, m!=notexture);
                setshader(b, as, m!=notexture);
            }
            if(s!=lasttex)
            {
                if(enableglow) glActiveTexture_(GL_TEXTURE1_ARB);
                glBindTexture(GL_TEXTURE_2D, s->id);
                if(enableglow) glActiveTexture_(GL_TEXTURE0_ARB);
                lasttex = s;
            }
            if(n && n!=lastnormalmap)
            {
                glActiveTexture_(GL_TEXTURE3_ARB);
                glBindTexture(GL_TEXTURE_2D, n->id);
                glActiveTexture_(GL_TEXTURE0_ARB);
            }
            if(s->bpp==4)
            {
                if(alphablend)
                {
                    if(!enablealphablend && !reflecting && !refracting)
                    {
                        glEnable(GL_BLEND);
                        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                        enablealphablend = true;
                    }
                }
                else if(enablealphablend) { glDisable(GL_BLEND); enablealphablend = false; }
                if(alphatest>0)
                {
                    if(!enablealphatest) { glEnable(GL_ALPHA_TEST); enablealphatest = true; }
                    if(lastalphatest!=alphatest)
                    {
                        glAlphaFunc(GL_GREATER, alphatest);
                        lastalphatest = alphatest;
                    }
                }
                else if(enablealphatest) { glDisable(GL_ALPHA_TEST); enablealphatest = false; }
            }
            else
            {
                if(enablealphatest) { glDisable(GL_ALPHA_TEST); enablealphatest = false; }
                if(enablealphablend && transparent>=1) { glDisable(GL_BLEND); enablealphablend = false; }
            }
            if(m!=lastmasks && m!=notexture)
            {
                if(!enableglow) glActiveTexture_(GL_TEXTURE1_ARB);
                glBindTexture(GL_TEXTURE_2D, m->id);
                if(!enableglow) glActiveTexture_(GL_TEXTURE0_ARB);
                lastmasks = m;
            }
            if((renderpath!=R_FIXEDFUNCTION || m!=notexture) && envmaptmu>=0 && envmapmax>0)
            {
                GLuint emtex = envmap ? envmap->id : closestenvmaptex;
                if(!enableenvmap || lastenvmaptex!=emtex)
                {
                    glActiveTexture_(GL_TEXTURE0_ARB+envmaptmu);
                    if(!enableenvmap)
                    {
                        glEnable(GL_TEXTURE_CUBE_MAP_ARB);
                        if(!lastenvmaptex && renderpath==R_FIXEDFUNCTION)
                        {
                            glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP_ARB);
                            glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP_ARB);
                            glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP_ARB);
                            glEnable(GL_TEXTURE_GEN_S);
                            glEnable(GL_TEXTURE_GEN_T);
                            glEnable(GL_TEXTURE_GEN_R);
                        }
                        enableenvmap = true;
                        if(!enablerescale) { glEnable(hasRN ? GL_RESCALE_NORMAL_EXT : GL_NORMALIZE); enablerescale = true; }
                    }
                    if(lastenvmaptex!=emtex) { glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, emtex); lastenvmaptex = emtex; }
                    glActiveTexture_(GL_TEXTURE0_ARB);
                }
            }
            else if(enableenvmap) disableenvmap();
        }
    };

    struct meshgroup;

    struct mesh
    {
        meshgroup *group;
        char *name;
        bool noclip;

        mesh() : group(NULL), name(NULL), noclip(false)
        {
        }

        virtual ~mesh()
        {
            DELETEA(name);
        }

        virtual void calcbb(int frame, vec &bbmin, vec &bbmax, const matrix3x4 &m) {}
        virtual void gentris(int frame, Texture *tex, vector<BIH::tri> *out, const matrix3x4 &m) {}

        virtual void setshader(Shader *s) 
        { 
            if(glaring) s->setvariant(0, 2);
            else s->set(); 
        }
    };

    struct meshgroup
    {
        meshgroup *next;
        int shared;
        char *name;
        vector<mesh *> meshes;
        float scale;
        vec translate;

        meshgroup() : next(NULL), shared(0), name(NULL), scale(1), translate(0, 0, 0)
        {
        }

        virtual ~meshgroup()
        {
            DELETEA(name);
            meshes.deletecontentsp();
            DELETEP(next);
        }            

        virtual int findtag(const char *name) { return -1; }
        virtual void concattagtransform(part *p, int frame, int i, const matrix3x4 &m, matrix3x4 &n) {}

        void calcbb(int frame, vec &bbmin, vec &bbmax, const matrix3x4 &m)
        {
            loopv(meshes) meshes[i]->calcbb(frame, bbmin, bbmax, m);
        }

        void gentris(int frame, vector<skin> &skins, vector<BIH::tri> *tris, const matrix3x4 &m)
        {
            loopv(meshes) meshes[i]->gentris(frame, skins[i].tex, tris, m);
        }

        virtual int totalframes() const { return 1; }
        bool hasframe(int i) const { return i>=0 && i<totalframes(); }
        bool hasframes(int i, int n) const { return i>=0 && i+n<=totalframes(); }
        int clipframes(int i, int n) const { return min(n, totalframes() - i); }

        virtual void cleanup() {}
        virtual void render(const animstate *as, float pitch, const vec &axis, dynent *d, part *p) {}
    };

    virtual meshgroup *loadmeshes(char *name, va_list args) { return NULL; }

    meshgroup *sharemeshes(char *name, ...)
    {
        static hashtable<char *, meshgroup *> meshgroups;
        if(!meshgroups.access(name))
        {
            va_list args;
            va_start(args, name);
            meshgroup *group = loadmeshes(name, args);
            va_end(args);
            if(!group) return NULL;
            meshgroups[group->name] = group;
        }
        return meshgroups[name];
    }

    struct linkedpart
    {
        part *p;
        int tag, anim, basetime;
        vec *pos;
        glmatrixf matrix;

        linkedpart() : p(NULL), tag(-1), anim(-1), basetime(0), pos(NULL) {}
    };

    struct part
    {
        animmodel *model;
        int index;
        meshgroup *meshes;
        vector<linkedpart> links;
        vector<skin> skins;
        vector<animspec> *anims[MAXANIMPARTS];
        int numanimparts;
        float pitchscale, pitchoffset, pitchmin, pitchmax;
        vec translate;

        part() : meshes(NULL), numanimparts(1), pitchscale(1), pitchoffset(0), pitchmin(0), pitchmax(0), translate(0, 0, 0)
        {
            loopk(MAXANIMPARTS) anims[k] = NULL;
        }
        virtual ~part()
        {
            loopk(MAXANIMPARTS) DELETEA(anims[k]);
        }

        virtual void cleanup()
        {
            if(meshes) meshes->cleanup();
        }

        void calcbb(int frame, vec &bbmin, vec &bbmax, const matrix3x4 &m)
        {
            matrix3x4 t = m;
            t.translate(translate);
            t.scale(model->scale);
            meshes->calcbb(frame, bbmin, bbmax, t);
            loopv(links)
            {
                matrix3x4 n;
                meshes->concattagtransform(this, frame, links[i].tag, m, n);
                links[i].p->calcbb(frame, bbmin, bbmax, n);
            }
        }

        void gentris(int frame, vector<BIH::tri> *tris, const matrix3x4 &m)
        {
            matrix3x4 t = m;
            t.translate(translate);
            t.scale(model->scale);
            meshes->gentris(frame, skins, tris, t);
            loopv(links)
            {
                matrix3x4 n;
                meshes->concattagtransform(this, frame, links[i].tag, m, n);
                links[i].p->gentris(frame, tris, n);
            }
        }

        bool link(part *p, const char *tag, int anim = -1, int basetime = 0, vec *pos = NULL)
        {
            int i = meshes ? meshes->findtag(tag) : -1;
            if(i<0)
            {
                loopv(links) if(links[i].p && links[i].p->link(p, tag, anim, basetime, pos)) return true;
                return false;
            }
            linkedpart &l = links.add();
            l.p = p;
            l.tag = i;
            l.anim = anim;
            l.basetime = basetime;
            l.pos = pos;
            return true;
        }

        bool unlink(part *p)
        {
            loopvrev(links) if(links[i].p==p) { links.remove(i, 1); return true; }
            loopv(links) if(links[i].p && links[i].p->unlink(p)) return true;
            return false;
        }

        void initskins(Texture *tex = notexture, Texture *masks = notexture, int limit = 0)
        {
            if(!limit)
            {
                if(!meshes) return;
                limit = meshes->meshes.length();
            }
            while(skins.length() < limit)
            {
                skin &s = skins.add();
                s.owner = this;
                s.tex = tex;
                s.masks = masks;
            }
        }

        void preloadshaders()
        {
            loopv(skins) skins[i].preloadshader();
        }

        virtual void getdefaultanim(animinfo &info, int anim, uint varseed, dynent *d)
        {
            info.frame = 0;
            info.range = 1;
        }

        bool calcanim(int animpart, int anim, int basetime, int basetime2, dynent *d, int interp, animinfo &info)
        {
            uint varseed = uint((size_t)d);
            info.anim = anim;
            info.basetime = basetime;
            info.varseed = varseed;
            info.speed = anim&ANIM_SETSPEED ? basetime2 : 100.0f;
            if((anim&ANIM_INDEX)==ANIM_ALL)
            {
                info.frame = 0;
                info.range = meshes->totalframes();
            }
            else 
            {
                animspec *spec = NULL;
                if(anims[animpart])
                {
                    vector<animspec> &primary = anims[animpart][anim&ANIM_INDEX];
                    if(primary.length()) spec = &primary[uint(varseed + basetime)%primary.length()];
                    if((anim>>ANIM_SECONDARY)&(ANIM_INDEX|ANIM_DIR))
                    {
                        vector<animspec> &secondary = anims[animpart][(anim>>ANIM_SECONDARY)&ANIM_INDEX];
                        if(secondary.length())
                        {
                            animspec &spec2 = secondary[uint(varseed + basetime2)%secondary.length()];
                            if(!spec || spec2.priority > spec->priority)
                            {
                                spec = &spec2;
                                info.anim >>= ANIM_SECONDARY;
                                info.basetime = basetime2;
                            }
                        }
                    }
                }
                if(spec)
                {
                    info.frame = spec->frame;
                    info.range = spec->range;
                    if(spec->speed>0) info.speed = 1000.0f/spec->speed;
                }
                else getdefaultanim(info, anim, uint(varseed + info.basetime), d);
            }

            info.anim &= (1<<ANIM_SECONDARY)-1;
            info.anim |= anim&ANIM_FLAGS;
            if(info.anim&(ANIM_LOOP|ANIM_START|ANIM_END))
            {
                info.anim &= ~ANIM_SETTIME;
                if(!info.basetime) info.basetime = -((int)(size_t)d&0xFFF);
            }
            if(info.anim&(ANIM_START|ANIM_END))
            {
                if(info.anim&ANIM_END) info.frame += info.range-1;
                info.range = 1;
            }

            if(!meshes->hasframes(info.frame, info.range))
            {
                if(!meshes->hasframe(info.frame)) return false;
                info.range = meshes->clipframes(info.frame, info.range);
            }

            if(d && interp>=0)
            {
                animinterpinfo &ai = d->animinterp[interp];
                if(d->ragdoll && !(anim&ANIM_RAGDOLL)) 
                {
                    ai.prev.range = ai.cur.range = 0;
                    ai.lastswitch = -1;
                }
                else if(ai.lastmodel!=this || ai.lastswitch<0 || lastmillis-d->lastrendered>animationinterpolationtime)
                {
                    ai.prev = ai.cur = info;
                    ai.lastswitch = lastmillis-animationinterpolationtime*2;
                }
                else if(ai.cur!=info)
                {
                    if(lastmillis-ai.lastswitch>animationinterpolationtime/2) ai.prev = ai.cur;
                    ai.cur = info;
                    ai.lastswitch = lastmillis;
                }
                else if(info.anim&ANIM_SETTIME) ai.cur.basetime = info.basetime;
                ai.lastmodel = this;
            }
            return true;
        }

        float calcpitchaxis(int anim, float pitch, vec &axis, vec &dir, vec &campos, plane &fogplane)
        {
            float angle = pitchscale*pitch + pitchoffset;
            if(pitchmin || pitchmax) angle = max(pitchmin, min(pitchmax, angle));
            if(!angle) return 0;

            float c = cosf(-angle*RAD), s = sinf(-angle*RAD);
            vec d(axis);
            axis.rotate(c, s, d);
            if(!(anim&ANIM_NOSKIN))
            {
                dir.rotate(c, s, d);
                campos.rotate(c, s, d);
                fogplane.rotate(c, s, d);
            }

            return angle;
        }

        void render(int anim, int basetime, int basetime2, float pitch, const vec &axis, dynent *d, const vec &dir, const vec &campos, const plane &fogplane)
        {
            animstate as[MAXANIMPARTS];
            render(anim, basetime, basetime2, pitch, axis, d, dir, campos, fogplane, as);
        }

        void render(int anim, int basetime, int basetime2, float pitch, const vec &axis, dynent *d, const vec &dir, const vec &campos, const plane &fogplane, animstate *as)
        {
            if(!(anim&ANIM_REUSE)) loopi(numanimparts)
            {
                animinfo info;
                int interp = d && index+numanimparts<=MAXANIMPARTS ? index+i : -1;
                if(!calcanim(i, anim, basetime, basetime2, d, interp, info)) return;
                animstate &p = as[i];
                p.owner = this;
                p.anim = info.anim;
                p.cur.setframes(info);
                p.interp = 1;
                if(interp>=0 && d->animinterp[interp].prev.range>0)
                {
                    int diff = lastmillis-d->animinterp[interp].lastswitch;
                    if(diff<animationinterpolationtime)
                    {
                        p.prev.setframes(d->animinterp[interp].prev);
                        p.interp = diff/float(animationinterpolationtime);
                    }
                }
            }

            vec raxis(axis), rdir(dir), rcampos(campos);
            plane rfogplane(fogplane);
            float pitchamount = calcpitchaxis(anim, pitch, raxis, rdir, rcampos, rfogplane);
            if(pitchamount)
            {
                ++matrixpos;
                matrixstack[matrixpos] = matrixstack[matrixpos-1];
                matrixstack[matrixpos].rotate(pitchamount*RAD, axis);
            }

            if(!(anim&ANIM_NORENDER))
            {
                glPushMatrix();
                glMultMatrixf(matrixstack[matrixpos].v);
                if(model->scale!=1) glScalef(model->scale, model->scale, model->scale);
                if(!translate.iszero()) glTranslatef(translate.x, translate.y, translate.z);
                if(renderpath!=R_FIXEDFUNCTION && envmaptmu>=0)
                {
                    glMatrixMode(GL_TEXTURE);
                    glLoadMatrixf(matrixstack[matrixpos].v);
                    glMatrixMode(GL_MODELVIEW);
                }
            }

            if(!(anim&(ANIM_NOSKIN|ANIM_NORENDER)))
            {
                if(renderpath!=R_FIXEDFUNCTION)
                {
                    if(fogging) setfogplane(plane(rfogplane).translate(translate).scale(model->scale));
                    setenvparamf("direction", SHPARAM_VERTEX, 0, rdir.x, rdir.y, rdir.z);
                    vec ocampos(rcampos);
                    ocampos.div(model->scale).sub(translate);
                    setenvparamf("camera", SHPARAM_VERTEX, 1, ocampos.x, ocampos.y, ocampos.z, 1);
                }
                else
                {
                    if(fogging) refractfogplane = plane(rfogplane).translate(translate).scale(model->scale);
                }
            }

            meshes->render(as, pitch, axis, d, this);

            if(!(anim&ANIM_NORENDER))
            {
                glPopMatrix();
            }

            if(!(anim&ANIM_REUSE)) 
            {
                if (d && LogicSystem::getLogicEntity(d).get()) // INTENSITY
                    LogicSystem::getLogicEntity(d)->noteActualRender(); // INTENSITY

                loopv(links)
                {
                    linkedpart &link = links[i];

                    matrixpos++;
                    matrixstack[matrixpos].mul(matrixstack[matrixpos-1], link.matrix);

                    if(link.pos) *link.pos = matrixstack[matrixpos].gettranslation();

                    if(!link.p)
                    {
                        matrixpos--;
                        continue;
                    }

                    vec naxis(raxis), ndir(rdir), ncampos(rcampos);
                    plane nfogplane(rfogplane);
                    link.matrix.invertnormal(naxis);
                    if(!(anim&(ANIM_NOSKIN|ANIM_NORENDER)))
                    {
                        link.matrix.invertnormal(ndir);
                        link.matrix.invertvertex(ncampos);
                        link.matrix.invertplane(nfogplane);
                    }

                    int nanim = anim, nbasetime = basetime, nbasetime2 = basetime2;
                    if(link.anim>=0)
                    {
                        nanim = link.anim | (anim&ANIM_FLAGS);
                        nbasetime = link.basetime;
                        nbasetime2 = 0;
                    }
                    link.p->render(nanim, nbasetime, nbasetime2, pitch, naxis, d, ndir, ncampos, nfogplane);

                    matrixpos--;
                }
            }

            if(pitchamount) matrixpos--;
        }

        void setanim(int animpart, int num, int frame, int range, float speed, int priority = 0)
        {
            if(animpart<0 || animpart>=MAXANIMPARTS) return;
            if(frame<0 || range<=0 || !meshes || !meshes->hasframes(frame, range))
            {
                conoutf("invalid frame %d, range %d in model %s", frame, range, model->loadname);
                return;
            }
            if(!anims[animpart]) anims[animpart] = new vector<animspec>[ANIM_ALL+1]; // INTENSITY: Was NUMANIMS. We allow 128 animations.
            animspec &spec = anims[animpart][num].add();
            spec.frame = frame;
            spec.range = range;
            spec.speed = speed;
            spec.priority = priority;
        }
    };

    enum
    {
        LINK_TAG = 0,
        LINK_COOP,
        LINK_REUSE
    };

    virtual int linktype(animmodel *m) const { return LINK_TAG; }

    void render(int anim, int basetime, int basetime2, float pitch, const vec &axis, dynent *d, modelattach *a, const vec &dir, const vec &campos, const plane &fogplane)
    {
        if(!loaded) return;

        int numtags = 0;
        if(a)
        {
            int index = parts.last()->index + parts.last()->numanimparts;
            for(int i = 0; a[i].tag; i++)
            {
                numtags++;

                animmodel *m = (animmodel *)a[i].m;
                if(!m || !m->loaded)
                {
                    if(a[i].pos) link(NULL, a[i].tag, 0, 0, a[i].pos);
                    continue;
                }
                part *p = m->parts[0];
                switch(linktype(m))
                {
                    case LINK_TAG:
                        p->index = link(p, a[i].tag, a[i].anim, a[i].basetime, a[i].pos) ? index : -1;
                        break;

                    case LINK_COOP:
                        p->index = index;
                        break;

                    default:
                        continue;
                }
                index += p->numanimparts;
            }
        }

        animstate as[MAXANIMPARTS];
        parts[0]->render(anim, basetime, basetime2, pitch, axis, d, dir, campos, fogplane, as);

        if(a) for(int i = numtags-1; i >= 0; i--)
        {
            animmodel *m = (animmodel *)a[i].m;
            if(!m || !m->loaded)
            {
                if(a[i].pos) unlink(NULL);
                continue;
            }
            part *p = m->parts[0];
            switch(linktype(m))
            {
                case LINK_TAG:    
                    if(p->index >= 0) unlink(p);
                    p->index = 0;
                    break;

                case LINK_COOP:
                    p->render(anim, basetime, basetime2, pitch, axis, d, dir, campos, fogplane);
                    p->index = 0;
                    break;

                case LINK_REUSE:
                    p->render(anim | ANIM_REUSE, basetime, basetime2, pitch, axis, d, dir, campos, fogplane, as); 
                    break;
            }
        }
    }

    void render(int anim, int basetime, int basetime2, const vec &o, float yaw, float pitch, float roll, dynent *d, modelattach *a, const vec &color, const vec &dir, float trans) // INTENSITY: roll
    {
        if(!loaded) return;

        vec rdir, campos;
        plane fogplane;

        yaw += offsetyaw + spinyaw*lastmillis/1000.0f;
        pitch += offsetpitch + spinpitch*lastmillis/1000.0f;

        matrixpos = 0;
        matrixstack[0].identity();
        if(!d || !d->ragdoll || anim&ANIM_RAGDOLL)
        {
            matrixstack[0].translate(o);
            matrixstack[0].rotate_around_z((yaw+180)*RAD);
            if(roll) matrixstack[0].rotate_around_x(-roll*RAD); // INTENSITY: roll
        }
        else pitch = 0;

        if(anim&ANIM_NORENDER)
        {
            render(anim, basetime, basetime2, pitch, vec(0, -1, 0), d, a, rdir, campos, fogplane);
            if(d) d->lastrendered = lastmillis;
            return;
        }

        if(!(anim&ANIM_NOSKIN))
        {
            if(renderpath==R_FIXEDFUNCTION && lightmodels)
            {
                GLfloat pos[4] = { dir.x*1000, dir.y*1000, dir.z*1000, 0 };
                glLightfv(GL_LIGHT0, GL_POSITION, pos);
            }
                
            transparent = trans;
            lightcolor = color;
            
            fogplane = plane(0, 0, 1, -reflectz);
            rdir = dir;
            campos = camera1->o;
            if(!d || !d->ragdoll || anim&ANIM_RAGDOLL) 
            {
                fogplane.offset += o.z;
                rdir.rotate_around_z((-yaw-180.0f)*RAD);
                rdir.rotate_around_x(roll*RAD); // INTENSITY: roll
                campos.sub(o);
                campos.rotate_around_z((-yaw-180.0f)*RAD);
                campos.rotate_around_x(roll*RAD); // INTENSITY: roll
            }

            if(envmapped()) envmaptmu = 2;
            else if(a) for(int i = 0; a[i].tag; i++) if(a[i].m && a[i].m->envmapped())
            {
                envmaptmu = 2;
                break;
            }
            if(envmaptmu>=0) closestenvmaptex = lookupenvmap(closestenvmap(o));
        }

        if(envmaptmu>=0 && renderpath==R_FIXEDFUNCTION)
        {
            if(fogging) envmaptmu = 3;

            glActiveTexture_(GL_TEXTURE0_ARB+envmaptmu);
            setuptmu(envmaptmu, "T , P @ Pa", transparent<1 ? "= Ka" : NULL);

            glmatrixf mmtrans = mvmatrix;
            if(reflecting) mmtrans.reflectz(reflectz);
            mmtrans.transpose();

            glMatrixMode(GL_TEXTURE);
            glLoadMatrixf(mmtrans.v);
            glMatrixMode(GL_MODELVIEW);
            glActiveTexture_(GL_TEXTURE0_ARB);
        }

        if(transparent<1)
        {
            if(alphadepth)
            {
                glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
                render(anim|ANIM_NOSKIN, basetime, basetime2, pitch, vec(0, -1, 0), d, a, rdir, campos, fogplane);
                glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, fading ? GL_FALSE : GL_TRUE);

                glDepthFunc(GL_LEQUAL);
            }

            if(!enablealphablend)
            {
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                enablealphablend = true;
            }
        }

        render(anim, basetime, basetime2, pitch, vec(0, -1, 0), d, a, rdir, campos, fogplane);

        if(envmaptmu>=0)
        {
            if(renderpath==R_FIXEDFUNCTION) glActiveTexture_(GL_TEXTURE0_ARB+envmaptmu);
            glMatrixMode(GL_TEXTURE);
            glLoadIdentity();
            glMatrixMode(GL_MODELVIEW);
            if(renderpath==R_FIXEDFUNCTION) glActiveTexture_(GL_TEXTURE0_ARB);
        }

        if(transparent<1 && alphadepth) glDepthFunc(GL_LESS);

        if(d) d->lastrendered = lastmillis;
    }

    bool loaded;
    char *loadname;
    vector<part *> parts;

    animmodel(const char *name) : loaded(false)
    {
        loadname = newstring(name);
    }

    virtual ~animmodel()
    {
        delete[] loadname;
        parts.deletecontentsp();
    }

    char *name() { return loadname; }

    void cleanup()
    {
        loopv(parts) parts[i]->cleanup();
        enablelight0 = false;
    }

    void initmatrix(matrix3x4 &m)
    {
        m.identity();
        if(offsetyaw) m.rotate_around_z(offsetyaw*RAD);
        if(offsetpitch) m.rotate_around_y(-offsetpitch*RAD);
    }

    void gentris(int frame, vector<BIH::tri> *tris)
    {
        if(parts.empty()) return;
        matrix3x4 m;
        initmatrix(m);
        parts[0]->gentris(frame, tris, m);
    }

    BIH *setBIH()
    {
        if(bih) return bih;
        vector<BIH::tri> tris[2];
        gentris(0, tris);
        bih = new BIH(tris);
        return bih;
    }

    bool link(part *p, const char *tag, int anim = -1, int basetime = 0, vec *pos = NULL)
    {
        if(parts.empty()) return false;
        return parts[0]->link(p, tag, anim, basetime, pos);
    }

    bool unlink(part *p)
    {
        if(parts.empty()) return false;
        return parts[0]->unlink(p);
    }

    bool envmapped()
    {
        loopv(parts) loopvj(parts[i]->skins) if(parts[i]->skins[j].envmapped()) return true;
        return false;
    }

    virtual bool loaddefaultparts()
    {
        return true;
    }

    void preloadshaders()
    {
        loopv(parts) parts[i]->preloadshaders();
    }

    void setshader(Shader *shader)
    {
        if(parts.empty()) loaddefaultparts();
        loopv(parts) loopvj(parts[i]->skins) parts[i]->skins[j].shader = shader;
    }

    void setenvmap(float envmapmin, float envmapmax, Texture *envmap)
    {
        if(parts.empty()) loaddefaultparts();
        loopv(parts) loopvj(parts[i]->skins)
        {
            skin &s = parts[i]->skins[j];
            if(envmapmax)
            {
                s.envmapmin = envmapmin;
                s.envmapmax = envmapmax;
            }
            if(envmap) s.envmap = envmap;
        }
    }

    void setspec(float spec)
    {
        if(parts.empty()) loaddefaultparts();
        loopv(parts) loopvj(parts[i]->skins) parts[i]->skins[j].spec = spec;
    }

    void setambient(float ambient)
    {
        if(parts.empty()) loaddefaultparts();
        loopv(parts) loopvj(parts[i]->skins) parts[i]->skins[j].ambient = ambient;
    }

    void setglow(float glow)
    {
        if(parts.empty()) loaddefaultparts();
        loopv(parts) loopvj(parts[i]->skins) parts[i]->skins[j].glow = glow;
    }

    void setglare(float specglare, float glowglare)
    {
        if(parts.empty()) loaddefaultparts();
        loopv(parts) loopvj(parts[i]->skins)
        {
            skin &s = parts[i]->skins[j];
            s.specglare = specglare;
            s.glowglare = glowglare;
        }
    }

    void setalphatest(float alphatest)
    {
        if(parts.empty()) loaddefaultparts();
        loopv(parts) loopvj(parts[i]->skins) parts[i]->skins[j].alphatest = alphatest;
    }

    void setalphablend(bool alphablend)
    {
        if(parts.empty()) loaddefaultparts();
        loopv(parts) loopvj(parts[i]->skins) parts[i]->skins[j].alphablend = alphablend;
    }

    void setfullbright(float fullbright)
    {
        if(parts.empty()) loaddefaultparts();
        loopv(parts) loopvj(parts[i]->skins) parts[i]->skins[j].fullbright = fullbright;
    }

    void setcullface(bool cullface)
    {
        if(parts.empty()) loaddefaultparts();
        loopv(parts) loopvj(parts[i]->skins) parts[i]->skins[j].cullface = cullface;
    }

    void calcbb(int frame, vec &center, vec &radius)
    {
        if(parts.empty()) return;
        vec bbmin(1e16f, 1e16f, 1e16f), bbmax(-1e16f, -1e16f, -1e16f);
        matrix3x4 m;
        initmatrix(m); 
        parts[0]->calcbb(frame, bbmin, bbmax, m);
        radius = bbmax;
        radius.sub(bbmin);
        radius.mul(0.5f);
        center = bbmin;
        center.add(radius);
    }

    static bool enabletc, enablemtc, enablealphatest, enablealphablend, enableenvmap, enableglow, enableoverbright, enablelighting, enablelight0, enablecullface, enablefog, enablenormals, enabletangents, enablebones, enablerescale;
    static vec lightcolor;
    static plane refractfogplane;
    static float transparent, lastalphatest;
    static void *lastvbuf, *lasttcbuf, *lastmtcbuf, *lastnbuf, *lastxbuf, *lastbbuf, *lastsdata, *lastbdata;
    static GLuint lastebuf, lastenvmaptex, closestenvmaptex;
    static Texture *lasttex, *lastmasks, *lastnormalmap;
    static int envmaptmu, fogtmu, matrixpos;
    static glmatrixf matrixstack[64];

    void startrender()
    {
        enabletc = enablemtc = enablealphatest = enablealphablend = enableenvmap = enableglow = enableoverbright = enablelighting = enablefog = enablenormals = enabletangents = enablebones = enablerescale = false;
        enablecullface = true;
        lastalphatest = -1;
        lastvbuf = lasttcbuf = lastmtcbuf = lastxbuf = lastnbuf = lastbbuf = lastsdata = lastbdata = NULL;
        lastebuf = lastenvmaptex = closestenvmaptex = 0;
        lasttex = lastmasks = lastnormalmap = NULL;
        envmaptmu = fogtmu = -1;
        transparent = 1;

        if(renderpath==R_FIXEDFUNCTION && lightmodels && !enablelight0)
        {
            glEnable(GL_LIGHT0);
            static const GLfloat zero[4] = { 0, 0, 0, 0 };
            glLightModelfv(GL_LIGHT_MODEL_AMBIENT, zero);
            glLightfv(GL_LIGHT0, GL_SPECULAR, zero);
            glMaterialfv(GL_FRONT, GL_SPECULAR, zero);
            glMaterialfv(GL_FRONT, GL_EMISSION, zero);
            enablelight0 = true;
        }
    }

    static void disablebones()
    {
        glDisableVertexAttribArray_(6);
        glDisableVertexAttribArray_(7);
        enablebones = false;
    }

    static void disabletangents()
    {
        glDisableVertexAttribArray_(1);
        enabletangents = false;
    }

    static void disablemtc()
    {
        glClientActiveTexture_(GL_TEXTURE1_ARB);
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        glClientActiveTexture_(GL_TEXTURE0_ARB);
        enablemtc = false;
    }

    static void disabletc()
    {
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        if(enablemtc) disablemtc();
        enabletc = false;
    }

    static void disablenormals()
    {
        glDisableClientState(GL_NORMAL_ARRAY);
        enablenormals = false;
    }

    static void disablevbo()
    {
        if(hasVBO)
        {
            glBindBuffer_(GL_ARRAY_BUFFER_ARB, 0);
            glBindBuffer_(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
        }
        glDisableClientState(GL_VERTEX_ARRAY);
        if(enabletc) disabletc();
        if(enablenormals) disablenormals();
        if(enabletangents) disabletangents();
        if(enablebones) disablebones();
        lastvbuf = lasttcbuf = lastmtcbuf = lastxbuf = lastnbuf = lastbbuf = NULL;
        lastebuf = 0;
    }

    static void disableoverbright()
    {
        resettmu(0);
        enableoverbright = false;
    }

    static void disableglow()
    {
        resettmu(0);
        glActiveTexture_(GL_TEXTURE1_ARB);
        resettmu(1);
        glDisable(GL_TEXTURE_2D);
        glActiveTexture_(GL_TEXTURE0_ARB);
        lasttex = lastmasks = NULL;
        enableglow = false;
    }

    static void disablefog(bool cleanup = false)
    {
        glActiveTexture_(GL_TEXTURE0_ARB+fogtmu);
        if(enablefog) glDisable(GL_TEXTURE_1D);
        if(cleanup)
        {
            resettmu(fogtmu);
            glDisable(GL_TEXTURE_GEN_S);
            fogtmu = -1;
        }
        glActiveTexture_(GL_TEXTURE0_ARB);
        enablefog = false;
    }

    static void disableenvmap(bool cleanup = false)
    {
        glActiveTexture_(GL_TEXTURE0_ARB+envmaptmu);
        if(enableenvmap) glDisable(GL_TEXTURE_CUBE_MAP_ARB);
        if(cleanup && renderpath==R_FIXEDFUNCTION)
        {
            resettmu(envmaptmu);
            glDisable(GL_TEXTURE_GEN_S);
            glDisable(GL_TEXTURE_GEN_T);
            glDisable(GL_TEXTURE_GEN_R);
        }
        glActiveTexture_(GL_TEXTURE0_ARB);
        enableenvmap = false;
    }

    void endrender()
    {
        if(lastvbuf || lastebuf) disablevbo();
        if(enablealphatest) glDisable(GL_ALPHA_TEST);
        if(enablealphablend) glDisable(GL_BLEND);
        if(enableglow) disableglow();
        if(enableoverbright) disableoverbright();
        if(enablelighting) glDisable(GL_LIGHTING);
        if(lastenvmaptex) disableenvmap(true);
        if(enablerescale) glDisable(hasRN ? GL_RESCALE_NORMAL_EXT : GL_NORMALIZE);
        if(!enablecullface) glEnable(GL_CULL_FACE);
        if(fogtmu>=0) disablefog(true);
    }
};

bool animmodel::enabletc = false, animmodel::enablemtc = false, animmodel::enablealphatest = false, animmodel::enablealphablend = false,
     animmodel::enableenvmap = false, animmodel::enableglow = false, animmodel::enableoverbright = false, animmodel::enablelighting = false, animmodel::enablelight0 = false, animmodel::enablecullface = true,
     animmodel::enablefog = false, animmodel::enablenormals = false, animmodel::enabletangents = false, animmodel::enablebones = false, animmodel::enablerescale = false;
vec animmodel::lightcolor;
plane animmodel::refractfogplane;
float animmodel::transparent = 1, animmodel::lastalphatest = -1;
void *animmodel::lastvbuf = NULL, *animmodel::lasttcbuf = NULL, *animmodel::lastmtcbuf = NULL, *animmodel::lastnbuf = NULL, *animmodel::lastxbuf = NULL, *animmodel::lastbbuf = NULL, *animmodel::lastsdata = NULL, *animmodel::lastbdata = NULL;
GLuint animmodel::lastebuf = 0, animmodel::lastenvmaptex = 0, animmodel::closestenvmaptex = 0;
Texture *animmodel::lasttex = NULL, *animmodel::lastmasks = NULL, *animmodel::lastnormalmap = NULL;
int animmodel::envmaptmu = -1, animmodel::fogtmu = -1, animmodel::matrixpos = 0;
glmatrixf animmodel::matrixstack[64];


