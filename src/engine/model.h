enum { MDL_MD2 = 1, MDL_MD3, MDL_MD5, MDL_OBJ };

struct model
{
    float spinyaw, spinpitch, offsetyaw, offsetpitch;
    bool collide, ellipsecollide, shadow, alphadepth;
    float scale;
    vec translate;
    BIH *bih;
    vec bbcenter, bbradius, bbextend;
    float eyeheight, collideradius, collideheight;
    int batch;

    // INTENSITY: Adding this, so we can have models that check collisions, but only for triggering events,
    // and not actual collisions. I.e., to check if someone passes through a collision box, but not prevent
    // them from passing through.
    bool collisionsonlyfortriggering;

    bool perentitycollisionboxes; // INTENSITY: Get the collision box from the entiy, not the model type

    model() : spinyaw(0), spinpitch(0), offsetyaw(0), offsetpitch(0), collide(true), ellipsecollide(false), shadow(true), alphadepth(true), scale(1.0f), translate(0, 0, 0), bih(0), bbcenter(0, 0, 0), bbradius(0, 0, 0), bbextend(0, 0, 0), eyeheight(0.9f), collideradius(0), collideheight(0), batch(-1)
          , collisionsonlyfortriggering(false), perentitycollisionboxes(false)
        {}
    virtual ~model() { DELETEP(bih); }
    virtual void calcbb(int frame, vec &center, vec &radius) = 0;
    virtual void render(int anim, int basetime, int basetime2, const vec &o, float yaw, float pitch, float roll, dynent *d, modelattach *a = NULL, const vec &color = vec(0, 0, 0), const vec &dir = vec(0, 0, 0), float transparent = 1, const quat &rotation=quat(0,0,0,0)) = 0; // INTENSITY: roll, rotation
    virtual bool load() = 0;
    virtual char *name() = 0;
    virtual int type() const = 0;
    virtual BIH *setBIH() { return 0; }
    virtual void gentris(int frame, vector<BIH::tri> *tris) { } // INTENSITY: Made this 'public' by putting it here
    virtual bool envmapped() { return false; }

    virtual void setshader(Shader *shader) {}
    virtual void setenvmap(float envmapmin, float envmapmax, Texture *envmap) {}
    virtual void setspec(float spec) {}
    virtual void setambient(float ambient) {}
    virtual void setglow(float glow) {}
    virtual void setglare(float specglare, float glowglare) {}
    virtual void setalphatest(float alpha) {}
    virtual void setalphablend(bool blend) {}
    virtual void setfullbright(float fullbright) {}
    virtual void setcullface(bool cullface) {}

    virtual void preloadshaders() {}
    virtual void cleanup() {}

    virtual void startrender() {}
    virtual void endrender() {}

    void boundbox(int frame, vec &center, vec &radius, CLogicEntity* entity=0) // INTENSITY: Added entity
    {
        if (perentitycollisionboxes && entity) { perentitybox(frame, center, radius, entity); return; } // INTENSITY

        if(frame) calcbb(frame, center, radius);
        else
        {
            if(bbradius.iszero()) calcbb(0, bbcenter, bbradius);
            center = bbcenter;
            radius = bbradius;
        }
        radius.add(bbextend);
    }

    void collisionbox(int frame, vec &center, vec &radius, CLogicEntity* entity=0) // INTENSITY: Added entity
    {
        if (perentitycollisionboxes && entity) { perentitybox(frame, center, radius, entity); return; } // INTENSITY

        boundbox(frame, center, radius);
        if(collideradius)
        {
            center[0] = center[1] = 0;
            radius[0] = radius[1] = collideradius;
        }
        if(collideheight)
        {
            center[2] = radius[2] = collideheight/2;
        }
    }

    float boundsphere(int frame, vec &center)
    {
        vec radius;
        boundbox(frame, center, radius);
        return radius.magnitude();
    }

    float above(int frame = 0)
    {
        vec center, radius;
        boundbox(frame, center, radius);
        return center.z+radius.z;
    }

    // INTENSITY: New function. A collision/bounding box that uses per-entity info
    void perentitybox(int frame, vec &center, vec &radius, CLogicEntity* entity=0)
    {
        assert(entity); // We should never be called without the parameter. It has a defaultvalue
                        // just in order to not need to change sauer code in irrelevant places

        float width = entity->collisionRadiusWidth;
        float height = entity->collisionRadiusHeight;

        if (width < 0) // If never loaded, load once from Python now. This occurs once per instance.
                        // This is necessary because these are values cached from Scripting, unlike normal
                        // Sauer C++ variables that are managed in C++. Here, the *true* values are in Scripting
        {
            width = entity->scriptEntity->getPropertyFloat("collisionRadiusWidth");
            height = entity->scriptEntity->getPropertyFloat("collisionRadiusHeight");
        }

        center[0] = center[1] = 0;
        radius[0] = radius[1] = width;
        center[2] = radius[2] = height;
    }
};

