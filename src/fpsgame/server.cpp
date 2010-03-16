
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

#include "cube.h"
#include "game.h"

#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>
#define _dup    dup
#define _fileno fileno
#endif

#include "intensity.h"

#include "server_system.h"
#include "network_system.h"
#include "message_system.h"

#include "fpsclient_interface.h"
#include "script_engine_manager.h"
#include "utility.h"

#ifdef CLIENT
    #include "client_system.h"
#endif


namespace server
{
    struct server_entity            // server side version of "entity" type
    {
    };

    struct gamestate : fpsstate
    {
        vec o;
        int state, editstate;
        int lifesequence;

        gamestate() : state(CS_DEAD), editstate(CS_DEAD), lifesequence(-1) {}

        void reset()
        {
            if(state!=CS_SPECTATOR) state = editstate = CS_DEAD;
            lifesequence = 0;
            respawn();
        }

        void respawn()
        {
            fpsstate::respawn();
            o = vec(-1e10f, -1e10f, -1e10f);
        }
    };

    struct clientinfo
    {
        int clientnum;

        std::string username; // Kripken: DV username. Is "" when not logged in
        int         uniqueId; // Kripken: Unique ID in the current module of this client
        bool        isAdmin; // Kripken: Whether we are admins of this map, and can edit it

        string name, team;
        int privilege;
        bool spectator, connected, local, timesync, wantsmaster;
        int gameoffset, lastevent;
        gamestate state;
        vector<uchar> position, messages; // Kripken: These are buffers for channels 0 (positions) and 1 (normal messages)

        //! The current scenario being run by the client
        bool runningCurrentScenario;

        clientinfo() { reset(); }

        void mapchange()
        {
            state.reset();
            timesync = false;
            lastevent = 0;

            runningCurrentScenario = false;
        }

        void reset()
        {
            username = ""; // Kripken
            uniqueId = DUMMY_SINGLETON_CLIENT_UNIQUE_ID - 5; // Kripken: Negative, and also different from dummy singleton
            isAdmin = false; // Kripken

            name[0] = team[0] = 0;
            privilege = PRIV_NONE;
            connected = spectator = local = wantsmaster = false;
            position.setsizenodelete(0);
            messages.setsizenodelete(0);
            mapchange();
        }
    };

    void *newclientinfo()
    {
        return new clientinfo;
    }

    void deleteclientinfo(void *ci)
    {
        // Delete the logic entity
        clientinfo *_ci = (clientinfo*)ci;
        int uniqueId = _ci->uniqueId;

        // If there are entities to remove, remove. For NPCs/bots, however, do not do this - we are in fact being called from there
        // Also do not do this if the uniqueId is negative - it means we are disconnecting this client *before* a scripting
        // entity is actually created for them (this can happen in the rare case of a network error causing a disconnect
        // between ENet connection and completing the login process).
        if (ScriptEngineManager::hasEngine() && !_ci->local && uniqueId >= 0)
            ScriptEngineManager::getGlobal()->call("removeEntity", uniqueId); // Will also disconnect from FPSClient
        
//        // Remove from internal mini FPSclient as well
//        FPSClientInterface::clientDisconnected(_ci->clientnum); // XXX No - do in parallel to character

        delete (clientinfo *)ci;
    } 

    clientinfo *getinfo(int n)
    {
        if(n < MAXCLIENTS) return (clientinfo *)getclientinfo(n);

        return NULL; // TODO: If we want bots
//        n -= MAXCLIENTS;
//        return bots.inrange(n) ? bots[n] : NULL;
    }

    // Kripken: Conveniences
    std::string& getUsername(int clientNumber)
    {
        clientinfo *ci = (clientinfo *)getinfo(clientNumber);
        static std::string DUMMY = ""; // Need the dummy, because ci may be NULL - there are empty slots in server:clients
        return (ci ? ci->username : DUMMY);
    }

    int& getUniqueId(int clientNumber)
    {
        clientinfo *ci = (clientinfo *)getinfo(clientNumber);
        static int DUMMY = -1;
        return (ci ? ci->uniqueId : DUMMY); // Kind of a hack, but we do use this to both set and get... maybe need two methods separate
    }


    struct worldstate
    {
        int uses;
        vector<uchar> positions, messages;
    };

    int gamemode = 0;
    int gamemillis = 0;

    string serverdesc = "";
    string smapname = "";
    enet_uint32 lastsend = 0;

    vector<clientinfo *> connects, clients;
    vector<worldstate *> worldstates;
    bool reliablemessages = false;

    struct servmode
    {
        virtual ~servmode() {}

        virtual void entergame(clientinfo *ci) {}
        virtual void leavegame(clientinfo *ci, bool disconnecting = false) {}

        virtual void moved(clientinfo *ci, const vec &oldpos, const vec &newpos) {}
        virtual bool canspawn(clientinfo *ci, bool connecting = false) { return true; }
        virtual void spawned(clientinfo *ci) {}
        virtual void died(clientinfo *victim, clientinfo *actor) {}
        virtual bool canchangeteam(clientinfo *ci, const char *oldteam, const char *newteam) { return true; }
        virtual void changeteam(clientinfo *ci, const char *oldteam, const char *newteam) {}
        virtual void initclient(clientinfo *ci, ucharbuf &p, bool connecting) {}
        virtual void update() {}
        virtual void reset(bool empty) {}
        virtual void intermission() {}
    };

    int nonspectators(int exclude = -1)
    {
        int n = 0;
        loopv(clients) if(i!=exclude && clients[i]->state.state!=CS_SPECTATOR) n++;
        return n;
    }

    void spawnstate(clientinfo *ci)
    {
        gamestate &gs = ci->state;
        gs.spawnstate(gamemode);
        gs.lifesequence++;
    }

    void sendspawn(clientinfo *ci)
    {
        spawnstate(ci);
    }

    struct arenaservmode : servmode
    {
        int arenaround;

        arenaservmode() : arenaround(0) {}

        bool canspawn(clientinfo *ci, bool connecting = false) 
        { 
            if(connecting && nonspectators(ci->clientnum)<=1) return true;
            return false; 
        }

        void reset(bool empty)
        {
            arenaround = 0;
        }
    
        void update()
        {
        }
    };

    servmode *smode = NULL;

    void *newinfo() { return new clientinfo; }
    void deleteinfo(void *ci) { delete (clientinfo *)ci; } 
    int getUniqueIdFromInfo(void *ci) { return ((clientinfo *)ci)->uniqueId; }

    void sendservmsg(const char *s) { sendf(-1, 1, "ris", SV_SERVMSG, s); }

    void resetitems() 
    { 
    }

    int spawntime(int type)
    {
        assert(0);
        return 1;
    }
        
    void changemap(const char *s, int mode)
    {
    }

    bool duplicatename(clientinfo *ci, char *name)
    {
        if(!name) name = ci->name;
        loopv(clients) if(clients[i]!=ci && !strcmp(name, clients[i]->name)) return true;
        return false;
    }

    char *colorname(clientinfo *ci, char *name = NULL)
    {
        if(!name) name = ci->name;
        if(name[0] && !duplicatename(ci, name)) return name;
        static string cname;
        formatstring(cname)("%s \fs\f5(%d)\fr", name, ci->clientnum);
        return cname;
    }   

    int checktype(int type, clientinfo *ci)
    {
        if(ci && ci->local) return type;
#if 0
        // other message types can get sent by accident if a master forces spectator on someone, so disabling this case for now and checking for spectator state in message handlers
        // spectators can only connect and talk
        static int spectypes[] = { SV_INITC2S, SV_POS, SV_TEXT, SV_PING, SV_CLIENTPING, SV_GETMAP, SV_SETMASTER };
        if(ci && ci->state.state==CS_SPECTATOR && !ci->privilege)
        {
            loopi(sizeof(spectypes)/sizeof(int)) if(type == spectypes[i]) return type;
            return -1;
        }
#endif
        // only allow edit messages in coop-edit mode
//        if(type>=SV_EDITENT && type<=SV_GETMAP && gamemode!=1) return -1; // Kripken: FIXME: For now, allowing editing all the time
        // server only messages
        static int servtypes[] = { SV_SERVINFO, SV_INITCLIENT, SV_WELCOME, SV_MAPRELOAD, SV_SERVMSG, SV_DAMAGE, SV_HITPUSH, SV_SHOTFX, SV_DIED, SV_SPAWNSTATE, SV_FORCEDEATH, SV_ITEMACC, SV_ITEMSPAWN, SV_TIMEUP, SV_CDIS, SV_CURRENTMASTER, SV_PONG, SV_RESUME, SV_BASESCORE, SV_BASEINFO, SV_BASEREGEN, SV_ANNOUNCE, SV_SENDDEMOLIST, SV_SENDDEMO, SV_DEMOPLAYBACK, SV_SENDMAP, SV_DROPFLAG, SV_SCOREFLAG, SV_RETURNFLAG, SV_RESETFLAG, SV_INVISFLAG, SV_CLIENT, SV_AUTHCHAL, SV_INITAI };
        if(ci) loopi(sizeof(servtypes)/sizeof(int)) if(type == servtypes[i])
        {
            Logging::log(Logging::ERROR, "checktype has decided to return -1 for %d\r\n", type);
            return -1;
        }
        return type;
    }

    void cleanworldstate(ENetPacket *packet)
    {
        loopv(worldstates)
        {
            worldstate *ws = worldstates[i];
            if(ws->positions.inbuf(packet->data) || ws->messages.inbuf(packet->data)) ws->uses--;
            else continue;
            if(!ws->uses)
            {
                delete ws;
                worldstates.remove(i);
            }
            break;
        }
    }

    bool buildworldstate()
    {
        static struct { int posoff, msgoff, msglen; } pkt[MAXCLIENTS];
        worldstate &ws = *new worldstate;

        loopv(clients)
        {
            clientinfo &ci = *clients[i];

            if(ci.position.empty()) pkt[i].posoff = -1;
            else
            {
                Logging::log(Logging::INFO, "SERVER: prepping relayed SV_POS data for sending %d, size: %d\r\n", ci.clientnum,
                             ci.position.length());

                pkt[i].posoff = ws.positions.length();
                loopvj(ci.position) ws.positions.add(ci.position[j]);
            }

            if(ci.messages.empty())
                pkt[i].msgoff = -1;
            else
            {
                pkt[i].msgoff = ws.messages.length();
                ucharbuf p = ws.messages.reserve(16);
                putint(p, SV_CLIENT);
                putint(p, ci.clientnum);
                putuint(p, ci.messages.length());
                ws.messages.addbuf(p);
                loopvj(ci.messages) ws.messages.add(ci.messages[j]);
                pkt[i].msglen = ws.messages.length() - pkt[i].msgoff;
            }
        }

        Logging::log(Logging::INFO, "SERVER: prepping sum of relayed data for sending, size: %d,%d\r\n", ws.positions.length(), ws.messages.length());

        int psize = ws.positions.length(), msize = ws.messages.length();
//        if(psize) recordpacket(0, ws.positions.getbuf(), psize);
//        if(msize) recordpacket(1, ws.messages.getbuf(), msize);
        loopi(psize) { uchar c = ws.positions[i]; ws.positions.add(c); }
        loopi(msize) { uchar c = ws.messages[i]; ws.messages.add(c); }
        ws.uses = 0;

        loopv(clients)
        {
            clientinfo &ci = *clients[i];

            Logging::log(Logging::INFO, "Processing update relaying for %d:%d\r\n", ci.clientnum, ci.uniqueId);

#ifdef SERVER
            // Kripken: FIXME: Send position updates only to real clients, not local ones. For multiple local
            // ones, a single manual sending suffices, which is done to the singleton dummy client
            fpsent* currClient = dynamic_cast<fpsent*>( FPSClientInterface::getPlayerByNumber(ci.clientnum) );
            if (!currClient) continue; // We have a server client, but no FPSClient client yet, because we have not yet
                                       // finished the player's login, only after which do we create the scripting entity,
                                       // which then gets a client added to the FPSClient (and the remote client's FPSClient)
            if (!currClient->serverControlled || ci.uniqueId == DUMMY_SINGLETON_CLIENT_UNIQUE_ID) // Send also to singleton dummy client
#endif
            {

                ENetPacket *packet;
                if(psize && (pkt[i].posoff<0 || psize-ci.position.length()>0))
                {
                    // Kripken: Trickery with offsets here prevents relaying back to the same client. Ditto below
                    packet = enet_packet_create(&ws.positions[pkt[i].posoff<0 ? 0 : pkt[i].posoff+ci.position.length()], 
                                                pkt[i].posoff<0 ? psize : psize-ci.position.length(), 
                                                ENET_PACKET_FLAG_NO_ALLOCATE);

                    Logging::log(Logging::INFO, "Sending positions packet to %d\r\n", ci.clientnum);

                    sendpacket(ci.clientnum, 0, packet); // Kripken: Sending queue of position changes, in channel 0?

                    if(!packet->referenceCount) enet_packet_destroy(packet);
                    else { ++ws.uses; packet->freeCallback = cleanworldstate; }
                }

                if(msize && (pkt[i].msgoff<0 || msize-pkt[i].msglen>0))
                {
                    packet = enet_packet_create(&ws.messages[pkt[i].msgoff<0 ? 0 : pkt[i].msgoff+pkt[i].msglen], 
                                                pkt[i].msgoff<0 ? msize : msize-pkt[i].msglen, 
                                                (reliablemessages ? ENET_PACKET_FLAG_RELIABLE : 0) | ENET_PACKET_FLAG_NO_ALLOCATE);

                    Logging::log(Logging::INFO, "Sending messages packet to %d\r\n", ci.clientnum);

                    sendpacket(ci.clientnum, 1, packet);
                    if(!packet->referenceCount) enet_packet_destroy(packet);
                    else { ++ws.uses; packet->freeCallback = cleanworldstate; }
                }

            }

            // Kripken: Do this, if we sent to this client or if not - either way. Note that the only reason 
            // Sauer does this in this loop and not elsewhere is so that we can do the index trickery above to prevent
            // relaying back to the same client
            ci.position.setsizenodelete(0);
            ci.messages.setsizenodelete(0); // Kripken: Client's relayed messages have been processed; clear them
        }

        reliablemessages = false;
        if(!ws.uses) 
        {
            delete &ws;
            return false;
        }
        else 
        {
            worldstates.add(&ws); 
            return true;
        }
    }

    bool sendpackets()
    {
        if(clients.empty()) return false;
        enet_uint32 curtime = enet_time_get()-lastsend;
        if(curtime<33) return false; // kripken: Server sends packets at most every 33ms? FIXME: fast rate, we might slow or dynamic this
        bool flush = buildworldstate();
        lastsend += curtime - (curtime%33);
        return flush;
    }

    void parsepacket(int sender, int chan, packetbuf &p)     // has to parse exactly each byte of the packet
    {
        Logging::log(Logging::INFO, "Server: Parsing packet, %d-%d\r\n", sender, chan);

        if(sender<0) return;
        if(chan==2) // Kripken: Channel 2 is, just like with the client, for file transfers
        {
            assert(0); // We do file transfers completely differently
        }
        if(p.packet->flags&ENET_PACKET_FLAG_RELIABLE) reliablemessages = true;
        char text[MAXTRANS];
        int cn = -1, type;
        clientinfo *ci = sender>=0 ? (clientinfo *)getinfo(sender) : NULL;

        if (ci == NULL) Logging::log(Logging::ERROR, "ci is null. Sender: %ld\r\n", (long) sender); // Kripken

        // Kripken: QUEUE_MSG puts the incoming message into the out queue. So after the server parses it,
        // it sends it to all *other* clients. This is in tune with the server-as-a-relay-server approach in Sauer.
        // Note that it puts the message in the messages for the current client. This is apparently what prevents
        // the client from getting it back.
        #define QUEUE_MSG { if(ci->uniqueId == DUMMY_SINGLETON_CLIENT_UNIQUE_ID || !ci->local) while(curmsg<p.length()) ci->messages.add(p.buf[curmsg++]); } // Kripken: We need to send messages through the dummy singleton
        #define QUEUE_BUF(size, body) { \
            if(!ci->local) \
            { \
                curmsg = p.length(); \
                ucharbuf buf = ci->messages.reserve(size); \
                { body; } \
                ci->messages.addbuf(buf); \
            } \
        }
        #define QUEUE_INT(n) QUEUE_BUF(5, putint(buf, n))
        #define QUEUE_UINT(n) QUEUE_BUF(4, putuint(buf, n))
        #define QUEUE_STR(text) QUEUE_BUF(2*strlen(text)+1, sendstring(text, buf))
        int curmsg;
        while((curmsg = p.length()) < p.maxlen)
        {
          type = checktype(getint(p), ci);  // kripken: checks type is valid for situation
          Logging::log(Logging::INFO, "Server: Parsing a message of type %d\r\n", type);
          switch(type)
          { // Kripken: Mangling sauer indentation as little as possible
            case SV_POS: // Kripken: position update for a client
            {
                NetworkSystem::PositionUpdater::QuantizedInfo info;
                info.generateFrom(p);

                // Kripken: This is a dummy read, we don't do anything with it (but relay)
                // But we do disconnect on errors
                cn = info.clientNumber;
                if(cn<0 || cn>=getnumclients()) // || cn!=sender) - we have multiple NPCs on single server TODO: But apply to clients?
                {
                    disconnect_client(sender, DISC_CN);
                    return;
                }

#ifdef CLIENT
                if ( !isRunningCurrentScenario(sender) ) break; // Silently ignore info from previous scenario
#endif

                //if(!ci->local) // Kripken: We relay even our local clients, PCs need to hear about NPC positions
                // && (ci->state.state==CS_ALIVE || ci->state.state==CS_EDITING)) // Kripken: We handle death differently
                {
                    Logging::log(Logging::INFO, "SERVER: relaying SV_POS data for client %d\r\n", cn);

                    // Modify the info depending on various server parameters
                    NetworkSystem::PositionUpdater::processServerPositionReception(info);

                    // Queue the info to be sent to the clients
                    int maxLength = p.length()*2 + 100; // Kripken: The server almost always DECREASES the size, but be careful
                    unsigned char* data = new unsigned char[maxLength];
                    ucharbuf temp(data, maxLength);
                    info.applyToBuffer(temp);
                    ci->position.setsizenodelete(0);
                    loopk(temp.length()) ci->position.add(temp.buf[k]);
                    delete[] data;
                }
//                if(smode && ci->state.state==CS_ALIVE) smode->moved(ci, oldpos, ci->state.o); // Kripken:Gametype(ctf etc.)-specific stuff
                break;
            }

            case SV_TEXT:
            {
                getstring(text, p);
                filtertext(text, text);

                REFLECT_PYTHON( signal_text_message );
                signal_text_message(sender, text);

                if (!ScriptEngineManager::hasEngine() ||
                    !ScriptEngineManager::getGlobal()->getProperty("ApplicationManager")->getProperty("instance")->call(
                        "handleTextMessage",
                        ScriptValueArgs().append(ci->uniqueId).append(std::string(text))
                    )->getBool())
                {
                    // No engine, or script did not completely handle this message, so relay it the normal way
                    QUEUE_INT(type);
                    QUEUE_STR(text);
                }
                break;
            }

            case SV_EDITVAR:
            {
                int type = getint(p);
                getstring(text, p);
                switch(type)
                {
                    case ID_VAR: getint(p); break;
                    case ID_FVAR: getfloat(p); break;
                    case ID_SVAR: getstring(text, p);
                }
                if(ci && ci->state.state!=CS_SPECTATOR) QUEUE_MSG;
                break;
            }

            case SV_PING:
                sendf(sender, 1, "i2", SV_PONG, getint(p));
                break;

            case SV_CLIENTPING:
                getint(p);
//                QUEUE_MSG; // Kripken: Do not let clients know other clients' pings
                break;

            default:
            {
                Logging::log(Logging::DEBUG, "Server: Handling a non-typical message: %d\r\n", type);
                if (!MessageSystem::MessageManager::receive(type, -1, sender, p))
                {
                    Logging::log(Logging::DEBUG, "Relaying Sauer protocol message: %d\r\n", type);

                    int size = msgsizelookup(type);
                    if(size==-1) { disconnect_client(sender, DISC_TAGT); return; }
                    if(size>0) loopi(size-1) getint(p);

                    if ( !isRunningCurrentScenario(sender) ) break; // Silently ignore info from previous scenario

                    if(ci && ci->state.state!=CS_SPECTATOR) QUEUE_MSG;

                    Logging::log(Logging::DEBUG, "Relaying complete\r\n");
                }
                break;
            }
          } // Kripken: Left indentation as per Sauer
        }
    }

    void serverupdate(int _lastmillis, int _totalmillis)
    {
        curtime = _lastmillis - lastmillis;
        gamemillis += curtime;
        lastmillis = _lastmillis;
        totalmillis = _totalmillis;

        if(smode) smode->update();
    }

    bool serveroption(char *arg)
    {
        if(arg[0]=='-') switch(arg[1])
        {
            case 'n': copystring(serverdesc, &arg[2]); return true;
        }
        return false;
    }

    void serverinit()
    {
        smapname[0] = '\0';
        resetitems();
    }
   
    void localconnect(int n)
    {
        clientinfo *ci = (clientinfo *)getinfo(n);
        ci->clientnum = n;
        ci->local = true;

        clients.add(ci);
    }

    void localdisconnect(int n)
    {
        clientinfo *ci = (clientinfo *)getinfo(n);
        if(smode) smode->leavegame(ci, true);
        clients.removeobj(ci);
    }

    void setAdmin(int clientNumber, bool isAdmin)
    {
        Logging::log(Logging::DEBUG, "setAdmin for client %d\r\n", clientNumber);

        clientinfo *ci = (clientinfo *)getinfo(clientNumber);
        if (!ci) return; // May have been kicked just before now
        ci->isAdmin = isAdmin;

        if (ci->isAdmin && ci->uniqueId >= 0) // If an entity was already created, update it
            ScriptEngineManager::runScript("getEntity(" + Utility::toString(ci->uniqueId) + ")._canEdit = true;");
    }

    bool isAdmin(int clientNumber)
    {
        clientinfo *ci = (clientinfo *)getinfo(clientNumber);
        if (!ci) return false;
        return ci->isAdmin;
    }

    // INTENSITY: Called when logging in, and also when the map restarts (need a new entity).
    // Creates a new scripting entity, in the process of which a uniqueId is generated.
    ScriptValuePtr createScriptingEntity(int cn, std::string _class)
    {
#ifdef CLIENT
        assert(0);
        return ScriptValuePtr();
#else // SERVER
        // cn of -1 means "all of them"
        if (cn == -1)
        {
            for (int i = 0; i < getnumclients(); i++)
            {
                clientinfo *ci = (clientinfo *)getinfo(i);
                if (!ci) continue;
                if (ci->uniqueId == DUMMY_SINGLETON_CLIENT_UNIQUE_ID) continue;
                if (ci->local) continue; // No need for NPCs created during the map script - they already exist

//                REFLECT_PYTHON( Clients );
//                EVAL_PYTHON(bool, loggedIn, Utility::toString(i) + " in Clients._map");
//                if (!loggedIn) continue; // Only create entities for people actually logged in, not those
//                                         // pending login. Also, they will create their own entities when
//                                         // the login finishes, so it would be a bug to do it here as well.

                Logging::log(Logging::DEBUG, "scriptingEntities creation: Adding %d\r\n", i);

                createScriptingEntity(i);
            }
            return ScriptEngineManager::getNull();
        }

        assert(cn >= 0);
        clientinfo *ci = (clientinfo *)getinfo(cn);
        if (!ci)
        {
            Logging::log(Logging::WARNING, "Asked to create a player entity for %d, but no clientinfo (perhaps disconnected meanwhile)\r\n", cn);
            return ScriptEngineManager::getNull();
        }

        fpsent* fpsEntity = dynamic_cast<fpsent*>(FPSClientInterface::getPlayerByNumber(cn));
        if (fpsEntity)
        {
            // Already created an entity
            Logging::log(Logging::WARNING, "createScriptingEntity(%d): already have fpsEntity, and hence scripting entity\r\n", cn);
            return ScriptEngineManager::getNull();
        }

        // Use the PC class, unless told otherwise
        if (_class == "")
             _class = ScriptEngineManager::runScript("ApplicationManager.instance.getPcClass()")->getString();

        Logging::log(Logging::DEBUG, "Creating player entity: %s, %d", _class.c_str(), cn);

        int uniqueId = ScriptEngineManager::runScript("getNewUniqueId()")->getInt();

        // Notify of uniqueId *before* creating the entity, so when the entity is created, player realizes it is them
        // and does initial connection correctly
        if (!ci->local)
            MessageSystem::send_YourUniqueId(cn, uniqueId);

        ci->uniqueId = uniqueId;

        ScriptEngineManager::runScript(
            "newEntity('" + _class + "', { clientNumber: " + Utility::toString(cn) + " }, " + Utility::toString(uniqueId) + ", true)"
        );

        assert( ScriptEngineManager::runScript("getEntity(" + Utility::toString(uniqueId) + ").clientNumber")->getInt() == cn);

        // Add admin status, if relevant
        if (ci->isAdmin)
            ScriptEngineManager::runScript("getEntity(" + Utility::toString(uniqueId) + ")._canEdit = true;");

        // Add nickname
        ScriptEngineManager::runScript("getEntity(" + Utility::toString(uniqueId) + ")")->setProperty(
            "_name",
             getUsername(cn)
        );

        // For NPCs/Bots, mark them as such and prepare them, exactly as the players do on the client for themselves
        if (ci->local)
        {
            fpsEntity = dynamic_cast<fpsent*>(FPSClientInterface::getPlayerByNumber(cn)); // It was created since fpsEntity was def'd
            assert(fpsEntity);

            fpsEntity->serverControlled = true; // Mark this as an NPC the server should control

            FPSClientInterface::spawnPlayer(fpsEntity);
        }

        return ScriptEngineManager::runScript("getEntity(" + Utility::toString(uniqueId) + ")");
#endif
    }

    int clientconnect(int n, uint ip)
    {
        Logging::log(Logging::DEBUG, "server::clientconnect: %d\r\n", n);

        clientinfo *ci = (clientinfo *)getinfo(n);
        ci->clientnum = n;
        clients.add(ci);

//        FPSClientInterface::newClient(n); // INTENSITY: Also connect to the server's internal client - XXX NO - do in parallel to client

        // Start the connection handshake process
        MessageSystem::send_InitS2C(n, n, PROTOCOL_VERSION);

        return DISC_NONE;
    }

    void clientdisconnect(int n) 
    { 
        Logging::log(Logging::DEBUG, "server::clientdisconnect: %d\r\n", n);
        INDENT_LOG(Logging::DEBUG);

        clientinfo *ci = (clientinfo *)getinfo(n);
        if(smode) smode->leavegame(ci, true);
        clients.removeobj(ci);

        REFLECT_PYTHON( on_logout );
        on_logout(n);
    }

    const char *servername() { return "sauerbratenserver"; }
    int serverinfoport(int servport) { return SAUERBRATEN_SERVINFO_PORT; }
    int laninfoport() { return -1; }
    int serverport(int infoport)
    {
        #ifdef CLIENT
            return ClientSystem::currPort;
        #else // SERVER
            if (pythonInitialized)
                return Utility::Config::getInt("Network", "port", SAUERBRATEN_SERVER_PORT);
            else
                return 0; // If no Python yet, no worry - we will be called again when the time is right
        #endif
    }

    #include "extinfo.h"

    void serverinforeply(ucharbuf &req, ucharbuf &p)
    {
        if(!getint(req))
        {
            extserverinforeply(req, p);
            return;
        }

        putint(p, clients.length());
        putint(p, 5);                   // number of attrs following
        putint(p, PROTOCOL_VERSION);    // a // generic attributes, passed back below
        putint(p, gamemode);            // b
        putint(p, 10); //minremain);    // c
        putint(p, maxclients);
        putint(p, 0);//mastermode);
        sendstring(smapname, p);
        sendstring(serverdesc, p);
        sendserverinforeply(p);
    }

    bool servercompatible(char *name, char *sdec, char *map, int ping, const vector<int> &attr, int np)
    {
        return attr.length() && attr[0]==PROTOCOL_VERSION;
    }

    int msgsizelookup(int msg)
    {
        // Kripken: Moved here from game.h, to prevent warnings
        static int msgsizes[] =               // size inclusive message token, 0 for variable or not-checked sizes
        {
            SV_CONNECT, 0, SV_SERVINFO, 5, SV_WELCOME, 2, SV_INITCLIENT, 0, SV_POS, 0, SV_TEXT, 0, SV_SOUND, 2, SV_CDIS, 2,
            SV_SHOOT, 0, SV_EXPLODE, 0, SV_SUICIDE, 1,
            SV_DIED, 4, SV_DAMAGE, 6, SV_HITPUSH, 7, SV_SHOTFX, 9,
            SV_TRYSPAWN, 1, SV_SPAWNSTATE, 14, SV_SPAWN, 3, SV_FORCEDEATH, 2,
            SV_GUNSELECT, 2, SV_TAUNT, 1,
            SV_MAPCHANGE, 0, SV_MAPVOTE, 0, SV_ITEMSPAWN, 2, SV_ITEMPICKUP, 2, SV_ITEMACC, 3,
            SV_PING, 2, SV_PONG, 2, SV_CLIENTPING, 2,
            SV_TIMEUP, 2, SV_MAPRELOAD, 1, SV_FORCEINTERMISSION, 1,
            SV_SERVMSG, 0, SV_ITEMLIST, 0, SV_RESUME, 0,
            SV_EDITMODE, 2, SV_EDITENT, 11, SV_EDITF, 16, SV_EDITT, 16, SV_EDITM, 16, SV_FLIP, 14, SV_COPY, 14, SV_PASTE, 14, SV_ROTATE, 15, SV_REPLACE, 16, SV_DELCUBE, 14, SV_REMIP, 1, SV_NEWMAP, 2, SV_GETMAP, 1, SV_SENDMAP, 0, SV_EDITVAR, 0,
            SV_MASTERMODE, 2, SV_KICK, 2, SV_CLEARBANS, 1, SV_CURRENTMASTER, 3, SV_SPECTATOR, 3, SV_SETMASTER, 0, SV_SETTEAM, 0,
            SV_BASES, 0, SV_BASEINFO, 0, SV_BASESCORE, 0, SV_REPAMMO, 1, SV_BASEREGEN, 6, SV_ANNOUNCE, 2,
            SV_LISTDEMOS, 1, SV_SENDDEMOLIST, 0, SV_GETDEMO, 2, SV_SENDDEMO, 0,
            SV_DEMOPLAYBACK, 3, SV_RECORDDEMO, 2, SV_STOPDEMO, 1, SV_CLEARDEMOS, 2,
            SV_TAKEFLAG, 2, SV_RETURNFLAG, 3, SV_RESETFLAG, 4, SV_INVISFLAG, 3, SV_TRYDROPFLAG, 1, SV_DROPFLAG, 6, SV_SCOREFLAG, 6, SV_INITFLAGS, 6,
            SV_SAYTEAM, 0,
            SV_CLIENT, 0,
            SV_AUTHTRY, 0, SV_AUTHCHAL, 0, SV_AUTHANS, 0, SV_REQAUTH, 0,
            SV_PAUSEGAME, 2,
            SV_ADDBOT, 2, SV_DELBOT, 1, SV_INITAI, 0, SV_FROMAI, 2, SV_BOTLIMIT, 2, SV_BOTBALANCE, 2,
            SV_MAPCRC, 0, SV_CHECKMAPS, 1,
            SV_SWITCHNAME, 0, SV_SWITCHMODEL, 2, SV_SWITCHTEAM, 0,
            -1
        };
        for(int *p = msgsizes; *p>=0; p += 2) if(*p==msg) return p[1];
        return -1;
    }

    void serverupdate()
    {
        gamemillis += curtime;
    }

    void recordpacket(int chan, void *data, int len)
    {
    }

    bool allowbroadcast(int n)
    {
        clientinfo *ci = (clientinfo *)getinfo(n);
        return ci && ci->connected; // XXX FIXME - code is wrong, but rare crashes if fix it by removing 'connected' bit
    }

    int reserveclients() { return 3+1; }

    void processmasterinput(const char *cmd, int cmdlen, const char *args) { assert(0); };

    int masterport() { assert(0); return -1; };

    const char *defaultmaster() { return "nada/nullo/"; } ;
//    const char *getdefaultmaster() { return "sauerbraten.org/masterserver/"; } 


    //// Scenario stuff

    void resetScenario()
    {
        loopv(clients)
        {
            clientinfo *ci = clients[i];
            ci->mapchange(); // Old sauer method to reset scenario/game/map transient info
        }
    }

    void setClientScenario(int clientNumber, std::string scenarioCode)
    {
        clientinfo *ci = getinfo(clientNumber);
        if (!ci) return;

        REFLECT_PYTHON( World );
        std::string serverScenarioCode = boost::python::extract<std::string>( World.attr("scenario_code") );
        ci->runningCurrentScenario = (scenarioCode == serverScenarioCode);
    }

    bool isRunningCurrentScenario(int clientNumber)
    {
        clientinfo *ci = getinfo(clientNumber);
        if (!ci) return false;

        return ci->runningCurrentScenario;
    }
};

