
/*
 *=============================================================================
 * Copyright (C) 2001-2006 Wouter van Oortmerssen.
 * Copyright (C) 2008 Alon Zakai ('Kripken') kripkensteiner@gmail.com
 *
 * This file is part of the Intensity Engine project,
 *    http://www.intensityengine.com
 *
 * The Intensity Engine is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * The Intensity Engine is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with the Intensity Engine.  If not, see
 *     http://www.gnu.org/licenses/
 *     http://www.gnu.org/licenses/agpl-3.0.html
 *=============================================================================
 */


// movable.h: implements physics for inanimate models

struct movableset
{
    fpsclient &cl;

    enum
    {
        BOXWEIGHT = 25,
        BARRELHEALTH = 50,
        BARRELWEIGHT = 25,
        PLATFORMWEIGHT = 1000,
        PLATFORMSPEED = 8,
        EXPLODEDELAY = 200
    };

    struct movable : dynent
    {
        int etype, mapmodel, health, weight, explode, tag, dir;
        movableset *ms;

        movable(const entity &e, movableset *ms) : 
            etype(e.type),
            mapmodel(e.attr2),
            health(e.type==BARREL ? (e.attr4 ? e.attr4 : BARRELHEALTH) : 0), 
            weight(e.type==PLATFORM || e.type==ELEVATOR ? PLATFORMWEIGHT : (e.type==e.attr3 ? e.attr3 : (e.type==BARREL ? BARRELWEIGHT : BOXWEIGHT))), 
            explode(0),
            tag(e.type==PLATFORM || e.type==ELEVATOR ? e.attr3 : 0),
            dir(e.type==PLATFORM || e.type==ELEVATOR ? (e.attr4 < 0 ? -1 : 1) : 0),
            ms(ms)
        {
            state = CS_ALIVE;
            type = ENT_INANIMATE;
            yaw = float((e.attr1+7)-(e.attr1+7)%15);
            if(e.type==PLATFORM || e.type==ELEVATOR) 
            {
                maxspeed = e.attr4 ? fabs(float(e.attr4)) : PLATFORMSPEED;
                if(tag) vel = vec(0, 0, 0);
                else if(e.type==PLATFORM) { vecfromyawpitch(yaw, 0, 1, 0, vel); vel.mul(dir*maxspeed); } 
                else vel = vec(0, 0, dir*maxspeed);
            }

            conoutf("Movables are deprecated, see next 2 lines\r\n");
/*            const char *mdlname = mapmodelname(e.attr2);
              if(mdlname) setbbfrommodel(this, mdlname); */
        }
       
        void hitpush(int damage, const vec &dir, fpsent *actor, int gun)
        { // Kripken: Here we see how boxes and barrels are moved when you damage them. Nice, depends on damage/weight
            if(etype!=BOX && etype!=BARREL) return;
            vec push(dir);
            push.mul(80*damage/weight);
            vel.add(push);
            moving = true;
        }
 
        void damaged(int damage, fpsent *at, int gun = -1)
        {
            if(etype!=BARREL || state!=CS_ALIVE || explode) return;
            health -= damage;
            if(health>0) return;
            if(gun==GUN_BARREL) explode = ms->cl.lastmillis + EXPLODEDELAY;
            else 
            {
                state = CS_DEAD;
#ifdef CLIENT
                ms->cl.ws.explode(true, at, o, this, guns[GUN_BARREL].damage, GUN_BARREL);
#endif
            }
        }

        void suicide()
        {
            state = CS_DEAD;
#ifdef CLIENT
            if(etype==BARREL) ms->cl.ws.explode(true, ms->cl.player1, o, this, guns[GUN_BARREL].damage, GUN_BARREL);
#endif
        }
    };

    movableset(fpsclient &cl) : cl(cl)
    {
        CCOMMAND(platform, "ii", (movableset *self, int *tag, int *newdir), self->triggerplatform(*tag, *newdir));
    }
    
    vector<movable *> movables;
   
    void clear(int gamemode)
    {
        if(movables.length())
        {
            cleardynentcache();
            movables.deletecontentsp();
        }
        if(!m_dmsp && !m_classicsp) return;
        loopv(cl.et.ents) // Kripken: Looks like we sift the movables out of the list of entities
        {
            const entity &e = *cl.et.ents[i];
            if(e.type!=BOX && e.type!=BARREL && e.type!=PLATFORM && e.type!=ELEVATOR) continue;
            movable *m = new movable(e, this);
            movables.add(m);
            m->o = e.o;
            entinmap(m);
            updatedynentcache(m);
        }
    }

    void triggerplatform(int tag, int newdir)
    {
        newdir = max(-1, min(1, newdir));
        loopv(movables)
        {
            movable *m = movables[i];
            if(m->state!=CS_ALIVE || (m->etype!=PLATFORM && m->etype!=ELEVATOR) || m->tag!=tag) continue;
            if(!newdir)
            {
                if(m->tag) m->vel = vec(0, 0, 0);
                else m->vel.neg();
            }
            else
            {
                if(m->etype==PLATFORM) { vecfromyawpitch(m->yaw, 0, 1, 0, m->vel); m->vel.mul(newdir*m->dir*m->maxspeed); }
                else m->vel = vec(0, 0, newdir*m->dir*m->maxspeed);
            }
        }
    }

    void update(int curtime)
    {
        if(!curtime) return;
        loopv(movables)
        {
            movable *m = movables[i];
            if(m->state!=CS_ALIVE) continue;
            if(m->etype==PLATFORM || m->etype==ELEVATOR)
            {
                if(m->vel.iszero()) continue;
                for(int remaining = curtime; remaining>0;)
                {
                    int step = min(remaining, 20);
                    remaining -= step;
                    if(!moveplatform(m, vec(m->vel).mul(step/1000.0f)))
                    {
                        if(m->tag) { m->vel = vec(0, 0, 0); break; }
                        else m->vel.neg();
                    }
                }
            }
            else if(m->explode && cl.lastmillis >= m->explode)
            {
                m->state = CS_DEAD;
                m->explode = 0;
#ifdef CLIENT
                cl.ws.explode(true, (fpsent *)m, m->o, m, guns[GUN_BARREL].damage, GUN_BARREL);
                adddecal(DECAL_SCORCH, m->o, vec(0, 0, 1), RL_DAMRAD/2);
#endif
            }
            else if(m->moving || (m->onplayer && (m->onplayer->state!=CS_ALIVE || m->lastmoveattempt <= m->onplayer->lastmove))) moveplayer(m, 1, true);
        }
    }

    void render()
    {
//        conoutf("Rendering movables in the old way is deprecated\r\n");

/*
        loopv(movables)
        {
            movable &m = *movables[i];
            if(m.state!=CS_ALIVE) continue;
            vec o(m.o);
            o.z -= m.eyeheight;
            const char *mdlname = mapmodelname(m.mapmodel);
            if(!mdlname) continue;
			rendermodel(NULL, mdlname, ANIM_MAPMODEL|ANIM_LOOP, o, m.yaw, 0,                 MDL_LIGHT | MDL_SHADOW | MDL_CULL_VFC | MDL_CULL_DIST | MDL_CULL_OCCLUDED, &m);
        }
*/
    }
};
