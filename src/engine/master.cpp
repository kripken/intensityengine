#include "cube.h"
#include <signal.h>
#include <enet/time.h>

#define INPUT_LIMIT 4096
#define OUTPUT_LIMIT (64*1024)
#define CLIENT_TIME (3*60*1000)
#define AUTH_TIME (60*1000)
#define AUTH_LIMIT 100
#define CLIENT_LIMIT 8192
#define DUP_LIMIT 16
#define PING_TIME 3000
#define PING_RETRY 5
#define KEEPALIVE_TIME (65*60*1000)
#define SERVER_LIMIT (10*1024)

FILE *logfile = NULL;

struct userinfo
{
    char *name;
    void *pubkey;
};
hashtable<char *, userinfo> users;

void adduser(char *name, char *pubkey)
{
    name = newstring(name);
    userinfo &u = users[name];
    u.name = name;
    u.pubkey = parsepubkey(pubkey);
}
COMMAND(adduser, "ss");

void clearusers()
{
    enumerate(users, userinfo, u, { delete[] u.name; freepubkey(u.pubkey); });
    users.clear();
}
COMMAND(clearusers, "");

struct baninfo
{
    enet_uint32 ip, mask;
};
vector<baninfo> bans, servbans;

void clearbans()
{
    bans.setsize(0);
    servbans.setsize(0);
}
COMMAND(clearbans, "");

void addban(vector<baninfo> &bans, const char *name)
{
    uchar ip[sizeof(enet_uint32)], mask[sizeof(enet_uint32)];
    memset(ip, 0, sizeof(ip));
    memset(mask, 0, sizeof(mask));
    loopi(4)
    {
        char *end = NULL;
        int n = strtol(name, &end, 10);
        if(!end) break;
        if(end > name) { ip[i] = n; mask[i] = 0xFF; } 
        name = end;
        while(*name && *name++ != '.');
    }
    baninfo &ban = bans.add();
    ban.ip = *(enet_uint32 *)ip; 
    ban.mask = *(enet_uint32 *)mask;
}
ICOMMAND(ban, "s", (char *name), addban(bans, name));
ICOMMAND(servban, "s", (char *name), addban(servbans, name));

bool checkban(vector<baninfo> &bans, enet_uint32 host)
{
    loopv(bans) if((host & bans[i].mask) == bans[i].ip) return true;
    return false;
}

struct authreq
{
    enet_uint32 reqtime; 
    uint id;
    void *answer;
};

struct gameserver
{
    ENetAddress address;
    string ip;
    int port, numpings;
    enet_uint32 lastping, lastpong;
};
vector<gameserver *> gameservers;

struct gameserverlist
{
    vector<char> buf;
    int refs;

    gameserverlist() : refs(0) {}

    const char *getbuf() { return buf.getbuf(); }
    int length() { return buf.length(); }
    void purge();
};
vector<gameserverlist *> gameserverlists;
bool updateserverlist = true;

struct client
{
    ENetAddress address;
    ENetSocket socket;
    char input[INPUT_LIMIT];
    gameserverlist *list;
    vector<char> output;
    int inputpos, outputpos;
    enet_uint32 connecttime, lastinput;
    int servport;
    vector<authreq> authreqs;

    client() : list(NULL), inputpos(0), outputpos(0), servport(-1) {}
};  
vector<client *> clients;

ENetSocket serversocket = ENET_SOCKET_NULL;

time_t starttime;
enet_uint32 servtime = 0;

void fatal(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vfprintf(logfile, fmt, args);
    fputc('\n', logfile);
    va_end(args);
    exit(EXIT_FAILURE);
}

void conoutfv(int type, const char *fmt, va_list args)
{
    vfprintf(logfile, fmt, args);
    fputc('\n', logfile);
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

void purgeclient(int n)
{
    client &c = *clients[n];
    if(c.list) c.list->purge();
    enet_socket_destroy(c.socket);
    delete clients[n];
    clients.remove(n);
}

void output(client &c, const char *msg, int len = 0)
{
    if(!len) len = strlen(msg);
    c.output.put(msg, len);
}

void outputf(client &c, const char *fmt, ...)
{
    string msg;
    va_list args;
    va_start(args, fmt);
    vformatstring(msg, fmt, args);
    va_end(args);

    output(c, msg);
}

ENetSocket pingsocket = ENET_SOCKET_NULL;

bool setuppingsocket()
{
    if(pingsocket != ENET_SOCKET_NULL) return true;
    pingsocket = enet_socket_create(ENET_SOCKET_TYPE_DATAGRAM);
    if(pingsocket == ENET_SOCKET_NULL) return false;
    enet_socket_set_option(pingsocket, ENET_SOCKOPT_NONBLOCK, 1);
    return true;
}

void setupserver(int port, const char *ip = NULL)
{
    ENetAddress address;
    address.host = ENET_HOST_ANY;
    address.port = port;

    if(ip)
    {
        if(enet_address_set_host(&address, ip)<0)
            fatal("failed to resolve server address: %s", ip);
    }
    serversocket = enet_socket_create(ENET_SOCKET_TYPE_STREAM);
    if(serversocket==ENET_SOCKET_NULL || 
       enet_socket_set_option(serversocket, ENET_SOCKOPT_REUSEADDR, 1) < 0 ||
       enet_socket_bind(serversocket, &address) < 0 ||
       enet_socket_listen(serversocket, -1) < 0)
        fatal("failed to create server socket");
    if(enet_socket_set_option(serversocket, ENET_SOCKOPT_NONBLOCK, 1)<0)
        fatal("failed to make server socket non-blocking");
    if(!setuppingsocket())
        fatal("failed to create ping socket");

    enet_time_set(0);
    
    starttime = time(NULL);
    char *ct = ctime(&starttime);
    if(strchr(ct, '\n')) *strchr(ct, '\n') = '\0';
    conoutf("*** Starting master server on %s %d at %s ***", ip ? ip : "localhost", port, ct);
}

void genserverlist()
{
    if(!updateserverlist) return;
    while(gameserverlists.length() && gameserverlists.last()->refs<=0)
        delete gameserverlists.pop();
    gameserverlist *l = new gameserverlist;
    loopv(gameservers)
    {
        gameserver &s = *gameservers[i];
        if(!s.lastpong) continue;
        defformatstring(cmd)("addserver %s %d\n", s.ip, s.port);
        l->buf.put(cmd, strlen(cmd));
    }
    l->buf.add('\0');
    gameserverlists.add(l);
    updateserverlist = false;
}

void addgameserver(client &c)
{
    if(gameservers.length() >= SERVER_LIMIT) return;
    loopv(gameservers)
    {
        gameserver &s = *gameservers[i];
        if(s.address.host == c.address.host && s.port == c.servport)
        {
            s.lastping = 0;
            s.numpings = 0;
            return;
        }
    }
    string hostname;
    if(enet_address_get_host_ip(&c.address, hostname, sizeof(hostname)) < 0)
    {
        outputf(c, "failreg failed resolving ip\n");
        return;
    }     
    gameserver &s = *gameservers.add(new gameserver);
    s.address.host = c.address.host;
    s.address.port = c.servport+1;
    copystring(s.ip, hostname);
    s.port = c.servport;
    s.numpings = 0;
    s.lastping = s.lastpong = 0;
}

void servermessage(gameserver &s, const char *msg)
{
    loopv(clients)
    {
        client &c = *clients[i];    
        if(s.address.host == c.address.host && s.port == c.servport)
        {
            outputf(c, msg);
            return;
        }
    }
}

void checkserverpongs()
{
    ENetBuffer buf;
    ENetAddress addr;
    static uchar pong[MAXTRANS];
    for(;;)
    {
        buf.data = pong;
        buf.dataLength = sizeof(pong);
        int len = enet_socket_receive(pingsocket, &addr, &buf, 1);
        if(len <= 0) break; 
        loopv(gameservers)
        {
            gameserver &s = *gameservers[i];
            if(s.address.host == addr.host && s.address.port == addr.port)
            {
                if(s.lastping && (!s.lastpong || ENET_TIME_GREATER(s.lastping, s.lastpong)))
                    servermessage(s, "succreg\n");
                if(!s.lastpong) updateserverlist = true;
                s.lastpong = servtime ? servtime : 1;
                break;
            }
        }
    }
}

void bangameservers()
{
    loopvrev(gameservers) if(checkban(servbans, gameservers[i]->address.host))
    {
        delete gameservers.remove(i);
        updateserverlist = true;
    }
}

void checkgameservers()
{
    ENetBuffer buf;
    loopv(gameservers)
    {
        gameserver &s = *gameservers[i];
        if(s.lastping && s.lastpong && ENET_TIME_LESS_EQUAL(s.lastping, s.lastpong))
        {
            if(ENET_TIME_DIFFERENCE(servtime, s.lastpong) > KEEPALIVE_TIME)
            {
                delete gameservers.remove(i--);
                updateserverlist = true;
            }
        }
        else if(!s.lastping || ENET_TIME_DIFFERENCE(servtime, s.lastping) > PING_TIME)
        {
            if(s.numpings >= PING_RETRY)
            {
                servermessage(s, "failreg failed pinging server\n");
                delete gameservers.remove(i--);
                updateserverlist = true;
            }
            else
            {
                static const uchar ping[] = { 1 };
                buf.data = (void *)ping;
                buf.dataLength = sizeof(ping);
                s.numpings++;
                s.lastping = servtime ? servtime : 1;
                enet_socket_send(pingsocket, &s.address, &buf, 1);
            }
        }
    }
}

void gameserverlist::purge()
{
    refs = max(refs - 1, 0);
    if(refs<=0 && gameserverlists.last()!=this)
    {
        gameserverlists.removeobj(this);
        delete this;
    }
}

void purgeauths(client &c)
{
    int expired = 0;
    loopv(c.authreqs)
    {
        if(ENET_TIME_DIFFERENCE(servtime, c.authreqs[i].reqtime) >= AUTH_TIME) 
        {
            outputf(c, "failauth %u\n", c.authreqs[i].id);
            freechallenge(c.authreqs[i].answer);
            expired = i + 1;
        }
        else break;
    }
    if(expired > 0) c.authreqs.remove(0, expired);
}

void reqauth(client &c, uint id, char *name)
{
    purgeauths(c);
    
    time_t t = time(NULL);
    char *ct = ctime(&t);
    if(ct) 
    { 
        char *newline = strchr(ct, '\n');
        if(newline) *newline = '\0'; 
    }
    string ip;
    if(enet_address_get_host_ip(&c.address, ip, sizeof(ip)) < 0) copystring(ip, "-");
    conoutf("%s: attempting \"%s\" as %u from %s", ct ? ct : "-", name, id, ip);

    userinfo *u = users.access(name);
    if(!u)
    {
        outputf(c, "failauth %u\n", id);
        return;
    }

    if(c.authreqs.length() >= AUTH_LIMIT)
    {
        outputf(c, "failauth %u\n", c.authreqs[0].id);
        freechallenge(c.authreqs[0].answer);
        c.authreqs.remove(0);
    }

    authreq &a = c.authreqs.add();
    a.reqtime = servtime;
    a.id = id;
    uint seed[3] = { starttime, servtime, randomMT() };
    static vector<char> buf;
    buf.setsizenodelete(0);
    a.answer = genchallenge(u->pubkey, seed, sizeof(seed), buf);

    outputf(c, "chalauth %u %s\n", id, buf.getbuf());
}

void confauth(client &c, uint id, const char *val)
{
    purgeauths(c);

    loopv(c.authreqs) if(c.authreqs[i].id == id)
    {
        string ip;
        if(enet_address_get_host_ip(&c.address, ip, sizeof(ip)) < 0) copystring(ip, "-");
        if(checkchallenge(val, c.authreqs[i].answer))
        {
            outputf(c, "succauth %u\n", id);
            conoutf("succeeded %u from %s", id, ip);
        }    
        else 
        {
            outputf(c, "failauth %u\n", id);
            conoutf("failed %u from %s", id, ip);
        }
        freechallenge(c.authreqs[i].answer);
        c.authreqs.remove(i--);
        return;
    }
    outputf(c, "failauth %u\n", id);
}

bool checkclientinput(client &c)
{
    if(c.inputpos<0) return true;
    char *end = (char *)memchr(c.input, '\n', c.inputpos);
    while(end)
    {
        *end++ = '\0';
        c.lastinput = servtime;

        int port;
        uint id;
        string user, val;
        if(!strncmp(c.input, "list", 4) && (!c.input[4] || isspace(c.input[4])))
        {
            genserverlist();
            if(gameserverlists.empty()) return false;
            c.list = gameserverlists.last();
            c.list->refs++;
            c.outputpos = 0;
            return true;
        }
        else if(sscanf(c.input, "regserv %d", &port) == 1)
        {
            if(checkban(servbans, c.address.host)) return false;
            if(port < 0 || port + 1 < 0 || (c.servport >= 0 && port != c.servport)) outputf(c, "failreg invalid port\n");
            else
            {
                c.servport = port;
                addgameserver(c);
            }
        }
        else if(sscanf(c.input, "reqauth %u %100s", &id, user) == 2)
        {
            reqauth(c, id, user);
        }
        else if(sscanf(c.input, "confauth %u %100s", &id, val) == 2)
        {
            confauth(c, id, val);
        }
        c.inputpos = &c.input[c.inputpos] - end;
        memmove(c.input, end, c.inputpos);

        end = (char *)memchr(c.input, '\n', c.inputpos);
    }
    return c.inputpos<(int)sizeof(c.input);
}

ENetSocketSet readset, writeset;

void checkclients()
{
    ENetSocketSet readset, writeset;
    ENetSocket maxsock = max(serversocket, pingsocket);
    ENET_SOCKETSET_EMPTY(readset);
    ENET_SOCKETSET_EMPTY(writeset);
    ENET_SOCKETSET_ADD(readset, serversocket);
    ENET_SOCKETSET_ADD(readset, pingsocket);
    loopv(clients)
    {
        client &c = *clients[i];
        if(c.list || c.outputpos < c.output.length()) ENET_SOCKETSET_ADD(writeset, c.socket);
        else ENET_SOCKETSET_ADD(readset, c.socket);
        maxsock = max(maxsock, c.socket);
    }
    if(enet_socketset_select(maxsock, &readset, &writeset, 1000)<=0) return;

    if(ENET_SOCKETSET_CHECK(readset, pingsocket)) checkserverpongs();
    if(ENET_SOCKETSET_CHECK(readset, serversocket))
    {
        ENetAddress address;
        ENetSocket clientsocket = enet_socket_accept(serversocket, &address);
        if(clients.length()>=CLIENT_LIMIT || checkban(bans, address.host)) enet_socket_destroy(clientsocket);
        else if(clientsocket!=ENET_SOCKET_NULL)
        {
            int dups = 0, oldest = -1;
            loopv(clients) if(clients[i]->address.host == address.host) 
            {
                dups++;
                if(oldest<0 || clients[i]->connecttime < clients[oldest]->connecttime) oldest = i;
            }
            if(dups >= DUP_LIMIT) purgeclient(oldest);
                
            client *c = new client;
            c->address = address;
            c->socket = clientsocket;
            c->connecttime = servtime;
            c->lastinput = servtime;
            clients.add(c);
        }
    }

    loopv(clients)
    {
        client &c = *clients[i];
        if((c.list || c.outputpos < c.output.length()) && ENET_SOCKETSET_CHECK(writeset, c.socket))
        {
            const char *data = c.list ? c.list->getbuf() : c.output.getbuf();
            int len = c.list ? c.list->length() : c.output.length();
            ENetBuffer buf;
            buf.data = (void *)&data[c.outputpos];
            buf.dataLength = len-c.outputpos;
            int res = enet_socket_send(c.socket, NULL, &buf, 1);
            if(res>=0) 
            {
                c.outputpos += res;
                if(c.outputpos>=len)
                {
                    if(c.list) { purgeclient(i--); continue; }
                    c.output.setsizenodelete(0);
                    c.outputpos = 0;
                }
            }
            else { purgeclient(i--); continue; }
        }
        if(ENET_SOCKETSET_CHECK(readset, c.socket))
        {
            ENetBuffer buf;
            buf.data = &c.input[c.inputpos];
            buf.dataLength = sizeof(c.input) - c.inputpos;
            int res = enet_socket_receive(c.socket, NULL, &buf, 1);
            if(res>0)
            {
                c.inputpos += res;
                c.input[min(c.inputpos, (int)sizeof(c.input)-1)] = '\0';
                if(!checkclientinput(c)) { purgeclient(i--); continue; }
            }
            else { purgeclient(i--); continue; }
        }
        if(c.output.length() > OUTPUT_LIMIT) { purgeclient(i--); continue; }
        if(ENET_TIME_DIFFERENCE(servtime, c.lastinput) >= CLIENT_TIME) { purgeclient(i--); continue; }
    }
}

void banclients()
{
    loopvrev(clients) if(checkban(bans, clients[i]->address.host)) purgeclient(i);
}
        
volatile bool reloadcfg = true;

void reloadsignal(int signum)
{
    reloadcfg = true;
}

int main(int argc, char **argv)
{
    const char *dir = "", *ip = NULL;
    int port = 28787;
    if(argc>=2) dir = argv[1];
    if(argc>=3) port = atoi(argv[2]);
    if(argc>=4) ip = argv[3];
    defformatstring(logname)("%smaster.log", dir);
    defformatstring(cfgname)("%smaster.cfg", dir);
    path(logname);
    path(cfgname);
    logfile = fopen(logname, "a");
    if(!logfile) logfile = stdout;
    setvbuf(logfile, NULL, _IOLBF, 0);
    signal(SIGUSR1, reloadsignal);
    setupserver(port, ip);
    for(;;) 
    {
        if(reloadcfg)
        {
            conoutf("reloading master.cfg");
            execfile(cfgname);
            bangameservers();
            banclients();
            reloadcfg = false;
        }

        servtime = enet_time_get();
        checkclients();
        checkgameservers();
    }

    return EXIT_SUCCESS;
}

