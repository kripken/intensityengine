// client.cpp, mostly network related client game code

#include "engine.h"
#include "game.h" // INTENSITY: Need this for fpsent, below

#ifdef CLIENT
    #include "client_system.h" // INTENSITY
#endif

#include "network_system.h" // INTENSITY



ENetHost *clienthost = NULL;
ENetPeer *curpeer = NULL, *connpeer = NULL;
int connmillis = 0, connattempts = 0, discmillis = 0;

bool multiplayer(bool msg)
{
#ifdef CLIENT // INTENSITY
    bool val = !ClientSystem::editingAlone; // INTENSITY
#else // SERVER - the old sauer code
    bool val = curpeer || hasnonlocalclients();
#endif // INTENSITY end
    if(val && msg) conoutf(CON_ERROR, "You must be in private edit mode for that"); // INTENSITY: Message contents
    return val;
}

void setrate(int rate)
{
   if(!curpeer) return;
   enet_host_bandwidth_limit(clienthost, rate, rate);
}

VARF(rate, 0, 0, 25000, setrate(rate));

void throttle();

VARF(throttle_interval, 0, 5, 30, throttle());
VARF(throttle_accel,    0, 2, 32, throttle());
VARF(throttle_decel,    0, 2, 32, throttle());

void throttle()
{
    if(!curpeer) return;
    ASSERT(ENET_PEER_PACKET_THROTTLE_SCALE==32);
    enet_peer_throttle_configure(curpeer, throttle_interval*1000, throttle_accel, throttle_decel);
}

bool isconnected(bool attempt)
{
    return curpeer || (attempt && connpeer);
}

ICOMMAND(isconnected, "i", (int *attempt), intret(isconnected(*attempt > 0) ? 1 : 0));

void abortconnect()
{
    if(!connpeer) return;
    game::connectfail();
    if(connpeer->state!=ENET_PEER_STATE_DISCONNECTED) enet_peer_reset(connpeer);
    connpeer = NULL;
    if(curpeer) return;
    enet_host_destroy(clienthost);
    clienthost = NULL;
}

void connectserv(const char *servername, int serverport, const char *serverpassword)
{   
    if(connpeer)
    {
        conoutf("aborting connection attempt");
        abortconnect();
    }

    if(serverport <= 0) serverport = server::serverport();

    ENetAddress address;
    address.port = serverport;

    if(servername)
    {
        addserver(servername, serverport, serverpassword[0] ? serverpassword : NULL); // INTENSITY: Remove?
        conoutf("attempting to connect to %s:%d", servername, serverport);
        if(!resolverwait(servername, &address))
        {
            conoutf("\f3could not resolve server %s", servername);
            return;
        }
    }
    else
    {
        conoutf("attempting to connect over LAN");
        address.host = ENET_HOST_BROADCAST;
    }

    if(!clienthost) clienthost = enet_host_create(NULL, 2, rate, rate);

    if(clienthost)
    {
        connpeer = enet_host_connect(clienthost, &address, game::numchannels()); 
        enet_host_flush(clienthost);
        connmillis = totalmillis;
        connattempts = 0;

        game::connectattempt(servername ? servername : "", serverpassword ? serverpassword : "", address);
    }
    else conoutf("\f3could not connect to server");
}

void disconnect(bool async, bool cleanup)
{
    if(curpeer) 
    {
        if(!discmillis)
        {
            enet_peer_disconnect(curpeer, DISC_NONE);
            enet_host_flush(clienthost);
            discmillis = totalmillis;
        }
        if(curpeer->state!=ENET_PEER_STATE_DISCONNECTED)
        {
            if(async) return;
            enet_peer_reset(curpeer);
        }
        curpeer = NULL;
        discmillis = 0;
        conoutf("disconnected");
        game::gamedisconnect(cleanup);
        mainmenu = 1;
    }
    if(!connpeer && clienthost)
    {
        enet_host_destroy(clienthost);
        clienthost = NULL;
    }
}

void trydisconnect()
{
    if(connpeer)
    {
        conoutf("aborting connection attempt");
        abortconnect();
    }
    else if(curpeer)
    {
        conoutf("attempting to disconnect...");
        disconnect(!discmillis);
    }
    else conoutf("not connected");
}

ICOMMAND(connect, "sis", (char *name, int *port, char *pw), connectserv(name, *port, pw));
ICOMMAND(lanconnect, "is", (int *port, char *pw), connectserv(NULL, *port, pw));
COMMANDN(disconnect, trydisconnect, "");
ICOMMAND(localconnect, "", (), { if(!isconnected() && !haslocalclients()) localconnect(); });
ICOMMAND(localdisconnect, "", (), { if(haslocalclients()) localdisconnect(); });

void sendclientpacket(ENetPacket *packet, int chan, int cn) // INTENSITY: added cn
{
    if(curpeer) enet_peer_send(curpeer, chan, packet);
    else localclienttoserver(chan, packet, cn); // INTENSITY: added cn

    NetworkSystem::Cataloger::packetSent(chan, packet->dataLength); // INTENSITY
}

void flushclient()
{
    if(clienthost) enet_host_flush(clienthost);
}

void neterr(const char *s, bool disc)
{
    conoutf(CON_ERROR, "\f3illegal network message (%s)", s);
    if(disc) disconnect();
}

void localservertoclient(int chan, ENetPacket *packet)   // processes any updates from the server
{
    packetbuf p(packet);
    game::parsepacketclient(chan, p);
}

void clientkeepalive() { if(clienthost) enet_host_service(clienthost, NULL, 0); }

void gets2c()           // get updates from the server
{
    ENetEvent event;
    if(!clienthost) return;
    if(connpeer && totalmillis/3000 > connmillis/3000)
    {
        conoutf("attempting to connect...");
        connmillis = totalmillis;
        ++connattempts; 
        if(connattempts > 3)
        {
            conoutf("\f3could not connect to server");
            abortconnect();
            return;
        }
    }
    while(clienthost && enet_host_service(clienthost, &event, 0)>0)
    switch(event.type)
    {
        case ENET_EVENT_TYPE_CONNECT:
            disconnect(false, false); 
            localdisconnect(false);
            curpeer = connpeer;
            connpeer = NULL;
            conoutf("connected to server");
            throttle();
            if(rate) setrate(rate);
            game::gameconnect(true);
            break;
         
        case ENET_EVENT_TYPE_RECEIVE:
#if CLIENT // INTENSITY
            if(discmillis) conoutf("attempting to disconnect...");
            else localservertoclient(event.channelID, event.packet);
            enet_packet_destroy(event.packet);
#else // SERVER
            assert(0);
#endif
            break;

        case ENET_EVENT_TYPE_DISCONNECT:
//            LogicSystem::init(); // INTENSITY: Not sure about this...?

            extern const char *disc_reasons[];
            if(event.data>=DISC_NUM) event.data = DISC_NONE;
            if(event.peer==connpeer)
            {
                conoutf("\f3could not connect to server");
                abortconnect();
            }
            else
            {
                if(!discmillis || event.data) conoutf("\f3server network error, disconnecting (%s) ...", disc_reasons[event.data]);
                disconnect();
            }
            return;

        default:
            break;
    }
}

