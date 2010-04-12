
// Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
// This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

#include "cube.h"
#include "engine.h"
#include "game.h"

#include "network_system.h"
#include "intensity.h"
#include "script_engine_manager.h"

#ifdef CLIENT
    #include "client_system.h"
    #include "character_render.h"
    #ifdef INTENSITY_PLUGIN
        #include "intensity_plugin_listener.h"
    #endif
#else // SERVER
    #include "server_system.h"
#endif

#include "utility.h"
#include "system_manager.h"
#include "message_system.h"
#include "world_system.h"
#include "intensity_physics.h"


// Enable to let *server* do physics for players - useful for debugging. Must also be defined in client.cpp!
#define SERVER_DRIVEN_PLAYERS 0


namespace game
{
    int gamemode = 0;
    string clientmap = "";
    int maptime = 0, maprealtime = 0;

    int following = -1, followdir = 0;

    fpsent *player1 = NULL;         // our client
    vector<fpsent *> players;       // other clients
    fpsent lastplayerstate;

	void follow(char *arg)
    {
        if(arg[0] ? player1->state==CS_SPECTATOR : following>=0)
        {
            following = arg[0] ? parseplayer(arg) : -1;
            if(following==player1->clientnum) following = -1;
            followdir = 0;
            conoutf("follow %s", following>=0 ? "on" : "off");
        }
	}

    void nextfollow(int dir)
    {
        if(player1->state!=CS_SPECTATOR || players.empty())
        {
            stopfollowing();
            return;
        }
        int cur = following >= 0 ? following : (dir < 0 ? players.length() - 1 : 0);
        loopv(players) 
        {
            cur = (cur + dir + players.length()) % players.length();
            if(players[cur])
            {
                if(following<0) conoutf("follow on");
                following = cur;
                followdir = dir;
                return;
            }
        }
        stopfollowing();
    }

    const char *getclientmap()
    {
        REFLECT_PYTHON( get_curr_map_prefix );

        static std::string ret;
        ret = boost::python::extract<std::string>(get_curr_map_prefix());
        ret += "map";
        return ret.c_str();
    }

    void resetgamestate()
    {
    }

    fpsent *spawnstate(fpsent *d)              // reset player state not persistent accross spawns
    {
        d->respawn();
        d->spawnstate(gamemode);
        return d;
    }

    void respawnself()
    {
        spawnplayer(player1);
    }

    fpsent *pointatplayer()
    {
        assert(0);
        return NULL;
#if 0
        loopv(players)
        {
            fpsent *o = players[i];
            if(!o) continue;
            if(intersect(o, player1->o, worldpos)) return o;
        }
        return NULL;
#endif
    }

    void stopfollowing()
    {
        if(following<0) return;
        following = -1;
        followdir = 0;
        conoutf("follow off");
    }

    fpsent *followingplayer()
    {
        if(player1->state!=CS_SPECTATOR || following<0) return NULL;
        fpsent *target = getclient(following);
        if(target && target->state!=CS_SPECTATOR) return target;
        return NULL;
    }

    fpsent *hudplayer()
    {
        if(thirdperson) return player1;
        fpsent *target = followingplayer();
        return target ? target : player1;
    }

    void setupcamera()
    {
        fpsent *target = followingplayer();
        if(target) 
        {
            player1->yaw = target->yaw;    // Kripken: needed?
            player1->pitch = target->state==CS_DEAD ? 0 : target->pitch; // Kripken: needed?
            player1->o = target->o;
            player1->resetinterp();
        }
    }

    bool detachcamera()
    {
        fpsent *d = hudplayer();
        return d->state==CS_DEAD;
    }

    bool collidecamera()
    {
        switch(player1->state)
        {
            case CS_EDITING: return false;
            case CS_SPECTATOR: return followingplayer()!=NULL;
        }
        return true;
    }

    VARP(smoothmove, 0, 75, 100);
    VARP(smoothdist, 0, 32, 64);

    void predictplayer(fpsent *d, bool move)
    {
        d->o = d->newpos;
        d->yaw = d->newyaw;
        d->pitch = d->newpitch;
        if(move)
        {
            moveplayer(d, 1, false);
            d->newpos = d->o;
        }
        float k = 1.0f - float(lastmillis - d->smoothmillis)/smoothmove;
        if(k>0)
        {
            d->o.add(vec(d->deltapos).mul(k));
            d->yaw += d->deltayaw*k;
            if(d->yaw<0) d->yaw += 360;
            else if(d->yaw>=360) d->yaw -= 360;
            d->pitch += d->deltapitch*k;
        }
    }

    void otherplayers(int curtime)
    {
        loopv(players) if(players[i] && LogicSystem::getUniqueId(players[i]) >= 0) // Need a complete entity for this
        {
            fpsent *d = players[i];
            if(d == player1 || d->ai) continue;

            if (d->uniqueId < 0) continue;

            #ifdef SERVER
                if (d->serverControlled)
                    continue; // On the server, 'other players' are only PCs
            #endif

            Logging::log(Logging::INFO, "otherplayers: moving %d from %f,%f,%f\r\n", d->uniqueId, d->o.x, d->o.y, d->o.z);

            if(d->state==CS_ALIVE)
            {
                if(lastmillis - d->lastaction >= d->gunwait) d->gunwait = 0; 
//                if(d->quadmillis) et.checkquad(curtime, d);
            }

            // TODO: Currently serverside physics for otherplayers run like clientside physics - if
            // there is *ANY* lag, run physics. But we can probably save a lot of CPU on the server
            // if we don't run physics if the lag is 'reasonable'. Note that this is already sort of
            // done by having say lower fps on the server - if the last update was recent enough
            // then the frame decision system may decide we need 0 frames at the moment. But, it
            // might be better to also add an explicit condition, that we don't just check for 0
            // lagtime as below, but also for lagtime within say 1-2 frames at the server's fps rate.
#if (SERVER_DRIVEN_PLAYERS == 0)
            const int lagtime = lastmillis-d->lastupdate; // Change to '1' to have server ALWAYS run physics
#else
            const int lagtime = 1;
#endif

            if(!lagtime) continue;
            if(lagtime>1000 && d->state==CS_ALIVE)
            {
                d->state = CS_LAGGED;
                continue;
            }

            // Ignore intentions to move, if immobile
            if ( !LogicSystem::getLogicEntity(d)->getCanMove() )
                d->turn_move = d->move = d->look_updown_move = d->strafe = d->jumping = 0;

            if(d->state==CS_ALIVE || d->state==CS_EDITING)
            {
#if (SERVER_DRIVEN_PLAYERS == 0)
                if(smoothmove && d->smoothmillis>0) predictplayer(d, true); // Disable to force server to always move clients
                else moveplayer(d, 1, false);
#else
                moveplayer(d, 1, false);
#endif
            }
            else if(d->state==CS_DEAD && lastmillis-d->lastpain<2000) moveplayer(d, 1, true);

            Logging::log(Logging::INFO, "                                      to %f,%f,%f\r\n", d->o.x, d->o.y, d->o.z);

#if (SERVER_DRIVEN_PLAYERS == 1)
            // Enable this to let server drive client movement
            ScriptEngineManager::runScript(
                "getEntity(" + Utility::toString(d->uniqueId) + ").position = [" +
                "getEntity(" + Utility::toString(d->uniqueId) + ").position.x," +
                "getEntity(" + Utility::toString(d->uniqueId) + ").position.y," +
                "getEntity(" + Utility::toString(d->uniqueId) + ").position.z]"
            );
#endif
        }
    }

    void moveControlledEntities()
    {
#ifdef CLIENT
        if ( ClientSystem::playerLogicEntity.get() )
        {
            ClientSystem::playerLogicEntity.get()->scriptEntity->debugPrint();

            if ( ClientSystem::playerLogicEntity.get()->scriptEntity->getPropertyBool("initialized") )
            {
                Logging::log(Logging::INFO, "Player %d (%lu) is initialized, run moveplayer(): %f,%f,%f.\r\n",
                    player1->uniqueId, (unsigned long)player1,
                    player1->o.x,
                    player1->o.y,
                    player1->o.z
                );

                // Ignore intentions to move, if immobile
                if ( !ClientSystem::playerLogicEntity->getCanMove() )
                {
                    player1->turn_move = player1->move = player1->look_updown_move = player1->strafe = player1->jumping = 0;
                }

//                if(player1->ragdoll && !(player1->anim&ANIM_RAGDOLL)) cleanragdoll(player1); XXX Needed? See below
#if (SERVER_DRIVEN_PLAYERS == 0)
                moveplayer(player1, 10, true); // Disable this to stop play from moving by client command
#endif

                Logging::log(Logging::INFO, "                              moveplayer(): %f,%f,%f.\r\n",
                    player1->o.x,
                    player1->o.y,
                    player1->o.z
                );

                swayhudgun(curtime);
                entities::checkitems(player1);
            } else
                Logging::log(Logging::INFO, "Player is not yet initialized, do not run moveplayer() etc.\r\n");
        }
        else
            Logging::log(Logging::INFO, "Player does not yet exist, or scenario not started, do not run moveplayer() etc.\r\n");
        
#else // SERVER
    #if 1
        // Loop over NPCs we control, moving and sending their info c2sinfo for each.
        loopv(players)
        {
            fpsent* npc = players[i];
            if (!npc->serverControlled || npc->uniqueId == DUMMY_SINGLETON_CLIENT_UNIQUE_ID)
                continue;

            // We do this so scripting need not worry in the NPC behaviour code
            while(npc->yaw < -180.0f) npc->yaw += 360.0f;
            while(npc->yaw > +180.0f) npc->yaw -= 360.0f;

            while(npc->pitch < -180.0f) npc->pitch += 360.0f;
            while(npc->pitch > +180.0f) npc->pitch -= 360.0f;

            // Apply physics to actually move the player
            moveplayer(npc, 10, false); // FIXME: Use Config param for resolution and local. 1, false does seem ok though

            Logging::log(Logging::INFO, "updateworld, server-controlled client %d: moved to %f,%f,%f\r\n", i,
                                            npc->o.x, npc->o.y, npc->o.z);

            //?? Dummy singleton still needs to send the messages vector. XXX - do we need this even without NPCs? XXX - works without it

//            c2sinfo(npc, Utility::Config::getInt("Network", "rate", 33)); // FIXME: Variable rate, different than player,
//                                                                          // perhaps depending on distance etc. etc.
        }
    #endif
#endif
    }

    void updateworld()        // main game update loop
    {
        Logging::log(Logging::INFO, "updateworld(?, %d)\r\n", curtime);
        INDENT_LOG(Logging::INFO);

        // SERVER used to initialize turn_move, move, look_updown_move and strafe to 0 for NPCs here

        if(!maptime) { maptime = lastmillis; maprealtime = totalmillis; return; }
        if(!curtime)
        {
#ifdef CLIENT
            gets2c();
            if(player1->clientnum>=0) c2sinfo();
#endif
            return;
        }

#ifdef CLIENT
        bool runWorld = ClientSystem::scenarioStarted();
#else
        bool runWorld = ScriptEngineManager::hasEngine();
#endif
        static Benchmarker physicsBenchmarker;

        //===================
        // Run physics
        //===================

        physicsBenchmarker.start();

            if (runWorld)
            {
                PhysicsManager::simulate(float(curtime)/1024.0f);

                //============================================
                // Additional physics: Collisions
                //============================================

                // If triggering collisions can be done by the scripting library code, use that
                if (ScriptEngineManager::getGlobal()->hasProperty("manageTriggeringCollisions"))
                    ScriptEngineManager::getGlobal()->call("manageTriggeringCollisions");
                else
                {
                    loopv(players)
                    {
                        fpsent* fpsEntity = players[i];
                        LogicEntityPtr entity = LogicSystem::getLogicEntity(fpsEntity);
                        if (!entity.get() || entity->isNone()) continue;

                        if(fpsEntity->state != CS_EDITING)
                        {
                            WorldSystem::checkTriggeringCollisions(entity);
                        }
                    }
                }
            }

        physicsBenchmarker.stop();

        //==============================================
        // Manage actions
        // Done after physics, so can override physics
        //==============================================

        static Benchmarker actionsBenchmarker;
        actionsBenchmarker.start();
            if (runWorld)
            {
                ScriptEngineManager::getGlobal()->call("startFrame");

                LogicSystem::manageActions(curtime);
            }
        actionsBenchmarker.stop();

#ifdef CLIENT
        //================================================================
        // Get messages - *AFTER* otherplayers, which applies smoothness,
        // and after actions, since gets2c may destroy the engine
        //================================================================

        gets2c();
#endif

        //============================================
        // Send network updates, last for least lag
        //============================================

#ifdef CLIENT
        // clientnum might be -1, if we have yet to get S2C telling us our clientnum, i.e., we are only partially connected
        if(player1->clientnum>=0) c2sinfo(); //player1, // do this last, to reduce the effective frame lag
#else // SERVER
        c2sinfo(); // Send all the info for all the NPCs
#endif

        SystemManager::showBenchmark("Physics", physicsBenchmarker);
        SystemManager::showBenchmark("                    Actions", actionsBenchmarker);
    }

    void spawnplayer(fpsent *d)   // place at random spawn. also used by monsters!
    {
//        findplayerspawn(d, respawnent>=0 ? respawnent : -1, 0); Kripken: We now do this manually
        spawnstate(d);
        #ifdef CLIENT
            d->state = spectator ? CS_SPECTATOR : (d==player1 && editmode ? CS_EDITING : CS_ALIVE);
        #else // SERVER
            d->state = CS_ALIVE;
        #endif
    }

    void respawn()
    {
        assert(0);
//           respawnself();
    }

    // inputs

    void doattack(bool on)
    {
    }

    bool canjump() 
    {
        return true; // Handled ourselves elsewhere
    }

    bool allowmove(physent *d)
    {
        return true; // Handled ourselves elsewhere
    }

    vector<fpsent *> clients;

    fpsent *newclient(int cn)   // ensure valid entity
    {
        Logging::log(Logging::DEBUG, "fps::newclient: %d\r\n", cn);

        if(cn < 0 || cn > max(0xFF, MAXCLIENTS)) // + MAXBOTS))
        {
            neterr("clientnum", false);
            return NULL;
        }

#ifdef CLIENT // INTENSITY
        if(cn == player1->clientnum)
        {
            player1->uniqueId = -5412; // Wipe uniqueId of new client
            return player1;
        }
#endif

        while(cn >= clients.length()) clients.add(NULL);

        fpsent *d = new fpsent;
        d->clientnum = cn;
        assert(clients[cn] == NULL); // XXX FIXME This fails if a player logged in exactly while the server was downloading assets
        clients[cn] = d;
        players.add(d);

        return clients[cn];
    }

    fpsent *getclient(int cn)   // ensure valid entity
    {
#ifdef CLIENT // INTENSITY
        if(cn == player1->clientnum) return player1;
#endif
        return clients.inrange(cn) ? clients[cn] : NULL;
    }

    void clientdisconnected(int cn, bool notify)
    {
        Logging::log(Logging::DEBUG, "fps::clientdisconnected: %d\r\n", cn);

        if(!clients.inrange(cn)) return;
        if(following==cn)
        {
            if(followdir) nextfollow(followdir);
            else stopfollowing();
        }
        fpsent *d = clients[cn];
        if(!d) return;
        if(notify && d->name[0]) conoutf("player %s disconnected", colorname(d));
//        removeweapons(d);
//        removetrackedparticles(d);
//        removetrackeddynlights(d);
//        if(cmode) cmode->removeplayer(d);

        players.removeobj(d);
        DELETEP(clients[cn]);
        cleardynentcache();
    }

    void initclient()
    {
        clientmap[0] = 0;
        player1 = spawnstate(new fpsent);
#ifdef CLIENT
        players.add(player1);
#endif
    }

/*
        const char *mdls[] =
        {
            "gibc", "gibh",
            "projectiles/grenade", "projectiles/rocket",
            "debris/debris01", "debris/debris02", "debris/debris03", "debris/debris04",
            "barreldebris/debris01", "barreldebris/debris02", "barreldebris/debris03", "barreldebris/debris04"
        };
        loopi(sizeof(mdls)/sizeof(mdls[0]))
        {
            loadmodel(mdls[i], -1, true);
        }
*/
    void preload() { }; // We use our own preloading system, but need to add the above projectiles etc.

    IVARP(startmenu, 0, 1, 1);

    void startmap(const char *name)   // called just after a map load
    {
//        if(multiplayer(false) && m_sp) { gamemode = 0; conoutf(CON_ERROR, "coop sp not supported yet"); } Kripken
//        clearmovables();
//        clearprojectiles();
//        clearbouncers();

#ifdef CLIENT
        spawnplayer(player1);
#endif

        entities::resetspawns();
        copystring(clientmap, name ? name : "");
        setvar("zoom", -1, true);
        maptime = 0;
//        if(*name) conoutf(CON_GAMEINFO, "\f2game mode is %s", fpsserver::modestr(gamemode));

        if(identexists("mapstart")) execute("mapstart");

#ifdef SERVER
        server::resetScenario();
#endif
    }

    void playsoundc(int n, fpsent *d = NULL)
    { 
#ifdef CLIENT
        if(!d || d==player1)
        {
            MessageSystem::send_SoundToServer(n);
//            addmsg(SV_SOUND, "i", n); 

            playsound(n); 
        }
        else playsound(n, &d->o);
#endif
    }

    void physicstrigger(physent *d, bool local, int floorlevel, int waterlevel, int material)
    {
#ifdef CLIENT
        if(d->type==ENT_INANIMATE) return;
        if     (waterlevel>0) { if(material!=MAT_LAVA) playsound(S_SPLASH1, d==player1 ? NULL : &d->o); }
        else if(waterlevel<0) playsound(material==MAT_LAVA ? S_BURN : S_SPLASH2, d==player1 ? NULL : &d->o);
        if     (floorlevel>0) { if(d==player1 || d->type!=ENT_PLAYER) playsoundc(S_JUMP, (fpsent *)d); }
        else if(floorlevel<0) { if(d==player1 || d->type!=ENT_PLAYER) playsoundc(S_LAND, (fpsent *)d); }
#endif
    }

    int numdynents()
    {
        return players.length();
    } //+movables.length(); }

    dynent *iterdynents(int i)
    {
        if(i<players.length()) return players[i];
//        i -= players.length();
//        if(i<movables.length()) return (dynent *)movables[i];
        return NULL;
    }

    std::string scriptname(fpsent *d)
    {
        return ScriptEngineManager::getGlobal()->call(
            "getEntity",
            LogicSystem::getUniqueId(d)
        )->getProperty("_name")->getString();
    }

    char *colorname(fpsent *d, char *name, const char *prefix)
    {
        std::string sName;
        if(!name)
        {
            sName = scriptname(d);
            name = (char*)sName.c_str();
        }
        const char* color = (d != player1) ? "" : "\f1";
        static string cname;
        formatstring(cname)("%s%s", color, name);
        return cname;
    }

    void suicide(physent *d)
    {
        assert(0);
    }

    IVARP(hudgun, 0, 1, 1);
    IVARP(hudgunsway, 0, 1, 1);
    IVARP(teamhudguns, 0, 1, 1);
   
    void drawhudmodel(fpsent *d, int anim, float speed = 0, int base = 0)
    {
        Logging::log(Logging::WARNING, "Rendering hudmodel is deprecated for now\r\n");
    }

    void drawhudgun()
    {
        Logging::log(Logging::WARNING, "Rendering hudgun is deprecated for now\r\n");
    }

    void drawicon(float tx, float ty, int x, int y)
    {
    }

#ifdef CLIENT
    float abovegameplayhud()
    {
        return 1650.0f/1800.0f;
    }

    void gameplayhud(int w, int h)
    {
        // Draw the HUD for the game
        ClientSystem::drawHUD(w, h);
    }
#endif

    void lighteffects(dynent *e, vec &color, vec &dir)
    {
    }

    void particletrack(physent *owner, vec &o, vec &d)
    {
#ifdef CLIENT
        if(owner->type!=ENT_PLAYER && owner->type!=ENT_AI) return;
//        fpsent *pl = (fpsent *)owner;
//        if(pl->muzzle.x < 0 || pl->lastattackgun != pl->gunselect) return;
        float dist = o.dist(d);
        o = vec(0,0,0); //pl->muzzle;
        if(dist <= 0) d = o;
        else
        { 
            vecfromyawpitch(owner->yaw, owner->pitch, 1, 0, d);
            float newdist = raycube(owner->o, d, dist, RAY_CLIPMAT|RAY_ALPHAPOLY);
            d.mul(min(newdist, dist)).add(owner->o);
        }
#else // SERVER
        assert(0);
#endif
    }

    void newmap(int size)
    {
        // Generally not used, as we fork emptymap, but useful to clear and resize
    }
 
    // any data written into this vector will get saved with the map data. Must take care to do own versioning, and endianess if applicable. Will not get called when loading maps from other games, so provide defaults.
    void writegamedata(vector<char> &extras) {}
    void readgamedata(vector<char> &extras) {}

    const char *gameident() { return "fps"; }
    const char *defaultmap() { return "login"; }
    const char *savedconfig() { return "config.cfg"; }
    const char *defaultconfig() { return "data/defaults.cfg"; }
    const char *autoexec() { return "autoexec.cfg"; }
    const char *savedservers() { return NULL; } //"servers.cfg"; }

    // Dummies

    bool serverinfostartcolumn(g3d_gui *g, int i)
    {
        assert(0);
        return false;
    }

     bool serverinfoentry(g3d_gui *g, int i, const char *name, const char *desc, const char *map, int ping, const vector<int> &attr, int np)
    {
        assert(0);
        return false;
    }

    void serverinfoendcolumn(g3d_gui *g, int i)
    {
        assert(0);
    }

    const char *defaultcrosshair(int index)
    {
        return "data/crosshair.png";
    }

    int selectcrosshair(float &r, float &g, float &b)
    {
        r = 1.0f; g = 1.0f; b = 1.0f;
        return 0;
    }

    void parseoptions(vector<const char *> &args)
    {
        loopv(args)
        {
            const char* arg = args[i];
            printf("parseoptions: %c\r\n", arg[1]);
            #ifdef INTENSITY_PLUGIN
                if (arg[1] == 'P')
                {
                    PluginListener::initialize();
                }
            #endif
        }
    }

    bool serverinfoentry(g3d_gui *g, int i, const char *name, int port, const char *desc, const char *map, int ping, const vector<int> &attr, int np) { assert(0); return false; }

    const char *getmapinfo()
    {
        return "";
    }

    int clipconsole(int w, int h)
    {
        return 0;
    }

    void loadconfigs()
    {
    }

    bool ispaused() { return false; };

    void dynlighttrack(physent *owner, vec &o)
    {
        return;
    }
}

