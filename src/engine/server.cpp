// server.cpp: little more than enhanced multicaster
// runs dedicated or as client coroutine

#include "engine.h"

#include "game.h" // INTENSITY: needed for fpsent
 // INTENSITY
#include "network_system.h"
#include "server_system.h"
#include "fpsclient_interface.h"
#include "utility.h"
#include "system_manager.h"
#include "script_engine_manager.h"


#ifdef STANDALONE
void fatal(const char *s, ...) 
{ 
    void cleanupserver();
    cleanupserver(); 
    defvformatstring(msg,s,s);
    printf("servererror: %s\n", msg); 
    exit(EXIT_FAILURE); 
}

void conoutfv(int type, const char *fmt, va_list args)
{
    string sf, sp;
    vformatstring(sf, fmt, args);
    filtertext(sp, sf);
    puts(sp);
}

void conoutf(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    conoutfv(CON_INFO, fmt, args);
    va_end(args);
}

void conoutf(int type, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    conoutfv(type, fmt, args);
    va_end(args);
}
#endif

// all network traffic is in 32bit ints, which are then compressed using the following simple scheme (assumes that most values are small).

template<class T>
static inline void putint_(T &p, int n)
{
    if(n<128 && n>-127) p.put(n);
    else if(n<0x8000 && n>=-0x8000) { p.put(0x80); p.put(n); p.put(n>>8); }
    else { p.put(0x81); p.put(n); p.put(n>>8); p.put(n>>16); p.put(n>>24); }
}
void putint(ucharbuf &p, int n) { putint_(p, n); }
void putint(packetbuf &p, int n) { putint_(p, n); }

int getint(ucharbuf &p)
{
    int c = (char)p.get();
    if(c==-128) { int n = p.get(); n |= char(p.get())<<8; return n; }
    else if(c==-127) { int n = p.get(); n |= p.get()<<8; n |= p.get()<<16; return n|(p.get()<<24); } 
    else return c;
}

// much smaller encoding for unsigned integers up to 28 bits, but can handle signed
template<class T>
static inline void putuint_(T &p, int n)
{
    if(n < 0 || n >= (1<<21))
    {
        p.put(0x80 | (n & 0x7F));
        p.put(0x80 | ((n >> 7) & 0x7F));
        p.put(0x80 | ((n >> 14) & 0x7F));
        p.put(n >> 21);
    }
    else if(n < (1<<7)) p.put(n);
    else if(n < (1<<14))
    {
        p.put(0x80 | (n & 0x7F));
        p.put(n >> 7);
    }
    else 
    { 
        p.put(0x80 | (n & 0x7F)); 
        p.put(0x80 | ((n >> 7) & 0x7F));
        p.put(n >> 14); 
    }
}
void putuint(ucharbuf &p, int n) { putuint_(p, n); }
void putuint(packetbuf &p, int n) { putuint_(p, n); }

int getuint(ucharbuf &p)
{
    int n = p.get();
    if(n & 0x80)
    {
        n += (p.get() << 7) - 0x80;
        if(n & (1<<14)) n += (p.get() << 14) - (1<<14);
        if(n & (1<<21)) n += (p.get() << 21) - (1<<21);
        if(n & (1<<28)) n |= 0xF0000000; 
    }
    return n;
}

template<class T>
static inline void putfloat_(T &p, float f)
{
    lilswap(&f, 1);
    p.put((uchar *)&f, sizeof(float));
}
void putfloat(ucharbuf &p, float f) { putfloat_(p, f); }
void putfloat(packetbuf &p, float f) { putfloat_(p, f); }

float getfloat(ucharbuf &p)
{
    float f;
    p.get((uchar *)&f, sizeof(float));
    return lilswap(f);
}

template<class T>
static inline void sendstring_(const char *t, T &p)
{
    while(*t) putint(p, *t++);
    putint(p, 0);
}
void sendstring(const char *t, ucharbuf &p) { sendstring_(t, p); }
void sendstring(const char *t, packetbuf &p) { sendstring_(t, p); }

void getstring(char *text, ucharbuf &p, int len)
{
    char *t = text;
    do
    {
        if(t>=&text[len]) { text[len-1] = 0; return; }
        if(!p.remaining()) { *t = 0; return; } 
        *t = getint(p);
    }
    while(*t++);
}

void filtertext(char *dst, const char *src, bool whitespace, int len)
{
    for(int c = *src; c; c = *++src)
    {
        switch(c)
        {
        case '\f': ++src; continue;
        }
        if(isspace(c) ? whitespace : isprint(c))
        {
            *dst++ = c;
            if(!--len) break;
        }
    }
    *dst = '\0';
}

enum { ST_EMPTY, ST_LOCAL, ST_TCPIP };

struct client                   // server side version of "dynent" type
{
    int type;
    int num;
    ENetPeer *peer;
    string hostname;
    void *info;
};

vector<client *> clients;

ENetHost *serverhost = NULL;
size_t bsend = 0, brec = 0;
int laststatus = 0; 
ENetSocket pongsock = ENET_SOCKET_NULL, lansock = ENET_SOCKET_NULL;

void cleanupserver()
{
    if(serverhost) enet_host_destroy(serverhost);
    serverhost = NULL;

    if(pongsock != ENET_SOCKET_NULL) enet_socket_destroy(pongsock);
    if(lansock != ENET_SOCKET_NULL) enet_socket_destroy(lansock);
    pongsock = lansock = ENET_SOCKET_NULL;
}

void process(ENetPacket *packet, int sender, int chan);
//void disconnect_client(int n, int reason);

void *getclientinfo(int i) { return !clients.inrange(i) || clients[i]->type==ST_EMPTY ? NULL : clients[i]->info; }
int getnumclients()        { return clients.length(); }
uint getclientip(int n)    { return clients.inrange(n) && clients[n]->type==ST_TCPIP ? clients[n]->peer->address.host : 0; }

void sendpacket(int n, int chan, ENetPacket *packet, int exclude)
{
    if(n<0)
    {
        if (getnumclients() <= 0) return; // INTENSITY: Added this, because otherwise sending to '-1' when there are no clients segfaults
        server::recordpacket(chan, packet->data, packet->dataLength);
        loopv(clients) if(i!=exclude && server::allowbroadcast(i)) sendpacket(i, chan, packet);
        return;
    }
    switch(clients[n]->type)
    {
        case ST_TCPIP:
        {
            enet_peer_send(clients[n]->peer, chan, packet);
            bsend += packet->dataLength;

            NetworkSystem::Cataloger::packetSent(chan, packet->dataLength); // INTENSITY

            break;
        }

#ifndef STANDALONE
        case ST_LOCAL:
            localservertoclient(chan, packet);
            break;
#endif
    }
}

void sendf(int cn, int chan, const char *format, ...)
{
    int exclude = -1;
    bool reliable = false;
    if(*format=='r') { reliable = true; ++format; }
    ENetPacket *packet = enet_packet_create(NULL, MAXTRANS, reliable ? ENET_PACKET_FLAG_RELIABLE : 0);
    ucharbuf p(packet->data, packet->dataLength);
    va_list args;
    va_start(args, format);
    while(*format) switch(*format++)
    {
        case 'x':
            exclude = va_arg(args, int);
            break;

        case 'v':
        {
            int n = va_arg(args, int);
            int *v = va_arg(args, int *);
            loopi(n) putint(p, v[i]);
            break;
        }

        case 'i': 
        {
            int n = isdigit(*format) ? *format++-'0' : 1;
            loopi(n) putint(p, va_arg(args, int));
            break;
        }
        case 'f':
        {
            int n = isdigit(*format) ? *format++-'0' : 1;
            loopi(n) putfloat(p, (float)va_arg(args, double));
            break;
        }
        case 's': sendstring(va_arg(args, const char *), p); break;
        case 'm':
        {
            int n = va_arg(args, int);
            enet_packet_resize(packet, packet->dataLength+n);
            p.buf = packet->data;
            p.maxlen += n;
            p.put(va_arg(args, uchar *), n);
            break;
        }
    }
    va_end(args);
    enet_packet_resize(packet, p.length());
    sendpacket(cn, chan, packet, exclude);
    if(packet->referenceCount==0) enet_packet_destroy(packet);
}

void sendfile(int cn, int chan, stream *file, const char *format, ...)
{
    assert(0); // INTENSITY: We use our own asset system to transfer files
#if 0
    if(cn < 0)
    {
#ifdef STANDALONE
        return;
#endif
    }
    else if(!clients.inrange(cn)) return;

    int len = file->size();
    if(len <= 0) return;

    bool reliable = false;
    if(*format=='r') { reliable = true; ++format; }
    ENetPacket *packet = enet_packet_create(NULL, MAXTRANS+len, ENET_PACKET_FLAG_RELIABLE);

    ucharbuf p(packet->data, packet->dataLength);
    va_list args;
    va_start(args, format);
    while(*format) switch(*format++)
    {
        case 'i':
        {
            int n = isdigit(*format) ? *format++-'0' : 1;
            loopi(n) putint(p, va_arg(args, int));
            break;
        }
        case 's': sendstring(va_arg(args, const char *), p); break;
        case 'l': putint(p, len); break;
    }
    va_end(args);
    enet_packet_resize(packet, p.length()+len);

    file->seek(0, SEEK_SET);
    file->read(&packet->data[p.length()], len);
    enet_packet_resize(packet, p.length()+len);

    if(cn >= 0) sendpacket(cn, chan, packet, -1);
#ifndef STANDALONE
    else sendclientpacket(packet, chan);
#endif
    if(!packet->referenceCount) enet_packet_destroy(packet);
#endif
}

const char *disc_reasons[] = { "normal", "end of packet", "client num", "kicked/banned", "tag type", "ip is banned", "server is in private mode", "server FULL (maxclients)", "connection timed out" };

void disconnect_client(int n, int reason)
{
    if(clients[n]->type!=ST_TCPIP) return;
    enet_peer_disconnect(clients[n]->peer, reason);
    server::clientdisconnect(n);
    clients[n]->type = ST_EMPTY;
    clients[n]->peer->data = NULL;
    server::deleteclientinfo(clients[n]->info);
    clients[n]->info = NULL;
    defformatstring(s)("client (%s) disconnected because: %s", clients[n]->hostname, disc_reasons[reason]);
    puts(s);
    server::sendservmsg(s);
}

void kicknonlocalclients(int reason)
{
    loopv(clients) if(clients[i]->type==ST_TCPIP) disconnect_client(i, reason);
}

void process(ENetPacket *packet, int sender, int chan)   // sender may be -1
{
    packetbuf p(packet);
    server::parsepacket(sender, chan, p);
    if(p.overread()) { disconnect_client(sender, DISC_EOP); return; }
}

void localclienttoserver(int chan, ENetPacket *packet, int cn) // INTENSITY: Added cn
{
    client *c = NULL;
    if (cn == -1) // INTENSITY
    {
        loopv(clients) if(clients[i]->type==ST_LOCAL) { c = clients[i]; break; }
    } else {
        c = clients[cn]; // INTENSITY
    }

    if(c) process(packet, c->num, chan);
}

client &addclient()
{
    // INTENSITY: Also check that getclient returns false. Otherwise in rare cases
    // we may think a clientnum is free, because at this level it is, but the
    // game::clients is not yet updated - perhaps because the game is being restarted,
    // or assets downloaded, etc. And in any case, there is no downside to doing
    // this check
    loopv(clients) if(clients[i]->type==ST_EMPTY && !game::getclient(clients[i]->num))
    {
        clients[i]->info = server::newclientinfo();
        return *clients[i];
    }
    client *c = new client;
    c->num = clients.length();
    c->info = server::newclientinfo();
    clients.add(c);
    return *c;
}

int localclients = 0, nonlocalclients = 0;

bool hasnonlocalclients() { return nonlocalclients!=0; }
bool haslocalclients() { return localclients!=0; }

#ifdef STANDALONE
bool resolverwait(const char *name, ENetAddress *address)
{
    return enet_address_set_host(address, name) >= 0;
}

int connectwithtimeout(ENetSocket sock, const char *hostname, const ENetAddress &remoteaddress)
{
    int result = enet_socket_connect(sock, &remoteaddress);
    if(result<0) enet_socket_destroy(sock);
    return result;
}
#endif

ENetSocket mastersock = ENET_SOCKET_NULL;
ENetAddress masteraddress = { ENET_HOST_ANY, ENET_PORT_ANY }, serveraddress = { ENET_HOST_ANY, ENET_PORT_ANY };
int lastupdatemaster = 0;
vector<char> masterout, masterin;
int masteroutpos = 0, masterinpos = 0;
VARN(updatemaster, allowupdatemaster, 0, 1, 1);
SVAR(mastername, server::defaultmaster());

void disconnectmaster()
{
    if(mastersock == ENET_SOCKET_NULL) return;

    enet_socket_destroy(mastersock);
    mastersock = ENET_SOCKET_NULL;

    masterout.setsizenodelete(0);
    masterin.setsizenodelete(0);
    masteroutpos = masterinpos = 0;
}

ENetSocket connectmaster()
{
    if(!mastername[0]) return ENET_SOCKET_NULL;

    if(masteraddress.host == ENET_HOST_ANY)
    {
#ifdef STANDALONE
        printf("looking up %s...\n", mastername);
#endif
        masteraddress.port = server::masterport();
        if(!resolverwait(mastername, &masteraddress)) return ENET_SOCKET_NULL;
    }
    ENetSocket sock = enet_socket_create(ENET_SOCKET_TYPE_STREAM);
    if(sock != ENET_SOCKET_NULL && serveraddress.host != ENET_HOST_ANY && enet_socket_bind(sock, &serveraddress) < 0)
    {
        enet_socket_destroy(sock);
        sock = ENET_SOCKET_NULL;
    }
    if(sock == ENET_SOCKET_NULL || connectwithtimeout(sock, mastername, masteraddress) < 0) 
    {
#ifdef STANDALONE
        printf(sock==ENET_SOCKET_NULL ? "could not open socket\n" : "could not connect\n"); 
#endif
        return ENET_SOCKET_NULL;
    }
    
    enet_socket_set_option(sock, ENET_SOCKOPT_NONBLOCK, 1);
    return sock;
}

bool requestmaster(const char *req)
{
    if(mastersock == ENET_SOCKET_NULL)
    {
        mastersock = connectmaster();
        if(mastersock == ENET_SOCKET_NULL) return false;
    }

    masterout.put(req, strlen(req));
    return true;
}

bool requestmasterf(const char *fmt, ...)
{
    defvformatstring(req, fmt, fmt);
    return requestmaster(req);
}

void processmasterinput()
{
    if(masterinpos >= masterin.length()) return;

    char *input = &masterin[masterinpos], *end = (char *)memchr(input, '\n', masterin.length() - masterinpos);
    while(end)
    {
        *end++ = '\0';

        const char *args = input;
        while(args < end && !isspace(*args)) args++;
        int cmdlen = args - input;
        while(args < end && isspace(*args)) args++;

        if(!strncmp(input, "failreg", cmdlen))
            conoutf(CON_ERROR, "master server registration failed: %s", args);
        else if(!strncmp(input, "succreg", cmdlen))
            conoutf("master server registration succeeded");
        else server::processmasterinput(input, cmdlen, args);

        masterinpos = end - masterin.getbuf();
        input = end;
        end = (char *)memchr(input, '\n', masterin.length() - masterinpos);
    } 

    if(masterinpos >= masterin.length())
    {
        masterin.setsizenodelete(0);
        masterinpos = 0;
    }
}

void flushmasteroutput()
{
    if(masterout.empty()) return;

    ENetBuffer buf;
    buf.data = &masterout[masteroutpos];
    buf.dataLength = masterout.length() - masteroutpos;
    int sent = enet_socket_send(mastersock, NULL, &buf, 1);
    if(sent >= 0)
    {
        masteroutpos += sent;
        if(masteroutpos >= masterout.length())
        {
            masterout.setsizenodelete(0);
            masteroutpos = 0;
        }
    }
    else disconnectmaster();
}

void flushmasterinput()
{
    if(masterin.length() >= masterin.capacity())
        masterin.reserve(4096);

    ENetBuffer buf;
    buf.data = &masterin[masterin.length()];
    buf.dataLength = masterin.capacity() - masterin.length();
    int recv = enet_socket_receive(mastersock, NULL, &buf, 1);
    if(recv > 0)
    {
        masterin.advance(recv);
        processmasterinput();
    }
    else disconnectmaster();
}

static ENetAddress pongaddr;

void sendserverinforeply(ucharbuf &p)
{
    ENetBuffer buf;
    buf.data = p.buf;
    buf.dataLength = p.length();
    enet_socket_send(pongsock, &pongaddr, &buf, 1);
}

void checkserversockets()        // reply all server info requests
{
#if 0 // INTENSITY: No need for pong sock
    static ENetSocketSet sockset;
    ENET_SOCKETSET_EMPTY(sockset);
    ENetSocket maxsock = pongsock;
    ENET_SOCKETSET_ADD(sockset, pongsock);
    if(mastersock != ENET_SOCKET_NULL)
    {
        maxsock = max(maxsock, mastersock);
        ENET_SOCKETSET_ADD(sockset, mastersock);
    }
    if(lansock != ENET_SOCKET_NULL)
    {
        maxsock = max(maxsock, lansock);
        ENET_SOCKETSET_ADD(sockset, lansock);
    }
    if(enet_socketset_select(maxsock, &sockset, NULL, 0) <= 0) return;

    ENetBuffer buf;
    uchar pong[MAXTRANS];
    loopi(2)
    {
        ENetSocket sock = i ? lansock : pongsock;
        if(sock == ENET_SOCKET_NULL || !ENET_SOCKETSET_CHECK(sockset, sock)) continue;

        buf.data = pong;
        buf.dataLength = sizeof(pong);
        int len = enet_socket_receive(sock, &pongaddr, &buf, 1);
        if(len < 0) return;
        ucharbuf req(pong, len), p(pong, sizeof(pong));
        p.len += len;
        server::serverinforeply(req, p);
    }

    if(mastersock != ENET_SOCKET_NULL && ENET_SOCKETSET_CHECK(sockset, mastersock)) flushmasterinput();
#endif
}


VAR(serveruprate, 0, 0, INT_MAX);
SVAR(serverip, "");
VARF(serverport, 0, server::serverport(), 0xFFFF, { if(!serverport) serverport = server::serverport(); });

#ifdef STANDALONE
int curtime = 0, lastmillis = 0, totalmillis = 0;
#endif

void updatemasterserver()
{
// INTENSITY
}

#define DEFAULTCLIENTS 6

int uprate = 0, maxclients = DEFAULTCLIENTS;
const char *ip = "", *master = NULL;

#if defined(STANDALONE) || defined(SERVER) // INTENSITY: Added server
int curtime = 0, lastmillis = 0, totalmillis = 0;
#endif

// INTENSITY: Moved this code to here, + additions, so it can all be called from Python
void show_server_stats()
{
    float seconds = float(totalmillis-laststatus)/1024.0f;

    if(seconds > 0 && (nonlocalclients || bsend || brec))
    {
        printf("%d remote clients, %.1f K/sec sent, %.1f K/sec received   [over last %.1f seconds]\n", nonlocalclients, bsend/seconds/1024, brec/seconds/1024, seconds);

        NetworkSystem::Cataloger::show(seconds);
    }
    else
        printf("No activity to report\r\n");

    // Initialise
    laststatus = totalmillis;
    bsend = brec = 0;
}

void serverslice(bool dedicated, uint timeout)   // main server update, called from main loop in sp, or from below in dedicated server
{
    localclients = nonlocalclients = 0;
    loopv(clients) switch(clients[i]->type)
    {
        case ST_LOCAL: localclients++; break;
        case ST_TCPIP: nonlocalclients++; break;
    }

    if(!serverhost) 
    {
        server::serverupdate();
        server::sendpackets();
        return;
    }
       
    // below is network only

    if(dedicated) 
    {
        int millis = (int)enet_time_get();
        curtime = millis - totalmillis;
        lastmillis = totalmillis = millis;
    }
    server::serverupdate();

    flushmasteroutput();
    checkserversockets();

    if(totalmillis-lastupdatemaster>60*60*1000)       // send alive signal to masterserver every hour of uptime
    {
        updatemasterserver();
        lastupdatemaster = totalmillis;
    }
    
#if 0 // INTENSITY: Moved above to show_server_stats
    if(totalmillis-laststatus>60*1000)   // display bandwidth stats, useful for server ops
    {
        laststatus = totalmillis;     
        if(nonlocalclients || bsend || brec) printf("status: %d remote clients, %.1f send, %.1f rec (K/sec)\n", nonlocalclients, bsend/60.0f/1024, brec/60.0f/1024);
        bsend = brec = 0;
    }
#endif

    ENetEvent event;
    bool serviced = false;
    while(!serviced)
    {
        if(enet_host_check_events(serverhost, &event) <= 0)
        {
            if(enet_host_service(serverhost, &event, timeout) <= 0) break;
            serviced = true;
        }
        switch(event.type)
        {
            case ENET_EVENT_TYPE_CONNECT:
            {
                client &c = addclient();
                c.type = ST_TCPIP;
                c.peer = event.peer;
                c.peer->data = &c;
                char hn[1024];
                copystring(c.hostname, (enet_address_get_host_ip(&c.peer->address, hn, sizeof(hn))==0) ? hn : "unknown");
                printf("client connected (%s)\n", c.hostname);
                int reason = server::clientconnect(c.num, c.peer->address.host);
                if(!reason) nonlocalclients++;
                else disconnect_client(c.num, reason);
                break;
            }
            case ENET_EVENT_TYPE_RECEIVE:
            {
                brec += event.packet->dataLength;
                client *c = (client *)event.peer->data;
                if(c) process(event.packet, c->num, event.channelID);
                if(event.packet->referenceCount==0) enet_packet_destroy(event.packet);
                break;
            }
            case ENET_EVENT_TYPE_DISCONNECT: 
            {
                client *c = (client *)event.peer->data;
                if(!c) break;
                printf("disconnected client (%s)\n", c->hostname);
                server::clientdisconnect(c->num);
                nonlocalclients--;
                c->type = ST_EMPTY;
                event.peer->data = NULL;
                server::deleteclientinfo(c->info);
                c->info = NULL;
                break;
            }
            default:
                break;
        }
    }
    if(server::sendpackets()) enet_host_flush(serverhost);
}

// INTENSITY: Added this, so we can flush out messages at will, e.g., login failure messages,
// which must be done before the next cycle as it might disconnect the other client before
// sending anything.
void force_network_flush()
{
    if (!serverhost)
    {
        Logging::log(Logging::ERROR, "Trying to force_flush, but no serverhost yet\r\n");
        return;
    }

//    if(sv->sendpackets())
        enet_host_flush(serverhost);
}

#ifndef STANDALONE
void localdisconnect(bool cleanup, int cn) // INTENSITY: Added cn
{
    bool disconnected = false;
    loopv(clients) if(clients[i]->type==ST_LOCAL) 
    {
        if (cn != -1 && cn != clients[i]->num) continue; // INTENSITY: if cn given, only process that one
        server::localdisconnect(i);
        localclients--;
        clients[i]->type = ST_EMPTY;
        server::deleteclientinfo(clients[i]->info);
        clients[i]->info = NULL;
        disconnected = true;
    }

#ifdef CLIENT // INTENSITY: Added this
    if(!disconnected) return;
    game::gamedisconnect(cleanup);
    mainmenu = 1;
#endif
}

int localconnect() // INTENSITY: Added returning client num
{
    client &c = addclient();
    c.type = ST_LOCAL;
    copystring(c.hostname, "local");
    localclients++;
    game::gameconnect(false);
    server::localconnect(c.num);
    return c.num; // INTENSITY: Added returning client num
}
#endif

void rundedicatedserver()
{
    #ifdef WIN32
    SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
    #endif
#if 0 // INTENSITY: We do the main loop in Python
    printf("dedicated server started, waiting for clients...\nCtrl-C to exit\n\n");
    for(;;) serverslice(true, 5);
#endif
}

bool servererror(bool dedicated, const char *desc)
{
#ifndef STANDALONE
    if(!dedicated)
    {
        conoutf(CON_ERROR, desc);
        cleanupserver();
    }
    else
#endif
        fatal(desc);
    return false;
}
  
bool setuplistenserver(bool dedicated)
{
    ENetAddress address = { ENET_HOST_ANY, serverport <= 0 ? server::serverport() : serverport };
    if(*serverip)
    {
        if(enet_address_set_host(&address, serverip)<0) conoutf(CON_WARN, "WARNING: server ip not resolved");
        else serveraddress.host = address.host;
    }
    serverhost = enet_host_create(&address, min(maxclients + server::reserveclients(), MAXCLIENTS), 0, serveruprate);
    if(!serverhost) return servererror(dedicated, "could not create server host");
    loopi(maxclients) serverhost->peers[i].data = NULL;
    address.port = server::serverinfoport(serverport > 0 ? serverport : -1);
#if 0 // INTENSITY: no need for pongsock
    pongsock = enet_socket_create(ENET_SOCKET_TYPE_DATAGRAM);
    if(pongsock != ENET_SOCKET_NULL && enet_socket_bind(pongsock, &address) < 0)
    {
        enet_socket_destroy(pongsock);
        pongsock = ENET_SOCKET_NULL;
    }
    if(pongsock == ENET_SOCKET_NULL) return servererror(dedicated, "could not create server info socket");
    else enet_socket_set_option(pongsock, ENET_SOCKOPT_NONBLOCK, 1);
#endif
#if 0 // INTENSITY: No need for LAN
    address.port = server::laninfoport();
    lansock = enet_socket_create(ENET_SOCKET_TYPE_DATAGRAM);
    if(lansock != ENET_SOCKET_NULL && (enet_socket_set_option(lansock, ENET_SOCKOPT_REUSEADDR, 1) < 0 || enet_socket_bind(lansock, &address) < 0))
    {
        enet_socket_destroy(lansock);
        lansock = ENET_SOCKET_NULL;
    }
    if(lansock == ENET_SOCKET_NULL) conoutf(CON_WARN, "WARNING: could not create LAN server info socket");
    else enet_socket_set_option(lansock, ENET_SOCKOPT_NONBLOCK, 1);
#endif
    return true;
}

void initserver(bool listen, bool dedicated)
{
    if(dedicated) execfile("server-init.cfg", false);

    if(listen) setuplistenserver(dedicated);

    server::serverinit();

    if(listen)
    {
        updatemasterserver();
        if(dedicated) rundedicatedserver(); // never returns
#ifndef STANDALONE
        else conoutf("listen server started");
#endif
    }
}

#ifndef STANDALONE
void startlistenserver(int *usemaster)
{
    if(serverhost) { conoutf(CON_ERROR, "listen server is already running"); return; }

    allowupdatemaster = *usemaster>0 ? 1 : 0;
 
    if(!setuplistenserver(false)) return;
    
    updatemasterserver();
   
    conoutf("listen server started for %d clients%s", maxclients, allowupdatemaster ? " and listed with master server" : ""); 
}
COMMAND(startlistenserver, "i");

void stoplistenserver()
{
    if(!serverhost) { conoutf(CON_ERROR, "listen server is not running"); return; }

    kicknonlocalclients();
    enet_host_flush(serverhost);
    cleanupserver();

    conoutf("listen server stopped");
}
COMMAND(stoplistenserver, "");
#endif

bool serveroption(char *opt)
{
    switch(opt[1])
    {
        case 'u': setvar("serveruprate", atoi(opt+2)); return true;
        case 'c': setvar("maxclients", atoi(opt+2)); return true;
        case 'i': setsvar("serverip", opt+2); return true;
        case 'j': setvar("serverport", atoi(opt+2)); return true; 
        case 'm': setsvar("mastername", opt+2); setvar("updatemaster", mastername[0] ? 1 : 0); return true;
#ifdef STANDALONE
        case 'q': printf("Using home directory: %s\n", opt+2); sethomedir(opt+2); return true;
        case 'k': printf("Adding package directory: %s\n", opt+2); addpackagedir(opt+2); return true;
#endif
        default: return false;
    }
}

vector<const char *> gameargs;

#if 0 // INTENSITY: We use a version of this, below, instead
#ifdef STANDALONE
int main(int argc, char* argv[])
{   
    if(enet_initialize()<0) fatal("Unable to initialise network module");
    atexit(enet_deinitialize);
    enet_time_set(0);
    for(int i = 1; i<argc; i++) if(argv[i][0]!='-' || !serveroption(argv[i])) gameargs.add(argv[i]);
    game::parseoptions(gameargs);
    initserver(true, true);
    return 0;
}
#endif
#else
#ifdef SERVER
void server_init()//int argc, char* argv[])
{
    if(enet_initialize()<0) fatal("Unable to initialise network module");
    atexit(enet_deinitialize);
    enet_time_set(0);

    SystemManager::init();

    // Init server
    initserver(true, true);

    // Generate 'dummy' singleton client. This is to whom we send position updates on the server so our internal
    // fpsclient is updates.

    localconnect();
    assert(clients.length() == 1); // Ensure noone else connected before

    fpsent* fpsEntity = dynamic_cast<fpsent*>( FPSClientInterface::newClient(0) ); // Create a new fpsclient for this client

    fpsEntity->serverControlled = true; // Mark this as not controlled by server, so we don't try to actually do anything with it
                                        // After all it doesn't really exist

    fpsEntity->uniqueId = DUMMY_SINGLETON_CLIENT_UNIQUE_ID;
    FPSServerInterface::getUniqueId(0) = DUMMY_SINGLETON_CLIENT_UNIQUE_ID;
}
#endif // SERVER
#endif // 0

#ifdef SERVER
// INTENSITY: Added this, the main slicing routine
void server_runslice()
{
    serverslice(true, 5);

    // Kripken: Simulate the curtime parameter in Sauer.
    double now = GET_PYTHON<double>("time.time()");
    static double total_time = 0;
    if (!total_time) // This prevents garbage on the very first call, since if total_time=0, then 1000*now overflows
        total_time = now;
    curtime = (long)(1000* (now - total_time) );
    total_time = now;

    if(lastmillis) game::updateworld();
}
#endif

