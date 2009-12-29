
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

// Windows
#undef PLATFORM

// console message types

enum
{
    CON_CHAT       = 1<<8,
    CON_TEAMCHAT   = 1<<9,
    CON_GAMEINFO   = 1<<10,
    CON_FRAG_SELF  = 1<<11,
    CON_FRAG_OTHER = 1<<12
};

// network quantization scale
#define DMF 16.0f                // for world locations
#define DNF 100.0f              // for normalized vectors
#define DVELF 1.0f              // for playerspeed based velocity vectors

enum                            // static entity types
{
    NOTUSED = ET_EMPTY,         // entity slot not in use in map
    LIGHT = ET_LIGHT,           // lightsource, attr1 = radius, attr2 = intensity
    MAPMODEL = ET_MAPMODEL,     // attr1 = angle, attr2 = idx
    PLAYERSTART,                // attr1 = angle, attr2 = team
    ENVMAP = ET_ENVMAP,         // attr1 = radius
    PARTICLES = ET_PARTICLES,
    MAPSOUND = ET_SOUND,
    SPOTLIGHT = ET_SPOTLIGHT,
    I_SHELLS, I_BULLETS, I_ROCKETS, I_ROUNDS, I_GRENADES, I_CARTRIDGES,
    I_HEALTH, I_BOOST,
    I_GREENARMOUR, I_YELLOWARMOUR,
    I_QUAD,
    TELEPORT,                   // attr1 = idx
    TELEDEST,                   // attr1 = angle, attr2 = idx
    MONSTER,                    // attr1 = angle, attr2 = monstertype
    CARROT,                     // attr1 = tag, attr2 = type
    JUMPPAD,                    // attr1 = zpush, attr2 = ypush, attr3 = xpush
    BASE,
    RESPAWNPOINT,
    BOX,                        // attr1 = angle, attr2 = idx, attr3 = weight
    BARREL,                     // attr1 = angle, attr2 = idx, attr3 = weight, attr4 = health
    PLATFORM,                   // attr1 = angle, attr2 = idx, attr3 = tag, attr4 = speed
    ELEVATOR,                   // attr1 = angle, attr2 = idx, attr3 = tag, attr4 = speed
    FLAG,                       // attr1 = angle, attr2 = team
    MAXENTTYPES
};

struct fpsentity : extentity
{
    // extend with additional fields if needed...
};

//                                           ==sniper 
enum { GUN_FIST = 0, GUN_SG, GUN_CG, GUN_RL, GUN_RIFLE, GUN_GL, GUN_PISTOL, GUN_FIREBALL, GUN_ICEBALL, GUN_SLIMEBALL, GUN_BITE, GUN_BARREL, NUMGUNS };
enum { A_BLUE, A_GREEN, A_YELLOW };     // armour types... take 20/40/60 % off
enum { M_NONE = 0, M_SEARCH, M_HOME, M_ATTACKING, M_PAIN, M_SLEEP, M_AIMING };  // monster states


// hardcoded sounds, defined in sounds.cfg
enum
{
    S_JUMP = 0, S_LAND, 
    S_SPLASH1, S_SPLASH2,
    S_BURN,
    S_MENUCLICK
};

// network messages codes, c2s, c2c, s2c

enum { PRIV_NONE = 0, PRIV_MASTER, PRIV_ADMIN };

enum
{
    SV_CONNECT = 0, SV_SERVINFO, SV_WELCOME, SV_INITCLIENT, SV_POS, SV_TEXT, SV_SOUND, SV_CDIS,
    SV_SHOOT, SV_EXPLODE, SV_SUICIDE,
    SV_DIED, SV_DAMAGE, SV_HITPUSH, SV_SHOTFX,
    SV_TRYSPAWN, SV_SPAWNSTATE, SV_SPAWN, SV_FORCEDEATH,
    SV_GUNSELECT, SV_TAUNT,
    SV_MAPCHANGE, SV_MAPVOTE, SV_ITEMSPAWN, SV_ITEMPICKUP, SV_ITEMACC,
    SV_PING, SV_PONG, SV_CLIENTPING,
    SV_TIMEUP, SV_MAPRELOAD, SV_FORCEINTERMISSION,
    SV_SERVMSG, SV_ITEMLIST, SV_RESUME,
    SV_EDITMODE, SV_EDITENT, SV_EDITF, SV_EDITT, SV_EDITM, SV_FLIP, SV_COPY, SV_PASTE, SV_ROTATE, SV_REPLACE, SV_DELCUBE, SV_REMIP, SV_NEWMAP, SV_GETMAP, SV_SENDMAP, SV_EDITVAR,
    SV_MASTERMODE, SV_KICK, SV_CLEARBANS, SV_CURRENTMASTER, SV_SPECTATOR, SV_SETMASTER, SV_SETTEAM,
    SV_BASES, SV_BASEINFO, SV_BASESCORE, SV_REPAMMO, SV_BASEREGEN, SV_ANNOUNCE,
    SV_LISTDEMOS, SV_SENDDEMOLIST, SV_GETDEMO, SV_SENDDEMO,
    SV_DEMOPLAYBACK, SV_RECORDDEMO, SV_STOPDEMO, SV_CLEARDEMOS,
    SV_TAKEFLAG, SV_RETURNFLAG, SV_RESETFLAG, SV_INVISFLAG, SV_TRYDROPFLAG, SV_DROPFLAG, SV_SCOREFLAG, SV_INITFLAGS,
    SV_SAYTEAM,
    SV_CLIENT,
    SV_AUTHTRY, SV_AUTHCHAL, SV_AUTHANS, SV_REQAUTH,
    SV_PAUSEGAME,
    SV_ADDBOT, SV_DELBOT, SV_INITAI, SV_FROMAI, SV_BOTLIMIT, SV_BOTBALANCE,
    SV_MAPCRC, SV_CHECKMAPS,
    SV_SWITCHNAME, SV_SWITCHMODEL, SV_SWITCHTEAM,
    NUMSV
};

#define SAUERBRATEN_SERVER_PORT 28787
#define SAUERBRATEN_SERVINFO_PORT 28789
#define PROTOCOL_VERSION 1000           // bump when protocol changes
#define DEMO_VERSION 1                  // bump when demo format changes
#define DEMO_MAGIC "SAUERBRATEN_DEMO"

struct demoheader
{
    char magic[16]; 
    int version, protocol;
};

#define MAXNAMELEN 15
#define MAXTEAMLEN 4

#define SGRAYS 20
#define SGSPREAD 4
#define RL_DAMRAD 40
#define RL_SELFDAMDIV 2
#define RL_DISTSCALE 1.5f

// inherited by fpsent and server clients
struct fpsstate
{
    int health, maxhealth;
    int armour, armourtype;
    int quadmillis;
    int gunselect, gunwait;
    int ammo[NUMGUNS];

    int uniqueId; // INTENSITY

    fpsstate() : maxhealth(100),
                 uniqueId(-821) // INTENSITY
        {}

    void respawn()
    {
        health = maxhealth;
        armour = 0;
        armourtype = A_BLUE;
        quadmillis = 0;
        gunselect = GUN_PISTOL;
        gunwait = 0;
        loopi(NUMGUNS) ammo[i] = 0;
        ammo[GUN_FIST] = 1;
    }

    void spawnstate(int gamemode)
    {
    }

    // just subtract damage here, can set death, etc. later in code calling this 
    int dodamage(int damage)
    {
        int ad = damage*(armourtype+1)*25/100; // let armour absorb when possible
        if(ad>armour) ad = armour;
        armour -= ad;
        damage -= ad;
        health -= damage;
        return damage;        
    }
};

struct fpsent : dynent, fpsstate
{   
    int weight;                         // affects the effectiveness of hitpush
    int clientnum, privilege, lastupdate, plag, ping;
    int lifesequence;                   // sequence id for each respawn, used in damage test
    int lastpain;
    int lastaction, lastattackgun;
    bool attacking;
    int lasttaunt;
    int lastpickup, lastpickupmillis, lastbase;
    int superdamage;
    int frags, deaths, totaldamage, totalshots;
    editinfo *edit;
    float deltayaw, deltapitch, newyaw, newpitch;
    int smoothmillis;

    string name, team, info;

    void *ai; // TODO: If we want, import rest of AI code

    int lastServerUpdate; // Kripken: This is the last time we sent the server an update. Might be different per NPC.

#ifdef SERVER
    bool serverControlled; // Kripken: Set to true for NPCs that this server controls. For now, that means all NPCs
#endif

    LogicEntityPtr logicEntity;

    char turn_move, look_updown_move;    // Kripken: New movements

    int physsteps, physframetime, lastphysframe; // Kripken: Moved this here from physics.cpp: now done on a per-ent basis
    vec lastPhysicsPosition; // Kripken: The position before the last physics calculation of frame rates, etc.

    //! An integer, reserved for use in the position protocol update system. This is meant to be used by
    //! individual maps, which place their own data here, and use it however they want (for rendering, etc.).
    //! The engine itself just sends this inside the protocol updates.
    //! The reason this is needed, and why a normal StateData cannot be used, is that StateData is sent
    //! in channel 1, using reliable transmission, whereas some information must be sent along with the
    //! position info in channel 0, which is unreliable. This information will arrive faster (if there
    //! are dropped packets or network congestion), and will be synched with the position info, as it
    //! is a part of it. So, for example, this could contain animation information, that must be synched
    //! with the position very closely.
    //! This data is an unsigned integer. It is set to '0' initially and when the entity resets (so it
    //! would make sense for maps to consider that value the initialized value).
    unsigned int mapDefinedPositionData;

    fpsent() : weight(100), clientnum(-1), privilege(PRIV_NONE), lastupdate(0), plag(0), ping(0), lifesequence(0), lastpain(0), frags(0), deaths(0), totaldamage(0), totalshots(0), edit(NULL), smoothmillis(-1), ai(NULL)
                                                                      , lastServerUpdate(0)
#ifdef SERVER
                                                                      , serverControlled(false)
#endif
                                                                      , physsteps(0), physframetime(5), lastphysframe(0), lastPhysicsPosition(0,0,0)
                                                                      , mapDefinedPositionData(0)
               { name[0] = team[0] = info[0] = 0; respawn(); }
    ~fpsent()
    {
#ifdef CLIENT
        freeeditinfo(edit);
#endif
     }

    void damageroll(float damage)
    {
        float damroll = 2.0f*damage;
        roll += roll>0 ? damroll : (roll<0 ? -damroll : (rnd(2) ? damroll : -damroll)); // give player a kick
    }

    void hitpush(int damage, const vec &dir, fpsent *actor, int gun)
    {
        vec push(dir);
        push.mul(80*damage/weight);
        if(gun==GUN_RL || gun==GUN_GL) push.mul(actor==this ? 5 : (type==ENT_AI ? 3 : 2));
        vel.add(push);
    }

    void respawn()
    {
        dynent::reset();
        fpsstate::respawn();
        lastaction = 0;
        lastattackgun = gunselect;
        attacking = false;
        lasttaunt = 0;
        lastpickup = -1;
        lastpickupmillis = 0;
        lastbase = -1;
        superdamage = 0;
    }

    virtual void reset()
    {
        dynent::reset();
        turn_move = look_updown_move = 0;

        physsteps = 0;
        physframetime = 5;
        lastphysframe = lastmillis; // So we don't move too much on our first frame
        lastPhysicsPosition = vec(0,0,0);
        mapDefinedPositionData = 0;
    }

    virtual void stopmoving()
    {
        dynent::stopmoving();
        turn_move = look_updown_move = 0;
    }

    // Kripken: Normalizations missing from the engine
    void normalize_pitch(float angle)
    {
        while(pitch<angle-180.0f) pitch += 360.0f;
        while(pitch>angle+180.0f) pitch -= 360.0f;
    }

    void normalize_roll(float angle)
    {
        while(roll<angle-180.0f) roll += 360.0f;
        while(roll>angle+180.0f) roll -= 360.0f;
    }

    float getheight() // Kripken: Added this
    {
        return aboveeye + eyeheight;
    }

    vec getcenter() // Kripken: Added this
    {
        vec center(o);
        center.z -= getheight();
        return center;
    }
};




// New in 2009 - INTENSITY

namespace entities
{
    extern vector<extentity *> ents;

    extern const char *entmdlname(int type);
    extern const char *itemname(int i);

    extern void preloadentities();
    extern void renderentities();
    extern void checkitems(fpsent *d);
    extern void checkquad(int time, fpsent *d);
    extern void resetspawns();
    extern void spawnitems();
    extern void putitems(ucharbuf &p);
    extern void setspawn(int i, bool on);
    extern void teleport(int n, fpsent *d);
    extern void pickupeffects(int n, fpsent *d);

    extern void repammo(fpsent *d, int type, bool local = true);
}

namespace game
{
    struct clientmode
    {
        virtual ~clientmode() {}

        virtual void preload() {}
        virtual void drawhud(fpsent *d, int w, int h) {}
        virtual void rendergame() {}
        virtual void respawned(fpsent *d) {}
        virtual void setup() {}
        virtual void checkitems(fpsent *d) {}
        virtual int respawnwait(fpsent *d) { return 0; }
        virtual void pickspawn(fpsent *d) { findplayerspawn(d); }
        virtual void senditems(ucharbuf &p) {}
        virtual const char *prefixnextmap() { return ""; }
        virtual void removeplayer(fpsent *d) {}
        virtual void gameover() {}
        virtual bool hidefrags() { return false; }
        virtual int getteamscore(const char *team) { return 0; }
//        virtual void getteamscores(vector<teamscore> &scores) {}
    };

    extern clientmode *cmode;
    extern void setclientmode();

    // fps
    extern int gamemode, minremain;
    extern bool intermission;
    extern int maptime, maprealtime;
    extern fpsent *player1;
    extern vector<fpsent *> players;
    extern int lastspawnattempt;
    extern int lasthit;
    extern int respawnent;
    extern int following;
    extern int smoothmove, smoothdist;

    extern bool clientoption(const char *arg);
    extern fpsent *getclient(int cn);
    extern fpsent *newclient(int cn);
    extern char *colorname(fpsent *d, char *name = NULL, const char *prefix = "");
    extern fpsent *pointatplayer();
    extern fpsent *hudplayer();
    extern fpsent *followingplayer();
    extern void stopfollowing();
    extern void clientdisconnected(int cn, bool notify = true);
    extern void spawnplayer(fpsent *);
    extern void deathstate(fpsent *d, bool restore = false);
    extern void damaged(int damage, fpsent *d, fpsent *actor, bool local = true);
    extern void killed(fpsent *d, fpsent *actor);
    extern void timeupdate(int timeremain);
    extern void msgsound(int n, fpsent *d = NULL);

    enum
    {
        HICON_BLUE_ARMOUR = 0,
        HICON_GREEN_ARMOUR,
        HICON_YELLOW_ARMOUR,

        HICON_HEALTH,

        HICON_FIST,
        HICON_SG,
        HICON_CG,
        HICON_RL,
        HICON_RIFLE,
        HICON_GL,
        HICON_PISTOL,

        HICON_QUAD,

        HICON_RED_FLAG,
        HICON_BLUE_FLAG
    };

    extern void drawicon(int icon, int x, int y);
 
    // client
    extern bool connected, remote, demoplayback, spectator;

    extern int parseplayer(const char *arg);
    extern void addmsg(int type, const char *fmt = NULL, ...);
    extern void switchname(const char *name);
    extern void switchteam(const char *name);
    extern void switchplayermodel(int playermodel);
    extern void sendmapinfo();
    extern void stopdemo();
    extern void changemap(const char *name, int mode);
    extern void c2sinfo();

    // monster
    struct monster;
    extern vector<monster *> monsters;

    extern void clearmonsters();
    extern void preloadmonsters();
    extern void updatemonsters(int curtime);
    extern void rendermonsters();
    extern void suicidemonster(monster *m);
    extern void hitmonster(int damage, monster *m, fpsent *at, const vec &vel, int gun);
    extern void monsterkilled();
    extern void endsp(bool allkilled);
    extern void spsummary(int accuracy);

    // movable
    struct movable;
    extern vector<movable *> movables;

    extern void clearmovables();
    extern void updatemovables(int curtime);
    extern void rendermovables();
    extern void suicidemovable(movable *m);
    extern void hitmovable(int damage, movable *m, fpsent *at, const vec &vel, int gun);

    // weapon
    extern void shoot(fpsent *d, const vec &targ);
    extern void shoteffects(int gun, const vec &from, const vec &to, fpsent *d, bool local);
    extern void explode(bool local, fpsent *owner, const vec &v, dynent *safe, int dam, int gun);
    extern void damageeffect(int damage, fpsent *d, bool thirdperson = true);
    extern void superdamageeffect(const vec &vel, fpsent *d);
    extern bool intersect(dynent *d, const vec &from, const vec &to);
    extern dynent *intersectclosest(const vec &from, const vec &to, fpsent *at);
    extern void clearbouncers(); 
    extern void updatebouncers(int curtime);
    extern void removebouncers(fpsent *owner);
    extern void renderbouncers();
    extern void clearprojectiles();
    extern void updateprojectiles(int curtime);
    extern void removeprojectiles(fpsent *owner);
    extern void renderprojectiles();
    extern void preloadbouncers();

    // scoreboard
    extern void showscores(bool on);
    extern void getbestplayers(vector<fpsent *> &best);
    extern void getbestteams(vector<const char *> &best);

    // render
    struct playermodelinfo
    {
        const char *ffa, *blueteam, *redteam, *hudguns,
                   *vwep, *quad, *armour[3],
                   *ffaicon, *blueicon, *redicon;
        bool ragdoll, selectable;
    };

    extern int playermodel, teamskins, testteam;

    extern void saveragdoll(fpsent *d);
    extern void clearragdolls();
    extern void moveragdolls();
    extern const playermodelinfo &getplayermodelinfo(fpsent *d);
    extern void swayhudgun(int curtime);
    extern vec hudgunorigin(int gun, const vec &from, const vec &to, fpsent *d);
}

namespace server
{
    extern const char *modename(int n, const char *unknown = "unknown"); 
    extern const char *mastermodename(int n, const char *unknown = "unknown");
    extern void startintermission();
    extern void stopdemo();
    extern void forcemap(const char *map, int mode);
    extern void hashpassword(int cn, int sessionid, const char *pwd, char *result);
    extern int msgsizelookup(int msg);
    extern bool serveroption(const char *arg);

    extern int getUniqueIdFromInfo(void *ci); // INTENSITY
    extern ScriptValuePtr createScriptingEntity(int cn, std::string _class=""); // INTENSITY: Called when logging in,
                                               // and also when the map restarts (need a new entity)
    extern void setAdmin(int clientNumber, bool isAdmin); // INTENSITY: Called when logging in,
                                                          // and this is later applied whenever
                                                          // creating the scripting logic entity (login and map restart)

    extern bool isAdmin(int clientNumber); // INTENSITY

    //! Clears info related to the current scenario, as a new one is being prepared
    extern void resetScenario();

    //! Update the current scenario being run by the client. The server uses this to make sure the
    //! client is running the same scenario when it accepts certain world update messages from the
    //! client.
    extern void setClientScenario(int clientNumber, std::string scenarioCode);

    extern bool isRunningCurrentScenario(int clientNumber);
}

