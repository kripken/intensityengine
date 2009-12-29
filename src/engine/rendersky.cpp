#include "engine.h"

Texture *sky[6] = { 0, 0, 0, 0, 0, 0 }, *clouds[6] = { 0, 0, 0, 0, 0, 0 };

void loadsky(const char *basename, Texture *texs[6])
{
    loopi(6)
    {
        const char *side = cubemapsides[i].name;
        defformatstring(name)("%s_%s.jpg", makerelpath("packages", basename), side);
        if((texs[i] = textureload(name, 3, true, false))==notexture)
        {
            strcpy(name+strlen(name)-3, "png");
            if((texs[i] = textureload(name, 3, true, false))==notexture) conoutf(CON_ERROR, "could not load sky texture packages/%s_%s", basename, side);
        }
    }
}

Texture *cloudoverlay = NULL;

Texture *loadskyoverlay(const char *basename)
{
    defformatstring(name)("%s.jpg", makerelpath("packages", basename));
    Texture *t = textureload(name, 0, true, false);
    if(t!=notexture) return t;
    strcpy(name+strlen(name)-3, "png");
    t = textureload(name, 0, true, false);
    if(t==notexture) conoutf(CON_ERROR, "could not load sky overlay texture packages/%s", basename);
    return t;
}

SVARFR(skybox, "", { if(skybox[0]) loadsky(skybox, sky); }); 
FVARR(spinsky, -720, 0, 720);
VARR(yawsky, 0, 0, 360);
SVARFR(cloudbox, "", { if(cloudbox[0]) loadsky(cloudbox, clouds); });
FVARR(spinclouds, -720, 0, 720);
VARR(yawclouds, 0, 0, 360);
FVARR(cloudclip, 0, 0.5f, 1);
SVARFR(cloudlayer, "", { if(cloudlayer[0]) cloudoverlay = loadskyoverlay(cloudlayer); });
FVARR(cloudscrollx, -16, 0, 16);
FVARR(cloudscrolly, -16, 0, 16);
FVARR(cloudscale, 0, 1, 64);
FVARR(spincloudlayer, -720, 0, 720);
VARR(yawcloudlayer, 0, 0, 360);
FVARR(cloudheight, -1, 0.2f, 1);
FVARR(cloudfade, 0, 0.2f, 1);
FVARR(cloudalpha, 0, 1, 1);
VARR(cloudsubdiv, 4, 16, 64);
HVARR(cloudcolour, 0, 0xFFFFFF, 0xFFFFFF);

void draw_envbox_face(float s0, float t0, int x0, int y0, int z0,
                      float s1, float t1, int x1, int y1, int z1,
                      float s2, float t2, int x2, int y2, int z2,
                      float s3, float t3, int x3, int y3, int z3,
                      GLuint texture)
{
    glBindTexture(GL_TEXTURE_2D, texture);
    glBegin(GL_QUADS);
    glTexCoord2f(s3, t3); glVertex3f(x3, y3, z3);
    glTexCoord2f(s2, t2); glVertex3f(x2, y2, z2);
    glTexCoord2f(s1, t1); glVertex3f(x1, y1, z1);
    glTexCoord2f(s0, t0); glVertex3f(x0, y0, z0);
    glEnd();
    xtraverts += 4;
}

void draw_envbox(int w, float zclip = 0.0f, int faces = 0x3F, Texture **sky = NULL)
{
    float vclip = 1-zclip;
    int z = int(ceil(2*w*(vclip-0.5f)));

    if(faces&0x01)
        draw_envbox_face(0.0f, 0.0f, -w, -w, -w,
                         1.0f, 0.0f, -w,  w, -w,
                         1.0f, vclip, -w,  w,  z,
                         0.0f, vclip, -w, -w,  z, sky[0] ? sky[0]->id : notexture->id);

    if(faces&0x02)
        draw_envbox_face(1.0f, vclip, +w, -w,  z,
                         0.0f, vclip, +w,  w,  z,
                         0.0f, 0.0f, +w,  w, -w,
                         1.0f, 0.0f, +w, -w, -w, sky[1] ? sky[1]->id : notexture->id);

    if(faces&0x04)
        draw_envbox_face(1.0f, vclip, -w, -w,  z,
                         0.0f, vclip,  w, -w,  z,
                         0.0f, 0.0f,  w, -w, -w,
                         1.0f, 0.0f, -w, -w, -w, sky[2] ? sky[2]->id : notexture->id);

    if(faces&0x08)
        draw_envbox_face(1.0f, vclip, +w,  w,  z,
                         0.0f, vclip, -w,  w,  z,
                         0.0f, 0.0f, -w,  w, -w,
                         1.0f, 0.0f, +w,  w, -w, sky[3] ? sky[3]->id : notexture->id);

    if(!zclip && faces&0x10)
        draw_envbox_face(0.0f, 1.0f, -w,  w,  w,
                         0.0f, 0.0f, +w,  w,  w,
                         1.0f, 0.0f, +w, -w,  w,
                         1.0f, 1.0f, -w, -w,  w, sky[4] ? sky[4]->id : notexture->id);

    if(faces&0x20)
        draw_envbox_face(0.0f, 1.0f, +w,  w, -w,
                         0.0f, 0.0f, -w,  w, -w,
                         1.0f, 0.0f, -w, -w, -w,
                         1.0f, 1.0f, +w, -w, -w, sky[5] ? sky[5]->id : notexture->id);
}

void draw_env_overlay(int w, Texture *overlay = NULL, float tx = 0, float ty = 0)
{
    float z = -w*cloudheight, tsz = 0.5f*(1-cloudfade)/cloudscale, psz = w*(1-cloudfade);
    glBindTexture(GL_TEXTURE_2D, overlay ? overlay->id : notexture->id);
    float r = (cloudcolour>>16)/255.0f, g = ((cloudcolour>>8)&255)/255.0f, b = (cloudcolour&255)/255.0f;
    glColor4f(r, g, b, cloudalpha);
    glBegin(GL_POLYGON);
    loopi(cloudsubdiv)
    {
        vec p(1, 1, 0);
        p.rotate_around_z((-2.0f*M_PI*i)/cloudsubdiv);
        glTexCoord2f(tx + p.x*tsz, ty + p.y*tsz); glVertex3f(p.x*psz, p.y*psz, z);
    }
    glEnd();
    float tsz2 = 0.5f/cloudscale;
    glBegin(GL_QUAD_STRIP);
    loopi(cloudsubdiv+1)
    {
        vec p(1, 1, 0);
        p.rotate_around_z((-2.0f*M_PI*i)/cloudsubdiv);
        glColor4f(r, g, b, cloudalpha);
        glTexCoord2f(tx + p.x*tsz, ty + p.y*tsz); glVertex3f(p.x*psz, p.y*psz, z);
        glColor4f(r, g, b, 0);
        glTexCoord2f(tx + p.x*tsz2, ty + p.y*tsz2); glVertex3f(p.x*w, p.y*w, z);
    }
    glEnd();    
}

VARP(sparklyfix, 0, 0, 1);
VAR(showsky, 0, 1, 1); 
VAR(clipsky, 0, 1, 1);

bool drawskylimits(bool explicitonly)
{
    nocolorshader->set();

    glDisable(GL_TEXTURE_2D);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    bool rendered = rendersky(explicitonly);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glEnable(GL_TEXTURE_2D);

    return rendered;
}

void drawskyoutline()
{
    notextureshader->set();

    glDisable(GL_TEXTURE_2D);
    glDepthMask(GL_FALSE);
    extern int wireframe;
    if(!wireframe)
    {
        enablepolygonoffset(GL_POLYGON_OFFSET_LINE);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }
    glColor3f(0.5f, 0.0f, 0.5f);
    rendersky(true);
    if(!wireframe) 
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        disablepolygonoffset(GL_POLYGON_OFFSET_LINE);
    }
    glDepthMask(GL_TRUE);
    glEnable(GL_TEXTURE_2D);

    if(!glaring) defaultshader->set();
}

VAR(clampsky, 0, 1, 1);

static int yawskyfaces(int faces, int yaw, float spin = 0)
{
    if(spin || yaw%90) return faces&0x0F ? faces | 0x0F : faces;
    static const int faceidxs[3][4] =
    {
        { 3, 2, 0, 1 },
        { 1, 0, 3, 2 },
        { 2, 3, 1, 0 }
    };
    yaw /= 90;
    if(yaw < 1 || yaw > 3) return faces;
    const int *idxs = faceidxs[yaw - 1];
    return (faces & ~0x0F) | (((faces>>idxs[0])&1)<<0) | (((faces>>idxs[1])&1)<<1) | (((faces>>idxs[2])&1)<<2) | (((faces>>idxs[3])&1)<<3);
}

void drawskybox(int farplane, bool limited)
{
    extern int renderedskyfaces, renderedskyclip; // , renderedsky, renderedexplicitsky;
    bool alwaysrender = editmode || !insideworld(camera1->o) || reflecting,
         explicitonly = false;
    if(limited)
    {
        explicitonly = alwaysrender || !sparklyfix || refracting; 
        if(!drawskylimits(explicitonly) && !alwaysrender) return;
        extern int ati_skybox_bug;
        if(!alwaysrender && !renderedskyfaces && !ati_skybox_bug) explicitonly = false;
    }
    else if(!alwaysrender)
    {
        extern vtxarray *visibleva;
        renderedskyfaces = 0;
        renderedskyclip = INT_MAX;
        for(vtxarray *va = visibleva; va; va = va->next)
        {
            if(va->occluded >= OCCLUDE_BB && va->skyfaces&0x80) continue;
            renderedskyfaces |= va->skyfaces&0x3F;
            if(!(va->skyfaces&0x1F) || camera1->o.z < va->skyclip) renderedskyclip = min(renderedskyclip, va->skyclip);
            else renderedskyclip = 0;
        }
        if(!renderedskyfaces) return;
    }
    
    if(alwaysrender)
    {
        renderedskyfaces = 0x3F;
        renderedskyclip = 0;
    }

    float skyclip = clipsky ? max(renderedskyclip-1, 0) : 0;
    if(reflectz<worldsize && reflectz>skyclip) skyclip = reflectz;

    if(glaring)
    {
        static Shader *skyboxglareshader = NULL;
        if(!skyboxglareshader) skyboxglareshader = lookupshaderbyname("skyboxglare");
        skyboxglareshader->set();
    }
    else defaultshader->set();

    if(renderpath!=R_FIXEDFUNCTION || !fogging) glDisable(GL_FOG);

    if(limited) 
    {
        if(explicitonly) glDisable(GL_DEPTH_TEST);
        else glDepthFunc(GL_GEQUAL);
    }
    else glDepthFunc(GL_LEQUAL);

    glDepthMask(GL_FALSE);

    if(clampsky) glDepthRange(1, 1);

    glColor3f(1, 1, 1);

    glPushMatrix();
    glLoadIdentity();
    glRotatef(camera1->roll, 0, 0, 1);
    glRotatef(camera1->pitch, -1, 0, 0);
    glRotatef(camera1->yaw+spinsky*lastmillis/1000.0f+yawsky, 0, 1, 0);
    glRotatef(90, 1, 0, 0);
    if(reflecting) glScalef(1, 1, -1);
    draw_envbox(farplane/2, skyclip ? 0.5f + 0.5f*(skyclip-camera1->o.z)/float(worldsize) : 0, yawskyfaces(renderedskyfaces, yawsky, spinsky), sky);
    glPopMatrix();

    if(!glaring && cloudbox[0])
    {
        if(fading) glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glPushMatrix();
        glLoadIdentity();
        glRotatef(camera1->roll, 0, 0, 1);
        glRotatef(camera1->pitch, -1, 0, 0);
        glRotatef(camera1->yaw+spinclouds*lastmillis/1000.0f+yawclouds, 0, 1, 0);
        glRotatef(90, 1, 0, 0);
        if(reflecting) glScalef(1, 1, -1);
        draw_envbox(farplane/2, skyclip ? 0.5f + 0.5f*(skyclip-camera1->o.z)/float(worldsize) : cloudclip, yawskyfaces(renderedskyfaces, yawclouds, spinclouds), clouds);
        glPopMatrix();

        glDisable(GL_BLEND);
    }

    if(!glaring && cloudlayer[0] && cloudheight && renderedskyfaces&(cloudheight < 0 ? 0x1F : 0x2F))
    {
        if(fading) glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);

        glDisable(GL_CULL_FACE);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glPushMatrix();
        glLoadIdentity();
        glRotatef(camera1->roll, 0, 0, 1);
        glRotatef(camera1->pitch, -1, 0, 0);
        glRotatef(camera1->yaw+spincloudlayer*lastmillis/1000.0f+yawcloudlayer, 0, 1, 0);
        glRotatef(90, 1, 0, 0);
        if(reflecting) glScalef(1, 1, -1);
        draw_env_overlay(farplane/2, cloudoverlay, cloudscrollx * lastmillis/1000.0f, cloudscrolly * lastmillis/1000.0f);
        glPopMatrix();

        glDisable(GL_BLEND);

        glEnable(GL_CULL_FACE);
    }

    if(clampsky) glDepthRange(0, 1);

    glDepthMask(GL_TRUE);

    if(limited)
    {
        if(explicitonly) glEnable(GL_DEPTH_TEST);
        else glDepthFunc(GL_LESS);
        if(!reflecting && !refracting && !envmapping && editmode && showsky) drawskyoutline();
    }
    else glDepthFunc(GL_LESS);

    if(renderpath!=R_FIXEDFUNCTION || !fogging) glEnable(GL_FOG);
}

VARNR(skytexture, useskytexture, 0, 1, 1);

int explicitsky = 0;
double skyarea = 0;

bool limitsky()
{
    return (explicitsky && (useskytexture || editmode)) || (sparklyfix && skyarea / (double(worldsize)*double(worldsize)*6) < 0.9);
}

