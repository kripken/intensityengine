
// Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
// This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

#include "cube.h"
#include "engine.h"
#include "game.h"

#include "editing_system.h"
#include "client_system.h"
#include "message_system.h"
#include "network_system.h"
#include "utility.h"

#ifdef SERVER
    #include "server_system.h"
#endif

#ifdef CLIENT
    #include "client_engine_additions.h"
    extern int enthover;
#endif

// Enable to let *server* do physics for players - useful for debugging. Must also be defined in fps.cpp!
#define SERVER_DRIVEN_PLAYERS 0

namespace game
{
    int lastping = 0; // Kripken: Last time we sent out a ping

    bool connected = false, remote = false;

    bool spectator = false;

    void parsemessages(int cn, fpsent *d, ucharbuf &p);

    int numchannels() { return 3; }

    void initclientnet()
    {
    }

    void writeclientinfo(stream *f)
    {
    }

    void gameconnect(bool _remote)
    {
        connected = true;
        remote = _remote;
#ifdef CLIENT
        if(editmode) toggleedit();
#endif
    }

    void gamedisconnect(bool cleanup)
    {
        Logging::log(Logging::DEBUG, "client.h: gamedisconnect()\r\n");
//        if(remote) stopfollowing(); Kripken
        connected = false;
        player1->clientnum = -1;
        player1->lifesequence = 0;
        player1->privilege = PRIV_NONE;
        spectator = false;
//        loopv(players) if(players[i]) clientdisconnected(i, false); Kripken: When we disconnect, we should shut down anyhow...
        Logging::log(Logging::WARNING, "Not doing normal Sauer disconnecting of other clients\r\n");

        #ifdef CLIENT
            ClientSystem::onDisconnect();
        #else
            assert(0); // What to do...?
        #endif

        if (player->ragdoll)
            cleanragdoll(player);
    }


    bool allowedittoggle()
    {
#ifdef CLIENT
        if (!ClientSystem::isAdmin())
        {
            conoutf("You are not authorized to enter edit mode\r\n");
            return false;
        }

        return true;
#else // SERVER
        assert(0);
        return false;
#endif
    }

    void edittoggled(bool on)
    {
        MessageSystem::send_EditModeC2S(on);
//        addmsg(SV_EDITMODE, "ri", on ? 1 : 0);
//        if(player1->state==CS_DEAD) deathstate(player1, true); Kripken
//        else if(player1->state==CS_EDITING && player1->editstate==CS_DEAD) sb.showscores(false); Kripken
        setvar("zoom", -1, true);

        #ifdef CLIENT
            enthover = -1; // Would be nice if sauer did this, but it doesn't... so without it you still hover on a nonseen edit ent
        #endif
    }

#ifdef CLIENT
    int getclientfocus()
    {
        fpsent *d = pointatplayer();
        return d ? d->clientnum : -1;
    }
#endif

    int parseplayer(const char *arg)
    {
        char *end;
        int n = strtol(arg, &end, 10);
        if(*arg && !*end) 
        {
            if(n!=player1->clientnum && !players.inrange(n)) return -1;
            return n;
        }
        // try case sensitive first
        loopi(numdynents())
        {
            fpsent *o = (fpsent *)iterdynents(i);
            if(o && !strcmp(arg, o->name)) return o->clientnum;
        }
        // nothing found, try case insensitive
        loopi(numdynents())
        {
            fpsent *o = (fpsent *)iterdynents(i);
            if(o && !strcasecmp(arg, o->name)) return o->clientnum;
        }
        return -1;
    }

    void listclients(bool local)
    {
        vector<char> buf;
        string cn;
        int numclients = 0;
        if(local)
        {
            formatstring(cn)("%d", player1->clientnum);
            buf.put(cn, strlen(cn));
            numclients++;
        }
        loopv(players) if(players[i])
        {
            formatstring(cn)("%d", players[i]->clientnum);
            if(numclients++) buf.add(' ');
            buf.put(cn, strlen(cn));
        }
        buf.add('\0');
        result(buf.getbuf());
    }

    void togglespectator(int val, const char *who)
    {
        if(!remote) return;
        int i = who[0] ? parseplayer(who) : player1->clientnum;
        if(i>=0) addmsg(SV_SPECTATOR, "rii", i, val);
    }

    // collect c2s messages conveniently
    vector<uchar> messages;
    int messagecn = -1, messagereliable = false;

    void addmsg(int type, const char *fmt, ...)
    {
        Logging::log(Logging::INFO, "Client: ADDMSG: adding a message of type %d\r\n", type);

        if(!connected) return;
        static uchar buf[MAXTRANS];
        ucharbuf p(buf, sizeof(buf));
        putint(p, type);
        int numi = 1, numf = 0, nums = 0, mcn = -1;
        bool reliable = false;
        if(fmt)
        {
            va_list args;
            va_start(args, fmt);
            while(*fmt) switch(*fmt++)
            {
                case 'r': reliable = true; break;
                case 'c':
                {
                    fpsent *d = va_arg(args, fpsent *);
                    mcn = !d || d == player1 ? -1 : d->clientnum;
                    break;
                }
                case 'v':
                {
                    int n = va_arg(args, int);
                    int *v = va_arg(args, int *);
                    loopi(n) putint(p, v[i]);
                    numi += n;
                    break;
                }

                case 'i':
                {
                    int n = isdigit(*fmt) ? *fmt++-'0' : 1;
                    loopi(n) putint(p, va_arg(args, int));
                    numi += n;
                    break;
                }
                case 'f':
                {
                    int n = isdigit(*fmt) ? *fmt++-'0' : 1;
                    loopi(n) putfloat(p, (float)va_arg(args, double));
                    numf += n;
                    break;
                }
                case 's': sendstring(va_arg(args, const char *), p); nums++; break;
            }
            va_end(args);
        }
        int num = nums || numf ? 0 : numi, msgsize = server::msgsizelookup(type);
        // Kripken: ignore message sizes for non-sauer messages, i.e., ones we added
        if (type < INTENSITY_MSG_TYPE_MIN)
        {
            if(msgsize && num!=msgsize) { defformatstring(s)("inconsistent msg size for %d (%d != %d)", type, num, msgsize); fatal(s); }
        }
        if(reliable) messagereliable = true;
        if(mcn != messagecn)
        {
            static uchar mbuf[16];
            ucharbuf m(mbuf, sizeof(mbuf));
            putint(m, SV_FROMAI);
            putint(m, mcn);
            messages.put(mbuf, m.length());
            messagecn = mcn;
        }
        messages.put(buf, p.length());
    }

    void toserver(char *text)
    {
#ifdef CLIENT
        if (ClientSystem::scenarioStarted())
#endif // XXX - Need a similar check for NPCs on the server, if/when we have them
        {
            conoutf(CON_CHAT, "%s:\f0 %s", colorname(player1), text);
            addmsg(SV_TEXT, "rcs", player1, text);
        }
    }
    COMMANDN(say, toserver, "C");

#if 0
    void sayteam(char *text)
    {
#ifdef CLIENT
        conoutf(CON_TEAMCHAT, "%s:\f1 %s", colorname(player1), text);
#endif
        addmsg(SV_SAYTEAM, "rs", text);
    }
#endif

    void printvar(fpsent *d, ident *id)
    {
        switch(id->type)
        {
            case ID_VAR:
            {
                int val = *id->storage.i;
                string str;
                if(id->flags&IDF_HEX && id->maxval==0xFFFFFF)
                    formatstring(str)("0x%.6X (%d, %d, %d)", val, (val>>16)&0xFF, (val>>8)&0xFF, val&0xFF);
                else
                    formatstring(str)(id->flags&IDF_HEX ? "0x%X" : "%d", val);
                conoutf("%s set map var \"%s\" to %s", colorname(d), id->name, str);
                break;
            }
            case ID_FVAR:
                conoutf("%s set map var \"%s\" to %s", colorname(d), id->name, floatstr(*id->storage.f));
                break;
            case ID_SVAR:
                conoutf("%s set map var \"%s\" to \"%s\"", colorname(d), id->name, *id->storage.s);
                break;
        }
    }

    void sendposition(fpsent *d)
    {
        Logging::log(Logging::INFO, "sendposition?, %d)\r\n", curtime);

//        if(d->state==CS_ALIVE || d->state==CS_EDITING) // Kripken: We handle death differently.
//        {
#ifdef CLIENT // If not logged in, or scenario not started, no need to send positions to self server (even can be buggy that way)
        if (ClientSystem::loggedIn && ClientSystem::scenarioStarted())
#else // SERVER
        if (d->uniqueId != DUMMY_SINGLETON_CLIENT_UNIQUE_ID)
#endif
        {
            Logging::log(Logging::INFO, "sendpacketclient: Sending for client %d: %f,%f,%f\r\n",
                                         d->clientnum, d->o.x, d->o.y, d->o.z);

            // send position updates separately so as to not stall out aiming
            packetbuf q(100);

            NetworkSystem::PositionUpdater::QuantizedInfo info;
            info.generateFrom(d);
            info.applyToBuffer(q);

#ifdef CLIENT
    #if (SERVER_DRIVEN_PLAYERS == 0)
            sendclientpacket(q.finalize(), 0, d->clientnum); // Disable this to stop client from updating server with position
    #endif
#else
            localclienttoserver(0, q.finalize(), d->clientnum); // Kripken: Send directly to server, we are its internal headless client
                                            // We feed the correct clientnum here, this is a new functionality of this func
#endif
        }
    }

    void sendmessages(fpsent *d)
    {
        packetbuf p(MAXTRANS);
        if(messages.length())
        {
            p.put(messages.getbuf(), messages.length());
            messages.setsizenodelete(0);
            if(messagereliable) p.reliable();
            messagereliable = false;
            messagecn = -1;
        }
        if(lastmillis-lastping>250)
        {
            putint(p, SV_PING);
            putint(p, lastmillis);
            lastping = lastmillis;
        }
        sendclientpacket(p.finalize(), 1, d->clientnum);
    }

    void c2sinfo() // send update to the server
    {
        static int lastupdate = -1000;

        Logging::log(Logging::INFO, "c2sinfo: %d,%d\r\n", totalmillis, lastupdate);

        int rate = Utility::Config::getInt("Network", "rate", 33);
        if(totalmillis - lastupdate < rate) return;    // don't update faster than the rate
        lastupdate = totalmillis;

#ifdef CLIENT
        if (ClientSystem::scenarioStarted())
            sendposition(player1);

        sendmessages(player1); // XXX - need to send messages from NPCs?
#else // SERVER
        loopv(players)
        {
            fpsent *d = players[i];
            if (d->serverControlled && d->uniqueId != DUMMY_SINGLETON_CLIENT_UNIQUE_ID)
            {
                sendposition(d);
            }
        }
#endif
        flushclient();
    }

    void updatepos(fpsent *d)
    {
#ifdef CLIENT
        // Only the client cares if other clients overlap him. NPCs on the server don't mind.
        // update the position of other clients in the game in our world
        // don't care if he's in the scenery or other players,
        // just don't overlap with our client

        const float r = player1->radius+d->radius;
        const float dx = player1->o.x-d->o.x;
        const float dy = player1->o.y-d->o.y;
        const float dz = player1->o.z-d->o.z;
        const float rz = player1->aboveeye+d->eyeheight;
        const float fx = (float)fabs(dx), fy = (float)fabs(dy), fz = (float)fabs(dz);
        if(fx<r && fy<r && fz<rz && player1->state!=CS_SPECTATOR && d->state!=CS_DEAD)
        {
            if(fx<fy) d->o.y += dy<0 ? r-fy : -(r-fy);  // push aside
            else      d->o.x += dx<0 ? r-fx : -(r-fx);
        }
#endif
        int lagtime = lastmillis-d->lastupdate;
        if(lagtime)
        {
            if(d->state!=CS_SPAWNING && d->lastupdate) d->plag = (d->plag*5+lagtime)/6;
            d->lastupdate = lastmillis;
        }

        // The client's position has been changed, not by running physics, but by info from the remote
        // client. This counts as if we ran physics, though, since next time we *DO* need to run
        // physics, we don't want to go back any more than this!
        d->lastphysframe = lastmillis;
    }

    void parsepositions(ucharbuf &p)
    {
        int type;
        while(p.remaining()) switch(type = getint(p))
        {
            case SV_POS:                        // position of another client
            {
                NetworkSystem::PositionUpdater::QuantizedInfo info;
                info.generateFrom(p);
                info.applyToEntity();

                break;
            }

            default:
                neterr("positions-type");
                return;
        }
    }

    void parsepacketclient(int chan, packetbuf &p)   // processes any updates from the server
    {
        Logging::log(Logging::INFO, "Client: Receiving packet, channel: %d\r\n", chan);

        switch(chan)
        {   // Kripken: channel 0 is just positions, for as-fast-as-possible position updates. We do not want to change this.
            //          channel 1 is used by essentially all the fps game logic events
            //          channel 2: a binary file is received, a map or a demo
            case 0: 
                parsepositions(p);
                break;

            case 1:
                parsemessages(-1, NULL, p);
                break;

            case 2:
                // kripken: TODO: For now, this should only be for players, not NPCs on the server
                assert(0);
//                receivefile(p.buf, p.maxlen);
                break;
        }
    }

    SVARP(chat_sound, "olpc/FlavioGaete/Vla_G_Major");

    void parsemessages(int cn, fpsent *d, ucharbuf &p) // cn: Sauer's sending client
    {
//        int gamemode = gamemode; Kripken
        static char text[MAXTRANS];
        int type;
//        bool mapchanged = false; Kripken

        while(p.remaining())
        {
          type = getint(p);
          Logging::log(Logging::INFO, "Client: Parsing a message of type %d\r\n", type);
          switch(type)
          { // Kripken: Mangling sauer indentation as little as possible

            case SV_CLIENT:
            {
                int cn = getint(p), len = getuint(p);
                ucharbuf q = p.subbuf(len);
                parsemessages(cn, getclient(cn), q); // Only the client needs relayed Sauer messages, not the NPCs.
                break;
            }

            case SV_TEXT:
            {
                if(!d) return;
                getstring(text, p);
                filtertext(text, text);
#ifdef CLIENT
                if(d->state!=CS_SPECTATOR)
                {
                    defformatstring(ds)("@%s", &text);
                    particle_text(d->abovehead(), ds, PART_TEXT, 2000, 0x32FF64, 4.0f, -8);
                }
                if (chat_sound[0])
                    playsoundname(chat_sound);
#endif
                conoutf(CON_CHAT, "%s:\f0 %s", colorname(d), text);
                break;
            }


            case SV_EDITF:              // coop editing messages
            case SV_EDITT:
            case SV_EDITM:
            case SV_FLIP:
            case SV_COPY:
            case SV_PASTE:
            case SV_ROTATE:
            case SV_REPLACE:
            case SV_DELCUBE:
            {
//                if(!d) return; Kripken: We can get edit commands from the server, which has no 'd' to speak of XXX FIXME - might be buggy

                Logging::log(Logging::DEBUG, "Edit command intercepted in client.h\r\n");

                selinfo sel;
                sel.o.x = getint(p); sel.o.y = getint(p); sel.o.z = getint(p);
                sel.s.x = getint(p); sel.s.y = getint(p); sel.s.z = getint(p);
                sel.grid = getint(p); sel.orient = getint(p);
                sel.cx = getint(p); sel.cxs = getint(p); sel.cy = getint(p), sel.cys = getint(p); // Why "," here and not all ;?
                sel.corner = getint(p);
                int dir, mode, mat, filter;
                #ifdef CLIENT
                    int tex, newtex, allfaces;
                #endif
                ivec moveo;
                switch(type)
                {
                    case SV_EDITF: dir = getint(p); mode = getint(p); mpeditface(dir, mode, sel, false); break;
                    case SV_EDITT:
                        #ifdef CLIENT
                            tex = getint(p); allfaces = getint(p); mpedittex(tex, allfaces, sel, false); break;
                        #else // SERVER
                            getint(p); getint(p); Logging::log(Logging::DEBUG, "Server ignoring texture change (a)\r\n"); break;
                        #endif
                    case SV_EDITM: mat = getint(p); filter = getint(p); mpeditmat(mat, filter, sel, false); break;
                    case SV_FLIP: mpflip(sel, false); break;
                    case SV_COPY: if(d) mpcopy(d->edit, sel, false); break;
                    case SV_PASTE: if(d) mppaste(d->edit, sel, false); break;
                    case SV_ROTATE: dir = getint(p); mprotate(dir, sel, false); break;
                    case SV_REPLACE:
                        #ifdef CLIENT
                            tex = getint(p); newtex = getint(p); mpreplacetex(tex, newtex, sel, false); break;
                        #else // SERVER
                            getint(p); getint(p); Logging::log(Logging::DEBUG, "Server ignoring texture change (b)\r\n"); break;
                        #endif
                    case SV_DELCUBE: mpdelcube(sel, false); break;
                }
                break;
            }
            case SV_REMIP:
            {
              #ifdef CLIENT
                if(!d) return;
                conoutf("%s remipped", colorname(d));
                mpremip(false);
              #endif

                break;
            }

            case SV_EDITVAR:
            {
                assert(0);
                break;
            }

            case SV_PONG:
#ifdef SERVER
assert(0);
#endif
                // Kripken: Do not let clients know other clients' pings
                player1->ping = (player1->ping*5+lastmillis-getint(p))/6;
//                addmsg(SV_CLIENTPING, "i", player1->ping = (player1->ping*5+lastmillis-getint(p))/6);
                break;

            case SV_CLIENTPING:
//#ifdef SERVER
assert(0); // Kripken: Do not let clients know other clients' pings
//#endif
                if(!d) return;
                d->ping = getint(p);
                break;


            default:
            {
                Logging::log(Logging::INFO, "Client: Handling a non-typical message: %d\r\n", type);
#ifdef CLIENT
                if (!MessageSystem::MessageManager::receive(type, ClientSystem::playerNumber, cn, p))
#else
                if (!MessageSystem::MessageManager::receive(type, 0, cn, p)) // Server's internal client is num '0'
#endif
                {
                    assert(0);
                    neterr("messages-type-client");
                    printf("Quitting\r\n");
                    return;
                }
                break;
            }
          }
        }
    }

    void changemap(const char *name, int mode)        // forced map change from the server // Kripken : TODO: Deprecated, Remove
    {
        Logging::log(Logging::INFO, "Client: Changing map: %s\r\n", name);

        mode = 0;
        gamemode = mode;
#ifdef CLIENT
        if(editmode) toggleedit();
#endif
        if((gamemode==1 && !name[0]) || (!load_world(name) && remote)) 
        {
            emptymap(0, true, name);
        }
    }

    void changemap(const char *name) // request map change, server may ignore
    {
        Logging::log(Logging::INFO, "Client: Requesting map: %s\r\n", name);

        if(spectator && !player1->privilege) return;
//        int nextmode = nextmode; // in case stopdemo clobbers nextmode
//        addmsg(SV_MAPVOTE, "rsi", name, nextmode);
#ifdef CLIENT
        MessageSystem::send_MapVote(name);
#endif
    }
        
    void gotoplayer(const char *arg)
    {
        if(player1->state!=CS_SPECTATOR && player1->state!=CS_EDITING) return;
        int i = parseplayer(arg);
        if(i>=0 && i!=player1->clientnum) 
        {
            fpsent *d = getclient(i);
            if(!d) return;
            player1->o = d->o;
            vec dir;
            vecfromyawpitch(player1->yaw, player1->pitch, 1, 0, dir);
            player1->o.add(dir.mul(-32));
            player1->resetinterp();
        }
    }

    void connectattempt(const char *name, const char *password, const ENetAddress &address)
    {
        Logging::log(Logging::DEBUG, "Connect attempt\r\n");
    }

    void connectfail()
    {
    }

    void adddynlights()
    {
        #ifdef CLIENT
            LightControl::addHoverLight();
            LightControl::showQueuedDynamicLights();
        #endif
    }

    void edittrigger(const selinfo &sel, int op, int arg1, int arg2, int arg3)
    {
#ifdef CLIENT
        if(!ClientSystem::isAdmin())
        {
            Logging::log(Logging::WARNING, "vartrigger invalid\r\n");
            return;
        }
#endif

        switch(op)
        {
            case EDIT_FLIP:
            case EDIT_COPY:
            case EDIT_PASTE:
            case EDIT_DELCUBE:
            {
                addmsg(SV_EDITF + op, "ri9i4",
                   sel.o.x, sel.o.y, sel.o.z, sel.s.x, sel.s.y, sel.s.z, sel.grid, sel.orient,
                   sel.cx, sel.cxs, sel.cy, sel.cys, sel.corner);
                break;
            }
            case EDIT_ROTATE:
            {
                addmsg(SV_EDITF + op, "ri9i5",
                   sel.o.x, sel.o.y, sel.o.z, sel.s.x, sel.s.y, sel.s.z, sel.grid, sel.orient,
                   sel.cx, sel.cxs, sel.cy, sel.cys, sel.corner,
                   arg1);
                break;
            }
            case EDIT_MAT:
            case EDIT_FACE:
            case EDIT_TEX:
            case EDIT_REPLACE:
            {
                addmsg(SV_EDITF + op, "ri9i6",
                   sel.o.x, sel.o.y, sel.o.z, sel.s.x, sel.s.y, sel.s.z, sel.grid, sel.orient,
                   sel.cx, sel.cxs, sel.cy, sel.cys, sel.corner,
                   arg1, arg2);
                break;
            }
            case EDIT_REMIP:
            {
                addmsg(SV_EDITF + op, "r");
                break;
            }
        }

        // Note that we made changes
        EditingSystem::madeChanges = true;
    }

    void vartrigger(ident *id)
    {
        // We do not use sauer protocol to update mapvars. Use our method to run a script
        // to make each client update its map vars, or upload/restart the map.
    }

    ICOMMAND(getmode, "", (), intret(0)); // Prevent warnings

    void forceedit(const char *name) { };
};

