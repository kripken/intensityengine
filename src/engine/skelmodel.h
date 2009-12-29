// INTENSITY: Version: rev 1944 in sauerbraten SVN, October 24 2009. Fix issue with antipodes having negative index, following our report, and also allow not all meshes to have bumpmaps
VARP(gpuskel, 0, 1, 1);
VARP(matskel, 0, 1, 1);

#define BONEMASK_NOT  0x8000
#define BONEMASK_END  0xFFFF
#define BONEMASK_BONE 0x7FFF

static int bonemaskcmp(ushort *x, ushort *y)
{
    if(*x<*y) return -1;
    if(*x>*y) return 1;
    return 0;
}

struct skelmodel : animmodel
{
    struct vert { vec pos, norm; float u, v; int blend, interpindex; };
    struct vvert { vec pos; float u, v; };
    struct vvertn : vvert { vec norm; };
    struct vvertw : vvertn { uchar weights[4]; uchar bones[4]; };
    struct vvertbump : vvertn { vec tangent; float bitangent; };
    struct vvertbumpw : vvertw { vec tangent; float bitangent; };
    struct bumpvert { vec tangent; float bitangent; };
    struct tri { ushort vert[3]; };

    struct blendcombo
    {
        int uses, interpindex;
        float weights[4];
        uchar bones[4], interpbones[4];

        blendcombo() : uses(1)
        {
        }

        bool operator==(const blendcombo &c) const
        {
            loopk(4) if(bones[k] != c.bones[k]) return false;
            loopk(4) if(weights[k] != c.weights[k]) return false;
            return true;
        }

        int size() const
        {
            int i = 1;
            while(i < 4 && weights[i]) i++;
            return i;
        }

        static int sortcmp(const blendcombo *x, const blendcombo *y)
        {
            loopi(4)
            {
                if(x->weights[i])
                {
                    if(!y->weights[i]) return -1;
                }
                else if(y->weights[i]) return 1;
                else break;
            }
            return 0;
        }

        int addweight(int sorted, float weight, int bone)
        {
            if(weight <= 1e-3f) return sorted;
            loopk(sorted) if(weight > weights[k])
            {
                for(int l = min(sorted-1, 2); l >= k; l--)
                {
                    weights[l+1] = weights[l];
                    bones[l+1] = bones[l];
                }
                weights[k] = weight;
                bones[k] = bone;
                return sorted<4 ? sorted+1 : sorted;
            }
            if(sorted>=4) return sorted;
            weights[sorted] = weight;
            bones[sorted] = bone;
            return sorted+1;
        }
        
        void finalize(int sorted)
        {
            loopj(4-sorted) { weights[sorted+j] = 0; bones[sorted+j] = 0; }
            if(sorted <= 0) return;
            float total = 0;
            loopj(sorted) total += weights[j];
            total = 1.0f/total;
            loopj(sorted) weights[j] *= total;
        }

        void serialize(vvertw &v)
        {
            if(interpindex >= 0)
            {
                v.weights[0] = 255;
                loopk(3) v.weights[k+1] = 0;
                v.bones[0] = (matskel ? 3 : 2)*interpindex;
                loopk(3) v.bones[k+1] = v.bones[0];
            }
            else
            {
                int total = 0;
                loopk(4) total += (v.weights[k] = uchar(weights[k]*255));
                while(total > 255)
                {
                    loopk(4) if(v.weights[k] > 0 && total > 255) { v.weights[k]--; total--; } 
                }
                while(total < 255)
                {
                    loopk(4) if(v.weights[k] < 255 && total < 255) { v.weights[k]++; total++; }
                }
                loopk(4) v.bones[k] = (matskel ? 3 : 2)*interpbones[k];
            }
        }
    };


    struct animcacheentry
    {
        animstate as[MAXANIMPARTS];
        float pitch;
        int millis;
        uchar *partmask;
        ragdolldata *ragdoll;

        animcacheentry() : ragdoll(NULL)
        {
            loopk(MAXANIMPARTS) as[k].cur.fr1 = as[k].prev.fr1 = -1;
        }

        bool operator==(const animcacheentry &c) const
        {
            loopi(MAXANIMPARTS) if(as[i]!=c.as[i]) return false;
            return pitch==c.pitch && partmask==c.partmask && ragdoll==c.ragdoll && (!ragdoll || min(millis, c.millis) >= ragdoll->lastmove);
        }
    };

    struct vbocacheentry : animcacheentry
    {
        uchar *vdata;
        GLuint vbuf;
        int owner;

        vbocacheentry() : vdata(NULL), vbuf(0), owner(-1) {}
    };
    
    struct skelcacheentry : animcacheentry
    {
        dualquat *bdata;
        matrix3x4 *mdata;

        skelcacheentry() : bdata(NULL), mdata(NULL) {}
    };

    struct blendcacheentry : skelcacheentry
    {
        int owner;

        blendcacheentry() : owner(-1) {}
    };

    struct skelmeshgroup;

    struct skelmesh : mesh
    {
        vert *verts;
        bumpvert *bumpverts;
        tri *tris;
        int numverts, numtris, maxweights;

        int voffset, eoffset, elen;
        ushort minvert, maxvert;

        skelmesh() : verts(NULL), bumpverts(NULL), tris(NULL), numverts(0), numtris(0), maxweights(0)
        {
        }

        virtual ~skelmesh()
        {
            DELETEA(verts);
            DELETEA(bumpverts);
            DELETEA(tris);
        }

        int addblendcombo(const blendcombo &c)
        {
            maxweights = max(maxweights, c.size());
            return ((skelmeshgroup *)group)->addblendcombo(c);
        }

        void smoothnorms(float limit = 0, bool areaweight = true)
        {
            hashtable<vec, int> share;
            int *next = new int[numverts];
            memset(next, -1, numverts*sizeof(int));
            loopi(numverts) 
            {
                vert &v = verts[i];
                v.norm = vec(0, 0, 0);
                int idx = share.access(v.pos, i);
                if(idx != i) { next[i] = next[idx]; next[idx] = i; }
            }
            loopi(numtris)
            {
                tri &t = tris[i];
                vert &v1 = verts[t.vert[0]], &v2 = verts[t.vert[1]], &v3 = verts[t.vert[2]];
                vec norm;
                norm.cross(vec(v2.pos).sub(v1.pos), vec(v3.pos).sub(v1.pos));
                if(!areaweight) norm.normalize();
                v1.norm.add(norm);
                v2.norm.add(norm);
                v3.norm.add(norm);
            }
            vec *norms = new vec[numverts];
            memset(norms, 0, numverts*sizeof(vec));
            loopi(numverts)
            {
                vert &v = verts[i];
                norms[i].add(v.norm);
                if(next[i] >= 0)
                {
                    float vlimit = limit*v.norm.magnitude();
                    for(int j = next[i]; j >= 0; j = next[j])
                    {
                        vert &o = verts[j];
                        if(v.norm.dot(o.norm) >= vlimit*o.norm.magnitude()) 
                        {
                            norms[i].add(o.norm);
                            norms[j].add(v.norm);
                        }
                    }
                }
            } 
            loopi(numverts) verts[i].norm = norms[i].normalize();
            delete[] next;
            delete[] norms;
        }

        void buildnorms(bool areaweight = true)
        {
            loopi(numverts) verts[i].norm = vec(0, 0, 0);
            loopi(numtris)
            {
                tri &t = tris[i];
                vert &v1 = verts[t.vert[0]], &v2 = verts[t.vert[1]], &v3 = verts[t.vert[2]];
                vec norm;
                norm.cross(vec(v2.pos).sub(v1.pos), vec(v3.pos).sub(v1.pos));
                if(!areaweight) norm.normalize();
                v1.norm.add(norm);
                v2.norm.add(norm);
                v3.norm.add(norm);
            }
            loopi(numverts) verts[i].norm.normalize();
        }

        void calctangents(bool areaweight = true)
        {
            if(bumpverts) return;
            vec *tangent = new vec[2*numverts], *bitangent = tangent+numverts;
            memset(tangent, 0, 2*numverts*sizeof(vec));
            bumpverts = new bumpvert[numverts];
            loopi(numtris)
            {
                const tri &t = tris[i];
                const vert &av = verts[t.vert[0]],
                           &bv = verts[t.vert[1]],
                           &cv = verts[t.vert[2]];

                vec e1(bv.pos), e2(cv.pos);
                e1.sub(av.pos);
                e2.sub(av.pos);

                float u1 = bv.u - av.u, v1 = bv.v - av.v,
                      u2 = cv.u - av.u, v2 = cv.v - av.v,
                      scale = u1*v2 - u2*v1;
                if(scale!=0) scale = 1.0f / scale;
                vec u(e1), v(e2);
                u.mul(v2).sub(vec(e2).mul(v1)).mul(scale);
                v.mul(u1).sub(vec(e1).mul(u2)).mul(scale);

                if(!areaweight)
                {
                    u.normalize();
                    v.normalize();
                }

                loopj(3)
                {
                    tangent[t.vert[j]].add(u);
                    bitangent[t.vert[j]].add(v);
                }
            }
            loopi(numverts)
            {
                const vec &n = verts[i].norm,
                          &t = tangent[i],
                          &bt = bitangent[i];
                bumpvert &bv = bumpverts[i];
                (bv.tangent = t).sub(vec(n).mul(n.dot(t))).normalize();
                bv.bitangent = vec().cross(n, t).dot(bt) < 0 ? -1 : 1;
            }
            delete[] tangent;
        }

        void calcbb(int frame, vec &bbmin, vec &bbmax, const matrix3x4 &m)
        {
            loopj(numverts)
            {
                vec v = m.transform(verts[j].pos);
                loopi(3)
                {
                    bbmin[i] = min(bbmin[i], v[i]);
                    bbmax[i] = max(bbmax[i], v[i]);
                }
            }
        }

        void gentris(int frame, Texture *tex, vector<BIH::tri> *out, const matrix3x4 &m)
        {
            loopj(numtris)
            {
                BIH::tri &t = out[noclip ? 1 : 0].add();
                t.tex = tex->bpp==4 ? tex : NULL;
                vert &av = verts[tris[j].vert[0]],
                     &bv = verts[tris[j].vert[1]],
                     &cv = verts[tris[j].vert[2]];
                t.a = m.transform(av.pos);
                t.b = m.transform(bv.pos);
                t.c = m.transform(cv.pos);
                t.tc[0] = av.u;
                t.tc[1] = av.v;
                t.tc[2] = bv.u;
                t.tc[3] = bv.v;
                t.tc[4] = cv.u;
                t.tc[5] = cv.v;
            }
        }

        static inline bool comparevert(vvert &w, int j, vert &v)
        {
            return v.u==w.u && v.v==w.v && v.pos==w.pos;
        }

        static inline bool comparevert(vvertn &w, int j, vert &v)
        {
            return v.u==w.u && v.v==w.v && v.pos==w.pos && v.norm==w.norm;
        }

        inline bool comparevert(vvertbump &w, int j, vert &v)
        {
            return v.u==w.u && v.v==w.v && v.pos==w.pos && v.norm==w.norm && (!bumpverts || (bumpverts[j].tangent==w.tangent && bumpverts[j].bitangent==w.bitangent));
        }

        static inline void assignvert(vvert &vv, int j, vert &v, blendcombo &c)
        {
            vv.pos = v.pos;
            vv.u = v.u;
            vv.v = v.v;
        }

        static inline void assignvert(vvertn &vv, int j, vert &v, blendcombo &c)
        {
            vv.pos = v.pos;
            vv.norm = v.norm;
            vv.u = v.u;
            vv.v = v.v;
        }

        inline void assignvert(vvertbump &vv, int j, vert &v, blendcombo &c)
        {
            vv.pos = v.pos;
            vv.norm = v.norm;
            vv.u = v.u;
            vv.v = v.v;
            if(bumpverts)
            {
                vv.tangent = bumpverts[j].tangent;
                vv.bitangent = bumpverts[j].bitangent;
            }
            else
            {
                vv.tangent = vec(0, 0, 0);
                vv.bitangent = 0;
            }
        }

        static inline void assignvert(vvertw &vv, int j, vert &v, blendcombo &c)
        {
            vv.pos = v.pos;
            vv.norm = v.norm;
            vv.u = v.u;
            vv.v = v.v;
            c.serialize(vv);
        }

        inline void assignvert(vvertbumpw &vv, int j, vert &v, blendcombo &c)
        {
            vv.pos = v.pos;
            vv.norm = v.norm;
            vv.u = v.u;
            vv.v = v.v;
            if(bumpverts)
            {
                vv.tangent = bumpverts[j].tangent;
                vv.bitangent = bumpverts[j].bitangent;
            }
            else
            {
                vv.tangent = vec(0, 0, 0);
                vv.bitangent = 0;
            }
            c.serialize(vv);
        }

        template<class T>
        int genvbo(vector<ushort> &idxs, int offset, vector<T> &vverts)
        {
            voffset = offset;
            eoffset = idxs.length();
            if(((skelmeshgroup *)group)->skel->numframes)
            {
                loopi(numverts)
                {
                    vert &v = verts[i];
                    assignvert(vverts.add(), i, v, ((skelmeshgroup *)group)->blendcombos[v.blend]);
                }
                loopi(numtris) loopj(3) idxs.add(voffset + tris[i].vert[j]);
                elen = idxs.length()-eoffset;
                minvert = voffset;
                maxvert = voffset + numverts-1;
                return numverts;
            }
            minvert = 0xFFFF;
            loopi(numtris)
            {
                tri &t = tris[i];
                loopj(3)
                {
                    int index = t.vert[j];
                    vert &v = verts[index];
                    loopvk(vverts)
                    {
                        if(comparevert(vverts[k], index, v)) { minvert = min(minvert, (ushort)k); idxs.add((ushort)k); goto found; }
                    }
                    idxs.add(vverts.length());
                    assignvert(vverts.add(), index, v, ((skelmeshgroup *)group)->blendcombos[v.blend]);
                found:;
                }
            }
            elen = idxs.length()-eoffset;
            minvert = min(minvert, ushort(voffset));
            maxvert = max(minvert, ushort(vverts.length()-1));
            return vverts.length()-voffset;
        }

        int genvbo(vector<ushort> &idxs, int offset)
        {
            loopi(numverts) verts[i].interpindex = ((skelmeshgroup *)group)->remapblend(verts[i].blend);
            
            voffset = offset;
            eoffset = idxs.length();
            loopi(numtris)
            {
                tri &t = tris[i];
                loopj(3) idxs.add(voffset+t.vert[j]);
            }
            minvert = voffset;
            maxvert = voffset + numverts-1;
            elen = idxs.length()-eoffset;
            return numverts;
        }

        void filltc(uchar *vdata, size_t stride)
        {
            vdata = (uchar *)&((vvert *)&vdata[voffset*stride])->u;
            loopi(numverts)
            {
                ((float *)vdata)[0] = verts[i].u;
                ((float *)vdata)[1] = verts[i].v; 
                vdata += stride;
            }
        }

        void fillbump(uchar *vdata, size_t stride)
        {
            if(stride==sizeof(vvertbumpw)) vdata = (uchar *)&((vvertbumpw *)&vdata[voffset*stride])->tangent;
            else vdata = (uchar *)&((vvertbump *)&vdata[voffset*stride])->tangent;
            if(bumpverts) loopi(numverts)
            {
                ((bumpvert *)vdata)->bitangent = bumpverts[i].bitangent;
                vdata += stride;
            }
            else loopi(numverts)
            {
                memset(vdata, 0, sizeof(bumpvert));
                vdata += stride;
            }
        }

        void interpmatverts(skelcacheentry &sc, blendcacheentry *bc, bool norms, bool tangents, void *vdata, skin &s)
        {
            const int blendoffset = ((skelmeshgroup *)group)->skel->numinterpbones;
            const matrix3x4 *mdata1 = sc.mdata, *mdata2 = bc ? bc->mdata - blendoffset : NULL;

            #define IPLOOPMAT(type, dosetup, dotransform) \
                loopi(numverts) \
                { \
                    const vert &src = verts[i]; \
                    type &dst = ((type *)vdata)[i]; \
                    dosetup; \
                    const matrix3x4 &m = (src.interpindex < blendoffset ? mdata1 : mdata2)[src.interpindex]; \
                    dst.pos = m.transform(src.pos); \
                    dotransform; \
                }

            if(tangents)
            {
                if(bumpverts)
                {
                    IPLOOPMAT(vvertbump, bumpvert &bsrc = bumpverts[i],
                    {
                        dst.norm = m.transformnormal(src.norm);
                        dst.tangent = m.transformnormal(bsrc.tangent);
                    });
                }
                else { IPLOOPMAT(vvertbump, , dst.norm = m.transformnormal(src.norm)); }
            }
            else if(norms) { IPLOOPMAT(vvertn, , dst.norm = m.transformnormal(src.norm)); }
            else { IPLOOPMAT(vvert, , ); }

            #undef IPLOOPMAT
        }

        void interpverts(skelcacheentry &sc, blendcacheentry *bc, bool norms, bool tangents, void *vdata, skin &s)
        {
            const int blendoffset = ((skelmeshgroup *)group)->skel->numinterpbones;
            const dualquat * const bdata1 = sc.bdata, * const bdata2 = bc ? bc->bdata - blendoffset : NULL;

            #define IPLOOP(type, dosetup, dotransform) \
                loopi(numverts) \
                { \
                    const vert &src = verts[i]; \
                    type &dst = ((type *)vdata)[i]; \
                    dosetup; \
                    const dualquat &d = (src.interpindex < blendoffset ? bdata1 : bdata2)[src.interpindex]; \
                    dst.pos = d.transform(src.pos); \
                    dotransform; \
                }

            if(tangents) 
            {
                if(bumpverts) 
                {
                    IPLOOP(vvertbump, bumpvert &bsrc = bumpverts[i], 
                    { 
                        dst.norm = d.real.rotate(src.norm);
                        dst.tangent = d.real.rotate(bsrc.tangent);
                    });
                }
                else { IPLOOP(vvertbump, , dst.norm = d.real.rotate(src.norm)); }
            }
            else if(norms) { IPLOOP(vvertn, , dst.norm = d.real.rotate(src.norm)); }
            else { IPLOOP(vvert, , ); }

            #undef IPLOOP
        }

        void setshader(Shader *s)
        {
            skelmeshgroup *g = (skelmeshgroup *)group;
            if(glaring)
            {
                if(!g->skel->usegpuskel) s->setvariant(0, 2);
                else if(g->skel->usematskel) s->setvariant(min(maxweights, g->vweights), 2);
                else s->setvariant(min(maxweights, g->vweights)-1, 3);
            }
            else if(!g->skel->usegpuskel) s->set();
            else if(g->skel->usematskel) s->setvariant(min(maxweights, g->vweights)-1, 0);
            else s->setvariant(min(maxweights, g->vweights)-1, 1);
        }

        void render(const animstate *as, skin &s, vbocacheentry &vc)
        {
            s.bind(this, as);

            if(!(as->anim&ANIM_NOSKIN))
            {
                if(s.multitextured())
                {
                    if(!enablemtc || lastmtcbuf!=lastvbuf)
                    {
                        glClientActiveTexture_(GL_TEXTURE1_ARB);
                        if(!enablemtc) glEnableClientState(GL_TEXTURE_COORD_ARRAY);
                        if(lastmtcbuf!=lastvbuf)
                        {
                            vvert *vverts = hasVBO ? 0 : (vvert *)vc.vdata;
                            glTexCoordPointer(2, GL_FLOAT, ((skelmeshgroup *)group)->vertsize, &vverts->u);
                        }
                        glClientActiveTexture_(GL_TEXTURE0_ARB);
                        lastmtcbuf = lastvbuf;
                        enablemtc = true;
                    }
                }
                else if(enablemtc) disablemtc();

                if(s.tangents())
                {
                    if(!enabletangents || lastxbuf!=lastvbuf)
                    {
                        if(!enabletangents) glEnableVertexAttribArray_(1);
                        if(lastxbuf!=lastvbuf)
                        {
                            if(((skelmeshgroup *)group)->vertsize==sizeof(vvertbumpw))
                            {
                                vvertbumpw *vverts = hasVBO ? 0 : (vvertbumpw *)vc.vdata;
                                glVertexAttribPointer_(1, 4, GL_FLOAT, GL_FALSE, ((skelmeshgroup *)group)->vertsize, &vverts->tangent.x);
                            }
                            else
                            {
                                vvertbump *vverts = hasVBO ? 0 : (vvertbump *)vc.vdata;
                                glVertexAttribPointer_(1, 4, GL_FLOAT, GL_FALSE, ((skelmeshgroup *)group)->vertsize, &vverts->tangent.x);
                            }
                        }
                        lastxbuf = lastvbuf;
                        enabletangents = true;
                    }
                }
                else if(enabletangents) disabletangents();

                if(renderpath==R_FIXEDFUNCTION && (s.scrollu || s.scrollv))
                {
                    glMatrixMode(GL_TEXTURE);
                    glPushMatrix();
                    glTranslatef(s.scrollu*lastmillis/1000.0f, s.scrollv*lastmillis/1000.0f, 0);

                    if(s.multitextured())
                    {
                        glActiveTexture_(GL_TEXTURE1_ARB);
                        glPushMatrix();
                        glTranslatef(s.scrollu*lastmillis/1000.0f, s.scrollv*lastmillis/1000.0f, 0);
                    }
                }
            }

            if(hasDRE) glDrawRangeElements_(GL_TRIANGLES, minvert, maxvert, elen, GL_UNSIGNED_SHORT, &((skelmeshgroup *)group)->edata[eoffset]);
            else glDrawElements(GL_TRIANGLES, elen, GL_UNSIGNED_SHORT, &((skelmeshgroup *)group)->edata[eoffset]);
            glde++;
            xtravertsva += numverts;

            if(renderpath==R_FIXEDFUNCTION && !(as->anim&ANIM_NOSKIN) && (s.scrollu || s.scrollv))
            {
                if(s.multitextured())
                {
                    glPopMatrix();
                    glActiveTexture_(GL_TEXTURE0_ARB);
                }

                glPopMatrix();
                glMatrixMode(GL_MODELVIEW);
            }

            return;
        }
    };

       
    struct tag
    {
        char *name;
        int bone;

        tag() : name(NULL) {}
        ~tag() { DELETEA(name); }
    };

    struct skelanimspec
    {
        char *name;
        int frame, range;

        skelanimspec() : name(NULL), frame(0), range(0) {}
        ~skelanimspec()
        {
            DELETEA(name);
        }
    };

    struct boneinfo
    {
        const char *name;
        int parent, children, next, group, scheduled, interpindex, interpparent, ragdollindex;
        float pitchscale, pitchoffset, pitchmin, pitchmax;
        dualquat base;

        boneinfo() : name(NULL), parent(-1), children(-1), next(-1), group(INT_MAX), scheduled(-1), interpindex(-1), interpparent(-1), ragdollindex(-1), pitchscale(0), pitchoffset(0), pitchmin(0), pitchmax(0) {}
        ~boneinfo()
        {
            DELETEA(name);
        }
    };

    struct antipode
    {
        int parent, child;

        antipode(int parent, int child) : parent(parent), child(child) {}
    };

    struct skeleton
    {
        char *name;
        int shared;
        vector<skelmeshgroup *> users;
        boneinfo *bones;
        int numbones, numinterpbones, numgpubones, numframes;
        dualquat *invbones, *framebones;
        matrix3x4 *matinvbones, *matframebones;
        vector<skelanimspec> skelanims;
        vector<tag> tags;
        vector<antipode> antipodes;
        ragdollskel *ragdoll;

        bool usegpuskel, usematskel;
        vector<skelcacheentry> skelcache;

        skeleton() : name(NULL), shared(0), bones(NULL), numbones(0), numinterpbones(0), numgpubones(0), numframes(0), invbones(NULL), framebones(NULL), matinvbones(NULL), matframebones(NULL), ragdoll(NULL), usegpuskel(false), usematskel(false)
        {
        }

        ~skeleton()
        {
            DELETEA(name);
            DELETEA(bones);
            DELETEA(invbones);
            DELETEA(framebones);
            DELETEA(matinvbones);
            DELETEA(matframebones);
            DELETEP(ragdoll);
            loopv(skelcache)
            {
                DELETEA(skelcache[i].bdata);
                DELETEA(skelcache[i].mdata);
            }
        }

        skelanimspec *findskelanim(const char *name, char sep = '\0')
        {
            int len = sep ? strlen(name) : 0;
            loopv(skelanims)
            {
                if(skelanims[i].name)
                {
                    if(sep)
                    {
                        const char *end = strchr(skelanims[i].name, ':');
                        if(end && end - skelanims[i].name == len && !memcmp(name, skelanims[i].name, len)) return &skelanims[i];
                    }
                    if(!strcmp(name, skelanims[i].name)) return &skelanims[i];
                }
            }
            return NULL;
        }

        skelanimspec &addskelanim(const char *name)
        {
            skelanimspec &sa = skelanims.add();
            sa.name = name ? newstring(name) : NULL;
            return sa;
        }

        int findbone(const char *name)
        {
            loopi(numbones) if(bones[i].name && !strcmp(bones[i].name, name)) return i;
            return -1;
        }

        int findtag(const char *name)
        {
            loopv(tags) if(!strcmp(tags[i].name, name)) return i;
            return -1;
        }

        bool addtag(const char *name, int bone)
        {
            if(findtag(name) >= 0) return false;
            tag &t = tags.add();
            t.name = newstring(name);
            t.bone = bone;
            return true;
        }

        void calcantipodes()
        {
            antipodes.setsize(0);
            vector<int> schedule;
            loopi(numbones) 
            {
                if(bones[i].group >= numbones) 
                {
                    bones[i].scheduled = schedule.length();
                    schedule.add(i);
                }
                else bones[i].scheduled = -1;
            }
            loopv(schedule)
            {
                int bone = schedule[i];
                const boneinfo &info = bones[bone];
                loopj(numbones) if(abs(bones[j].group) == bone && bones[j].scheduled < 0)
                {
                    antipodes.add(antipode(info.interpindex, bones[j].interpindex));
                    bones[j].scheduled = schedule.length();
                    schedule.add(j);
                }
                if(i + 1 == schedule.length())
                {
                    int conflict = INT_MAX;
                    loopj(numbones) if(bones[j].group < numbones && bones[j].scheduled < 0) conflict = min(conflict, abs(bones[j].group));
                    if(conflict < numbones)
                    {
                        bones[conflict].scheduled = schedule.length();
                        schedule.add(conflict);
                    }
                }
            }
        }

        void remapbones()
        {
            loopi(numbones) 
            {
                boneinfo &info = bones[i];
                info.interpindex = -1;
                info.ragdollindex = -1;
            }
            numgpubones = 0;
            loopv(users)
            {
                skelmeshgroup *group = users[i];
                loopvj(group->blendcombos)
                {
                    blendcombo &c = group->blendcombos[j];
                    loopk(4) 
                    {
                        if(!c.weights[k]) { c.interpbones[k] = k > 0 ? c.interpbones[k-1] : 0; continue; } 
                        boneinfo &info = bones[c.bones[k]];
                        if(info.interpindex < 0) info.interpindex = numgpubones++;
                        c.interpbones[k] = info.interpindex;
                        if(info.group < 0) continue;
                        loopl(4)
                        {
                            if(!c.weights[l]) break;
                            if(l == k) continue;
                            int parent = c.bones[l];
                            if(info.parent == parent || (info.parent >= 0 && info.parent == bones[parent].parent)) { info.group = -info.parent; break; }
                            if(info.group <= parent) continue;
                            int child = c.bones[k];
                            while(parent > child) parent = bones[parent].parent;
                            if(parent != child) info.group = c.bones[l];
                        }
                    }
                }
            }
            numinterpbones = numgpubones;
            loopv(tags)
            {
                boneinfo &info = bones[tags[i].bone];
                if(info.interpindex < 0) info.interpindex = numinterpbones++;
            }
            if(ragdoll)
            {
                loopv(ragdoll->joints) 
                {
                    boneinfo &info = bones[ragdoll->joints[i].bone];
                    if(info.interpindex < 0) info.interpindex = numinterpbones++;
                    info.ragdollindex = i;
                }
            }
            loopi(numbones)
            {
                boneinfo &info = bones[i];
                if(info.interpindex < 0) continue;
                for(int parent = info.parent; parent >= 0 && bones[parent].interpindex < 0; parent = bones[parent].parent)
                    bones[parent].interpindex = numinterpbones++;
            }
            loopi(numbones)
            {
                boneinfo &info = bones[i];
                if(info.interpindex < 0) continue;
                info.interpparent = info.parent >= 0 ? bones[info.parent].interpindex : -1;
            }
            if(ragdoll)
            {
                loopi(numbones)
                {
                    boneinfo &info = bones[i];
                    if(info.interpindex < 0 || info.ragdollindex >= 0) continue;
                    for(int parent = info.parent; parent >= 0; parent = bones[parent].parent)
                    {
                        if(bones[parent].ragdollindex >= 0) { ragdoll->addreljoint(i, bones[parent].ragdollindex); break; }
                    }
                }
            }
            calcantipodes();
        }

        void optimize()
        {
            DELETEA(invbones);
            DELETEA(matinvbones);
            DELETEA(matframebones);
            cleanup();
            if(ragdoll) ragdoll->setup();
            remapbones();
        }

        void expandbonemask(uchar *expansion, int bone, int val)
        {
            expansion[bone] = val;
            bone = bones[bone].children;
            while(bone>=0) { expandbonemask(expansion, bone, val); bone = bones[bone].next; }
        }

        void applybonemask(ushort *mask, uchar *partmask, int partindex)
        {
            if(!mask || *mask==BONEMASK_END) return;
            uchar *expansion = new uchar[numbones];
            memset(expansion, *mask&BONEMASK_NOT ? 1 : 0, numbones);
            while(*mask!=BONEMASK_END)
            {
                expandbonemask(expansion, *mask&BONEMASK_BONE, *mask&BONEMASK_NOT ? 0 : 1);
                mask++;
            }
            loopi(numbones) if(expansion[i]) partmask[i] = partindex;
            delete[] expansion;
        }

        void linkchildren()
        {
            loopi(numbones)
            {
                boneinfo &b = bones[i];
                b.children = -1;
                if(b.parent<0) b.next = -1;
                else
                {
                    b.next = bones[b.parent].children;
                    bones[b.parent].children = i;
                }
            }
        }

        int availgpubones() const { return (min(maxvpenvparams - reservevpparams, 256) - 10) / (matskel ? 3 : 2); }
        bool gpuaccelerate() const { return renderpath!=R_FIXEDFUNCTION && numframes && gpuskel && numgpubones<=availgpubones(); }

        void geninvbones()
        {
            if(invbones) return;
            invbones = new dualquat[numinterpbones];
            loopi(numbones)
            {
                boneinfo &info = bones[i];
                if(info.interpindex < 0) continue;
                invbones[info.interpindex] = dualquat(info.base).invert();
            }
        }

        void interpmatbones(const animstate *as, float pitch, const vec &axis, int numanimparts, uchar *partmask, skelcacheentry &sc)
        {
            if(!invbones) geninvbones();
            if(!matframebones)
            {
                matframebones = new matrix3x4[numframes*numbones];
                loopi(numframes*numbones) matframebones[i] = framebones[i];
            }
            if(!matinvbones)
            {
                matinvbones = new matrix3x4[numinterpbones];
                loopi(numinterpbones) matinvbones[i] = invbones[i];
            }
            if(!sc.mdata) sc.mdata = new matrix3x4[numinterpbones];
            if(lastsdata == sc.mdata) lastsdata = NULL;
            struct framedata
            {
                matrix3x4 *fr1, *fr2, *pfr1, *pfr2;
            } partframes[MAXANIMPARTS];
            loopi(numanimparts)
            {
                partframes[i].fr1 = &matframebones[as[i].cur.fr1*numbones];
                partframes[i].fr2 = &matframebones[as[i].cur.fr2*numbones];
                if(as[i].interp<1)
                {
                    partframes[i].pfr1 = &matframebones[as[i].prev.fr1*numbones];
                    partframes[i].pfr2 = &matframebones[as[i].prev.fr2*numbones];
                }
            }
            loopi(numbones) if(bones[i].interpindex>=0)
            {
                const animstate &s = as[partmask[i]];
                const framedata &f = partframes[partmask[i]];
                matrix3x4 m;
                (m = f.fr1[i]).scale((1-s.cur.t)*s.interp);
                m.accumulate(f.fr2[i], s.cur.t*s.interp);
                if(s.interp<1)
                {
                    m.accumulate(f.pfr1[i], (1-s.prev.t)*(1-s.interp));
                    m.accumulate(f.pfr2[i], s.prev.t*(1-s.interp));
                }
                const boneinfo &b = bones[i];
                if(b.interpparent<0) sc.mdata[b.interpindex] = m;
                else sc.mdata[b.interpindex].mul(sc.mdata[b.interpparent], m);
                if(b.pitchscale)
                {
                    float angle = b.pitchscale*pitch + b.pitchoffset;
                    if(b.pitchmin || b.pitchmax) angle = max(b.pitchmin, min(b.pitchmax, angle));
                    matrix3x3 rmat;
                    rmat.rotate(angle*RAD, axis);
                    sc.mdata[b.interpindex].mulorient(rmat);
                }
            }
            loopi(numinterpbones)
            {
                sc.mdata[i].normalize();
                sc.mdata[i].mul(matinvbones[i]);
            }
        }

        void interpbones(const animstate *as, float pitch, const vec &axis, int numanimparts, uchar *partmask, skelcacheentry &sc)
        {
            if(!invbones) geninvbones();
            if(!sc.bdata) sc.bdata = new dualquat[numinterpbones];
            if(lastsdata == sc.bdata) lastsdata = NULL;
            struct framedata
            {
                dualquat *fr1, *fr2, *pfr1, *pfr2;
            } partframes[MAXANIMPARTS];
            loopi(numanimparts)
            {
                partframes[i].fr1 = &framebones[as[i].cur.fr1*numbones];
                partframes[i].fr2 = &framebones[as[i].cur.fr2*numbones];
                if(as[i].interp<1)
                {
                    partframes[i].pfr1 = &framebones[as[i].prev.fr1*numbones];
                    partframes[i].pfr2 = &framebones[as[i].prev.fr2*numbones];
                }
            }
            loopi(numbones) if(bones[i].interpindex>=0)
            {
                const animstate &s = as[partmask[i]];
                const framedata &f = partframes[partmask[i]];
                dualquat d;
                (d = f.fr1[i]).mul((1-s.cur.t)*s.interp);
                d.accumulate(f.fr2[i], s.cur.t*s.interp);
                if(s.interp<1)
                {
                    d.accumulate(f.pfr1[i], (1-s.prev.t)*(1-s.interp));
                    d.accumulate(f.pfr2[i], s.prev.t*(1-s.interp));
                }
                const boneinfo &b = bones[i];
                if(b.interpparent<0) sc.bdata[b.interpindex] = d;
                else sc.bdata[b.interpindex].mul(sc.bdata[b.interpparent], d);
                if(b.pitchscale)
                {
                    float angle = b.pitchscale*pitch + b.pitchoffset;
                    if(b.pitchmin || b.pitchmax) angle = max(b.pitchmin, min(b.pitchmax, angle));
                    sc.bdata[b.interpindex].mulorient(quat(axis, angle*RAD));
                }
            }
            loopi(numinterpbones)
            {
                dualquat &d = sc.bdata[i];
                d.normalize();
                d.mul(invbones[i]);
            }
            loopv(antipodes) sc.bdata[antipodes[i].child].fixantipodal(sc.bdata[antipodes[i].parent]);
        }

        void initmatragdoll(ragdolldata &d, skelcacheentry &sc, part *p)
        {
            const matrix3x4 *mdata = sc.mdata;
            loopv(ragdoll->joints)
            {
                const ragdollskel::joint &j = ragdoll->joints[i];
                const boneinfo &b = bones[j.bone];
                const matrix3x4 &m = mdata[b.interpindex];
                loopk(3) if(j.vert[k] >= 0)
                {
                    int vert = j.vert[k];
                    vec pos;
                    matrixstack[matrixpos].transform(m.transform(ragdoll->verts[vert].pos).add(p->translate).mul(p->model->scale), pos);
                    d.verts[vert].pos.add(pos);
                }
            }
            loopv(ragdoll->reljoints)
            {
                const ragdollskel::reljoint &r = ragdoll->reljoints[i];
                const ragdollskel::joint &j = ragdoll->joints[r.parent];
                const boneinfo &br = bones[r.bone], &bj = bones[j.bone];
                d.reljoints[i].transposemul(mdata[bj.interpindex], mdata[br.interpindex]);
            }
            loopv(ragdoll->verts) d.verts[i].pos.mul(ragdoll->verts[i].weight);
        }

        void initragdoll(ragdolldata &d, skelcacheentry &sc, part *p)
        {
            const dualquat *bdata = sc.bdata;
            loopv(ragdoll->joints)
            {
                const ragdollskel::joint &j = ragdoll->joints[i];
                const boneinfo &b = bones[j.bone];
                const dualquat &q = bdata[b.interpindex];
                loopk(3) if(j.vert[k] >= 0)
                {
                    int vert = j.vert[k];
                    vec pos;
                    matrixstack[matrixpos].transform(q.transform(ragdoll->verts[vert].pos).add(p->translate).mul(p->model->scale), pos);
                    d.verts[vert].pos.add(pos);
                }
            }
            loopv(ragdoll->reljoints)
            {
                const ragdollskel::reljoint &r = ragdoll->reljoints[i];
                const ragdollskel::joint &j = ragdoll->joints[r.parent];
                const boneinfo &br = bones[r.bone], &bj = bones[j.bone];
                dualquat q = bdata[bj.interpindex];
                q.invert().mul(bdata[br.interpindex]);
                d.reljoints[i] = matrix3x4(q);
            }
            loopv(ragdoll->verts) d.verts[i].pos.mul(ragdoll->verts[i].weight);
        }

        void genmatragdollbones(ragdolldata &d, skelcacheentry &sc, part *p)
        {
            if(!sc.mdata) sc.mdata = new matrix3x4[numinterpbones];
            if(lastsdata == sc.mdata) lastsdata = NULL;
            loopv(ragdoll->joints)
            {
                const ragdollskel::joint &j = ragdoll->joints[i];
                const boneinfo &b = bones[j.bone];
                vec pos(0, 0, 0);
                loopk(3) if(j.vert[k]>=0) pos.add(d.verts[j.vert[k]].pos);
                pos.mul(j.weight/p->model->scale).sub(p->translate);
                sc.mdata[b.interpindex].transposemul(d.tris[j.tri], pos, j.orient);
            }
            loopv(ragdoll->reljoints)
            {
                const ragdollskel::reljoint &r = ragdoll->reljoints[i];
                const ragdollskel::joint &j = ragdoll->joints[r.parent];
                const boneinfo &br = bones[r.bone], &bj = bones[j.bone];
                sc.mdata[br.interpindex].mul(sc.mdata[bj.interpindex], d.reljoints[i]);
            }
        }

        void genragdollbones(ragdolldata &d, skelcacheentry &sc, part *p)
        {
            if(!sc.bdata) sc.bdata = new dualquat[numinterpbones];
            if(lastsdata == sc.bdata) lastsdata = NULL;
            loopv(ragdoll->joints)
            {
                const ragdollskel::joint &j = ragdoll->joints[i];
                const boneinfo &b = bones[j.bone];
                vec pos(0, 0, 0);
                loopk(3) if(j.vert[k]>=0) pos.add(d.verts[j.vert[k]].pos);
                pos.mul(j.weight/p->model->scale).sub(p->translate);
                matrix3x4 m;
                m.transposemul(d.tris[j.tri], pos, j.orient);
                sc.bdata[b.interpindex] = dualquat(m);
            }
            loopv(ragdoll->reljoints)
            {
                const ragdollskel::reljoint &r = ragdoll->reljoints[i];
                const ragdollskel::joint &j = ragdoll->joints[r.parent];
                const boneinfo &br = bones[r.bone], &bj = bones[j.bone];
                sc.bdata[br.interpindex].mul(sc.bdata[bj.interpindex], dualquat(d.reljoints[i]));
            }
            loopv(antipodes) sc.bdata[antipodes[i].child].fixantipodal(sc.bdata[antipodes[i].parent]);
        }

        void concattagtransform(part *p, int frame, int i, const matrix3x4 &m, matrix3x4 &n)
        {
            matrix3x4 t = bones[tags[i].bone].base;
            t.translate(vec(p->translate).mul(p->model->scale));
            n.mul(m, t);
        }

        void calctagmatrix(part *p, int bone, const matrix3x4 &m, linkedpart &l)
        {
            if(numframes) 
            {
                matrix3x4 t;
                t.mul(m, bones[bone].base); 
                l.matrix = t;
            }
            else l.matrix = m;
            l.matrix[12] = (l.matrix[12] + p->translate.x) * p->model->scale;
            l.matrix[13] = (l.matrix[13] + p->translate.y) * p->model->scale;
            l.matrix[14] = (l.matrix[14] + p->translate.z) * p->model->scale;
        }

        void calctags(skelcacheentry &sc, part *p)
        {
            loopv(p->links)
            {
                int tagbone = tags[p->links[i].tag].bone, interpindex = bones[tagbone].interpindex;
                calctagmatrix(p, tagbone, usematskel ? sc.mdata[interpindex] : sc.bdata[interpindex], p->links[i]);
            }
        }

        void calctags(part *p)
        {
            loopv(p->links)
            {
               int tagbone = tags[p->links[i].tag].bone;
               calctagmatrix(p, tagbone, bones[tagbone].base, p->links[i]);
            }
        }

        void cleanup()
        {
            loopv(skelcache)
            {
                skelcacheentry &sc = skelcache[i];
                loopj(MAXANIMPARTS) sc.as[j].cur.fr1 = -1;
                DELETEA(sc.bdata);
                DELETEA(sc.mdata);
            }
            skelcache.setsizenodelete(0);
            lastsdata = lastbdata = NULL;
            loopv(users) users[i]->cleanup();
        }

        skelcacheentry &checkskelcache(part *p, const animstate *as, float pitch, const vec &axis, ragdolldata *rdata)
        {
            if(skelcache.empty()) 
            {
                usegpuskel = gpuaccelerate();
                usematskel = matskel!=0;
            }

            int numanimparts = ((skelpart *)as->owner)->numanimparts;
            uchar *partmask = ((skelpart *)as->owner)->partmask;
            skelcacheentry *sc = NULL;
            bool match = false;
            loopv(skelcache)
            {
                skelcacheentry &c = skelcache[i];
                loopj(numanimparts) if(c.as[j]!=as[j]) goto mismatch;
                if(c.pitch != pitch || c.partmask != partmask || c.ragdoll != rdata || (rdata && c.millis < rdata->lastmove)) goto mismatch;
                match = true;
                sc = &c;
                break;
            mismatch:
                if(c.millis < lastmillis) { sc = &c; break; }
            }
            if(!sc) sc = &skelcache.add();
            if(!match)
            {
                loopi(numanimparts) sc->as[i] = as[i];
                sc->pitch = pitch;
                sc->partmask = partmask;
                sc->ragdoll = rdata;
                if(rdata)
                {
                    if(matskel) genmatragdollbones(*rdata, *sc, p);
                    else genragdollbones(*rdata, *sc, p);
                }
                else if(matskel) interpmatbones(as, pitch, axis, numanimparts, partmask, *sc);
                else interpbones(as, pitch, axis, numanimparts, partmask, *sc);
            }
            sc->millis = lastmillis;
            return *sc;
        }

        void setgpubones(skelcacheentry &sc, int count = 0)
        {
            if((count ? lastbdata : lastsdata) == (usematskel ? (void *)sc.mdata : (void *)sc.bdata)) return;
            int offset = count ? numgpubones : 0;
            if(!offset) count = numgpubones;
            if(hasPP)
            {
                if(usematskel) glProgramEnvParameters4fv_(GL_VERTEX_PROGRAM_ARB, 10 + 3*offset, 3*count, sc.mdata[0].a.v);
                else glProgramEnvParameters4fv_(GL_VERTEX_PROGRAM_ARB, 10 + 2*offset, 2*count, sc.bdata[0].real.v);
            }
            else if(usematskel) loopi(count)
            {
                glProgramEnvParameter4fv_(GL_VERTEX_PROGRAM_ARB, 10 + 3*(offset+i), sc.mdata[i].a.v);
                glProgramEnvParameter4fv_(GL_VERTEX_PROGRAM_ARB, 11 + 3*(offset+i), sc.mdata[i].b.v);
                glProgramEnvParameter4fv_(GL_VERTEX_PROGRAM_ARB, 12 + 3*(offset+i), sc.mdata[i].c.v);
            }
            else loopi(count)
            {
                glProgramEnvParameter4fv_(GL_VERTEX_PROGRAM_ARB, 10 + 2*(offset+i), sc.bdata[i].real.v);
                glProgramEnvParameter4fv_(GL_VERTEX_PROGRAM_ARB, 11 + 2*(offset+i), sc.bdata[i].dual.v);
            }
            if(offset) lastbdata = usematskel ? (void *)sc.mdata : (void *)sc.bdata;
            else lastsdata = usematskel ? (void *)sc.mdata : (void *)sc.bdata;
        }

        bool shouldcleanup() const
        {
            return numframes && (skelcache.empty() || gpuaccelerate()!=usegpuskel || (matskel!=0)!=usematskel);
        }
    };

    struct skelmeshgroup : meshgroup
    {
        skeleton *skel;

        vector<blendcombo> blendcombos;
        int numblends[4];

        static const int MAXBLENDCACHE = 16;
        blendcacheentry blendcache[MAXBLENDCACHE];

        static const int MAXVBOCACHE = 16;
        vbocacheentry vbocache[MAXVBOCACHE];
 
        ushort *edata;
        GLuint ebuf;
        bool vnorms, vtangents;
        int vlen, vertsize, vblends, vweights;
        uchar *vdata;

        skelmeshgroup() : skel(NULL), edata(NULL), ebuf(0), vdata(NULL)
        {
            memset(numblends, 0, sizeof(numblends));
        }

        virtual ~skelmeshgroup()
        {
            if(skel)
            {
                if(skel->shared) skel->users.removeobj(this);
                else DELETEP(skel);
            }
            if(ebuf) glDeleteBuffers_(1, &ebuf);
            loopi(MAXBLENDCACHE)
            {
                DELETEA(blendcache[i].bdata);
                DELETEA(blendcache[i].mdata);
            }
            loopi(MAXVBOCACHE)
            {
                DELETEA(vbocache[i].vdata);
                if(vbocache[i].vbuf) glDeleteBuffers_(1, &vbocache[i].vbuf);
            }
            DELETEA(vdata);
        }

        void shareskeleton(char *name)
        {
            if(!name)
            {
                skel = new skeleton;
                skel->users.add(this);
                return;
            }

            static hashtable<char *, skeleton *> skeletons;
            if(skeletons.access(name)) skel = skeletons[name];
            else
            {
                skel = new skeleton;
                skel->name = newstring(name);
                skeletons[skel->name] = skel;
            }
            skel->users.add(this);
            skel->shared++;
        }

        int findtag(const char *name)
        {
            return skel->findtag(name);
        }

        int totalframes() const { return max(skel->numframes, 1); }

        void genvbo(bool norms, bool tangents, vbocacheentry &vc)
        {
            if(hasVBO)
            {
                if(!vc.vbuf) glGenBuffers_(1, &vc.vbuf);
                if(ebuf) return;
            }
            else if(edata)
            {
                #define ALLOCVDATA(vdata) \
                    do \
                    { \
                        DELETEA(vdata); \
                        vdata = new uchar[vlen*vertsize]; \
                        loopv(meshes) \
                        { \
                            skelmesh &m = *(skelmesh *)meshes[i]; \
                            m.filltc(vdata, vertsize); \
                            if(tangents) m.fillbump(vdata, vertsize); \
                        } \
                    } while(0)
                if(!vc.vdata) ALLOCVDATA(vc.vdata);
                return;
            }

            vector<ushort> idxs;

            vnorms = norms;
            vtangents = tangents;
            vlen = 0;
            vblends = 0;
            if(skel->numframes && !skel->usegpuskel)
            {
                vweights = 1;
                loopv(blendcombos)
                {
                    blendcombo &c = blendcombos[i];
                    c.interpindex = c.weights[1] ? skel->numinterpbones + vblends++ : -1;
                }

                vertsize = tangents ? sizeof(vvertbump) : (norms ? sizeof(vvertn) : sizeof(vvert));
                loopv(meshes) vlen += ((skelmesh *)meshes[i])->genvbo(idxs, vlen);
                DELETEA(vdata);
                if(hasVBO) ALLOCVDATA(vdata);
                else ALLOCVDATA(vc.vdata);
            }
            else
            {
                if(skel->numframes)
                {
                    vweights = 4;
                    int availbones = skel->availgpubones() - skel->numgpubones;
                    while(vweights > 1 && availbones >= numblends[vweights-1]) availbones -= numblends[--vweights];
                    loopv(blendcombos)
                    {
                        blendcombo &c = blendcombos[i];
                        c.interpindex = c.size() > vweights ? skel->numgpubones + vblends++ : -1;
                    }
                }
                else
                {
                    vweights = 0;
                    loopv(blendcombos) blendcombos[i].interpindex = -1;
                }

                if(hasVBO) glBindBuffer_(GL_ARRAY_BUFFER_ARB, vc.vbuf);
                #define GENVBO(type) \
                    do \
                    { \
                        vertsize = sizeof(type); \
                        vector<type> vverts; \
                        loopv(meshes) vlen += ((skelmesh *)meshes[i])->genvbo(idxs, vlen, vverts); \
                        if(hasVBO) glBufferData_(GL_ARRAY_BUFFER_ARB, vverts.length()*sizeof(type), vverts.getbuf(), GL_STATIC_DRAW_ARB); \
                        else \
                        { \
                            DELETEA(vc.vdata); \
                            vc.vdata = new uchar[vverts.length()*sizeof(type)]; \
                            memcpy(vc.vdata, vverts.getbuf(), vverts.length()*sizeof(type)); \
                        } \
                    } while(0)
                if(skel->numframes)
                {
                    if(tangents) GENVBO(vvertbumpw);
                    else GENVBO(vvertw);
                }
                else if(tangents) GENVBO(vvertbump);
                else if(norms) GENVBO(vvertn);
                else GENVBO(vvert);
            }

            if(hasVBO)
            {
                glGenBuffers_(1, &ebuf);
                glBindBuffer_(GL_ELEMENT_ARRAY_BUFFER_ARB, ebuf);
                glBufferData_(GL_ELEMENT_ARRAY_BUFFER_ARB, idxs.length()*sizeof(ushort), idxs.getbuf(), GL_STATIC_DRAW_ARB);
            }
            else
            {
                edata = new ushort[idxs.length()];
                memcpy(edata, idxs.getbuf(), idxs.length()*sizeof(ushort));
            }
            #undef GENVBO
            #undef ALLOCVDATA
        }

        void bindvbo(const animstate *as, vbocacheentry &vc, skelcacheentry *sc = NULL, blendcacheentry *bc = NULL)
        {
            vvertn *vverts = hasVBO ? 0 : (vvertn *)vc.vdata;
            if(hasVBO && lastebuf!=ebuf)
            {
                glBindBuffer_(GL_ELEMENT_ARRAY_BUFFER_ARB, ebuf);
                lastebuf = ebuf;
            }
            if(lastvbuf != (hasVBO ? (void *)(size_t)vc.vbuf : vc.vdata))
            {
                if(hasVBO) glBindBuffer_(GL_ARRAY_BUFFER_ARB, vc.vbuf);
                if(!lastvbuf) glEnableClientState(GL_VERTEX_ARRAY);
                glVertexPointer(3, GL_FLOAT, vertsize, &vverts->pos);
                lastvbuf = hasVBO ? (void *)(size_t)vc.vbuf : vc.vdata;
            }
            if(as->anim&ANIM_NOSKIN)
            {
                if(enabletc) disabletc();
                if(enablenormals) disablenormals();
            }
            else
            {
                if(vnorms || vtangents)
                {
                    if(!enablenormals)
                    {
                        glEnableClientState(GL_NORMAL_ARRAY);
                        enablenormals = true;
                    }
                    if(lastnbuf!=lastvbuf)
                    {
                        glNormalPointer(GL_FLOAT, vertsize, &vverts->norm);
                        lastnbuf = lastvbuf;
                    }
                }
                else if(enablenormals) disablenormals();

                if(!enabletc)
                {
                    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
                    enabletc = true;
                }
                if(lasttcbuf!=lastvbuf)
                {
                    glTexCoordPointer(2, GL_FLOAT, vertsize, &vverts->u);
                    lasttcbuf = lastnbuf;
                }
            }
            if(!sc || !skel->usegpuskel)
            {
                if(enablebones) disablebones();
                return;
            }
            if(!enablebones)
            {
                glEnableVertexAttribArray_(6);
                glEnableVertexAttribArray_(7);
                enablebones = true;
            }
            if(lastbbuf!=lastvbuf)
            {
                glVertexAttribPointer_(6, 4, GL_UNSIGNED_BYTE, GL_TRUE, vertsize, &((vvertw *)vverts)->weights);
                glVertexAttribPointer_(7, 4, GL_UNSIGNED_BYTE, GL_FALSE, vertsize, &((vvertw *)vverts)->bones);
                lastbbuf = lastvbuf;
            }
            skel->setgpubones(*sc);
            if(bc && vblends) skel->setgpubones(*bc, vblends);
        }

        void concattagtransform(part *p, int frame, int i, const matrix3x4 &m, matrix3x4 &n)
        {
            skel->concattagtransform(p, frame, i, m, n);
        }

        int addblendcombo(const blendcombo &c)
        {
            loopv(blendcombos) if(blendcombos[i]==c)
            {
                blendcombos[i].uses += c.uses;
                return i;
            }
            numblends[c.size()-1]++;
            blendcombo &a = blendcombos.add(c);
            return a.interpindex = blendcombos.length()-1; 
        }

        void sortblendcombos()
        {
            blendcombos.sort(blendcombo::sortcmp);
            int *remap = new int[blendcombos.length()];
            loopv(blendcombos) remap[blendcombos[i].interpindex] = i;
            loopv(meshes)
            {
                skelmesh *m = (skelmesh *)meshes[i];
                loopj(m->numverts)
                {
                    vert &v = m->verts[j];
                    v.blend = remap[v.blend];
                }
            }
            delete[] remap;
        }

        int remapblend(int blend)
        {
            const blendcombo &c = blendcombos[blend];
            return c.weights[1] ? c.interpindex : c.interpbones[0];
        }

        void blendmatbones(const skelcacheentry &sc, blendcacheentry &bc)
        {
            if(!bc.mdata) bc.mdata = new matrix3x4[vblends];
            if(lastbdata == bc.mdata) lastbdata = NULL;
            matrix3x4 *dst = bc.mdata - (skel->usegpuskel ? skel->numgpubones : skel->numinterpbones);
            loopv(blendcombos)
            {
                const blendcombo &c = blendcombos[i];
                if(c.interpindex<0) break;
                matrix3x4 &m = dst[c.interpindex];
                m = sc.mdata[c.interpbones[0]];
                m.scale(c.weights[0]);
                m.accumulate(sc.mdata[c.interpbones[1]], c.weights[1]);
                if(c.weights[2])
                {
                    m.accumulate(sc.mdata[c.interpbones[2]], c.weights[2]);
                    if(c.weights[3]) m.accumulate(sc.mdata[c.interpbones[3]], c.weights[3]);
                }
            }
        }

        void blendbones(const skelcacheentry &sc, blendcacheentry &bc)
        {
            if(!bc.bdata) bc.bdata = new dualquat[vblends];
            if(lastbdata == bc.bdata) lastbdata = NULL;
            dualquat *dst = bc.bdata - (skel->usegpuskel ? skel->numgpubones : skel->numinterpbones);
            bool normalize = !skel->usegpuskel || vweights<=1;
            loopv(blendcombos)
            {
                const blendcombo &c = blendcombos[i];
                if(c.interpindex<0) break;
                dualquat &d = dst[c.interpindex];
                d = sc.bdata[c.interpbones[0]];
                d.mul(c.weights[0]);
                d.accumulate(sc.bdata[c.interpbones[1]], c.weights[1]);
                if(c.weights[2])
                {
                    d.accumulate(sc.bdata[c.interpbones[2]], c.weights[2]);
                    if(c.weights[3]) d.accumulate(sc.bdata[c.interpbones[3]], c.weights[3]);
                }
                if(normalize) d.normalize();
            }
        }

        void cleanup()
        {
            loopi(MAXBLENDCACHE)
            {
                blendcacheentry &c = blendcache[i];
                DELETEA(c.bdata);
                DELETEA(c.mdata);
                c.owner = -1;
            }
            loopi(MAXVBOCACHE)
            {
                vbocacheentry &c = vbocache[i];
                if(c.vbuf) { glDeleteBuffers_(1, &c.vbuf); c.vbuf = 0; }
                DELETEA(c.vdata);
                c.owner = -1;
            }
            if(hasVBO) { if(ebuf) { glDeleteBuffers_(1, &ebuf); ebuf = 0; } }
            else DELETEA(vdata);
        }

        #define SEARCHCACHE(cachesize, cacheentry, cache, reusecheck) \
            loopi(cachesize) \
            { \
                cacheentry &c = cache[i]; \
                if(c.owner==owner) \
                { \
                     if(c==sc) return c; \
                     else c.owner = -1; \
                     break; \
                } \
            } \
            loopi(cachesize-1) \
            { \
                cacheentry &c = cache[i]; \
                if(reusecheck c.owner < 0 || c.millis < lastmillis) \
                    return c; \
            } \
            return cache[cachesize-1];

        vbocacheentry &checkvbocache(skelcacheentry &sc, int owner)
        {
            SEARCHCACHE(MAXVBOCACHE, vbocacheentry, vbocache, (hasVBO ? !c.vbuf : !c.vdata) || );
        }

        blendcacheentry &checkblendcache(skelcacheentry &sc, int owner)
        {
            SEARCHCACHE(MAXBLENDCACHE, blendcacheentry, blendcache, )
        }

        void render(const animstate *as, float pitch, const vec &axis, dynent *d, part *p)
        {
            bool norms = false, tangents = false;
            loopv(p->skins)
            {
                if(p->skins[i].normals()) norms = true;
                if(p->skins[i].tangents()) tangents = true;
            }
            if(skel->shouldcleanup()) { skel->cleanup(); disablevbo(); }
            else if(norms!=vnorms || tangents!=vtangents) { cleanup(); disablevbo(); }

            if(!skel->numframes)
            {
                if(!(as->anim&ANIM_NORENDER))
                {
                    if(hasVBO ? !vbocache->vbuf : !vbocache->vdata) genvbo(norms, tangents, *vbocache);
                    bindvbo(as, *vbocache);
                    loopv(meshes) ((skelmesh *)meshes[i])->render(as, p->skins[i], *vbocache);
                }
                skel->calctags(p);
                return;
            }

            skelcacheentry &sc = skel->checkskelcache(p, as, pitch, axis, as->anim&ANIM_RAGDOLL || !d || !d->ragdoll || d->ragdoll->skel != skel->ragdoll ? NULL : d->ragdoll);
            if(!(as->anim&ANIM_NORENDER))
            {
                int owner = &sc-&skel->skelcache[0];
                vbocacheentry &vc = skel->usegpuskel ? *vbocache : checkvbocache(sc, owner);
                vc.millis = lastmillis;
                if(hasVBO ? !vc.vbuf : !vc.vdata) genvbo(norms, tangents, vc);
                blendcacheentry *bc = NULL;
                if(vblends)
                {
                    bc = &checkblendcache(sc, owner);
                    bc->millis = lastmillis;
                    if(bc->owner!=owner)
                    {
                        bc->owner = owner;
                        *(animcacheentry *)bc = sc;
                        if(skel->usematskel) blendmatbones(sc, *bc);
                        else blendbones(sc, *bc);
                    }
                }
                if(!skel->usegpuskel && vc.owner!=owner)
                { 
                    vc.owner = owner;
                    (animcacheentry &)vc = sc;
                    loopv(meshes)
                    {
                        skelmesh &m = *(skelmesh *)meshes[i];
                        if(skel->usematskel) m.interpmatverts(sc, bc, norms, tangents, (hasVBO ? vdata : vc.vdata) + m.voffset*vertsize, p->skins[i]);
                        else m.interpverts(sc, bc, norms, tangents, (hasVBO ? vdata : vc.vdata) + m.voffset*vertsize, p->skins[i]);
                    }
                    if(hasVBO)
                    {
                        glBindBuffer_(GL_ARRAY_BUFFER_ARB, vc.vbuf);
                        glBufferData_(GL_ARRAY_BUFFER_ARB, vlen*vertsize, vdata, GL_STREAM_DRAW_ARB);
                    }
                }

                bindvbo(as, vc, &sc, bc);
                loopv(meshes) ((skelmesh *)meshes[i])->render(as, p->skins[i], vc);
            }

            skel->calctags(sc, p);

            if(as->anim&ANIM_RAGDOLL && skel->ragdoll && !d->ragdoll)
            {
                d->ragdoll = new ragdolldata(skel->ragdoll, p->model->scale);
                if(matskel) skel->initmatragdoll(*d->ragdoll, sc, p);
                else skel->initragdoll(*d->ragdoll, sc, p);
                d->ragdoll->init(d);
            }
        }
    };

    struct animpartmask
    {
        animpartmask *next;
        int numbones;
        uchar bones[1];
    };

    struct skelpart : part
    {
        animpartmask *buildingpartmask;

        uchar *partmask;
        
        skelpart() : buildingpartmask(NULL), partmask(NULL)
        {
        }

        virtual ~skelpart()
        {
            DELETEA(buildingpartmask);
        }

        uchar *sharepartmask(animpartmask *o)
        {
            static animpartmask *partmasks = NULL;
            animpartmask *p = partmasks;
            for(; p; p = p->next) if(p->numbones==o->numbones && !memcmp(p->bones, o->bones, p->numbones))
            {
                delete[] (uchar *)o;
                return p->bones;
            }

            o->next = p;
            partmasks = o;
            return o->bones;
        }

        animpartmask *newpartmask()
        {
            animpartmask *p = (animpartmask *)new uchar[sizeof(animpartmask) + ((skelmeshgroup *)meshes)->skel->numbones-1];
            p->numbones = ((skelmeshgroup *)meshes)->skel->numbones;
            memset(p->bones, 0, p->numbones);
            return p;
        }

        void initanimparts()
        {
            DELETEA(buildingpartmask);
            buildingpartmask = newpartmask();
        }

        bool addanimpart(ushort *bonemask)
        {
            if(!buildingpartmask || numanimparts>=MAXANIMPARTS) return false;
            ((skelmeshgroup *)meshes)->skel->applybonemask(bonemask, buildingpartmask->bones, numanimparts);
            numanimparts++;
            return true;
        }

        void endanimparts()
        {
            if(buildingpartmask)
            {
                partmask = sharepartmask(buildingpartmask);
                buildingpartmask = NULL;
            }

            ((skelmeshgroup *)meshes)->skel->optimize();
        }
    };

    skelmodel(const char *name) : animmodel(name)
    {
    }

    int linktype(animmodel *m) const
    {
        return type()==m->type() &&
            ((skelmeshgroup *)parts[0]->meshes)->skel == ((skelmeshgroup *)m->parts[0]->meshes)->skel ? 
                LINK_REUSE : 
                LINK_TAG;
    }
};

