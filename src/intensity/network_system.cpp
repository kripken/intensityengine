
/*
 *=============================================================================
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
#include "engine.h"
#include "game.h"

#include "network_system.h"
#include "fpsclient_interface.h"
#include "utility.h"


namespace NetworkSystem
{

namespace Cataloger
{
#define NUM_CHANNELS 2

int bytesSentPerChannel[NUM_CHANNELS];

int cachedBytes = 0;    // For cache that gets flushed to a Python signal
int lastCacheFlush = 0; // every second or so

void packetSent(int channel, int size)
{
    assert(channel >= 0 && channel < NUM_CHANNELS);
    bytesSentPerChannel[channel] += size;

    // Caching, then sending out of a signal
    cachedBytes += size;
    if (lastmillis - lastCacheFlush >= 1000) // use lastmillis for speed - this happens a lot
    {
        REFLECT_PYTHON( signal_bandwidth_out );
        signal_bandwidth_out(cachedBytes);

        lastCacheFlush = lastmillis;
        cachedBytes = 0;
    }
}

typedef std::map<int, int> MessagesSentPerCode;
MessagesSentPerCode messagesSentPerCode;

void messageSent(int code)
{
    messagesSentPerCode[code] += 1;
}

void show(float seconds)
{
    printf("   Network activity breakdown by channel:\r\n");
    for (int i = 0; i < NUM_CHANNELS; i++)
    {
        printf("      %d - %.1fK/sec sent\r\n", i, float(bytesSentPerChannel[i])/seconds/1024);
        bytesSentPerChannel[i] = 0;
    }

    printf("   Network activity breakdown by message code:\r\n");
    for(MessagesSentPerCode::iterator iter = messagesSentPerCode.begin();
        iter != messagesSentPerCode.end();
        iter++)
    {
        int code = iter->first;
        printf("      %d - %.1f messages/sec sent\r\n", code, float(messagesSentPerCode[code])/seconds);
    }
    messagesSentPerCode.clear();
}

std::string briefSummary(float seconds)
{
    std::string ret = "";
    static string temp;
    for (int i = 0; i < NUM_CHANNELS; i++)
    {
        formatstring(temp)("%d - %.1fKB/s sent", i, float(bytesSentPerChannel[i])/seconds/1024);
        ret += temp;
        bytesSentPerChannel[i] = 0;

        if (i != NUM_CHANNELS-1)
            ret += "   ";
    }

    return ret;
}

}


//==================
// Benchmarker
//==================

namespace Benchmarker
{

void showOtherClients()
{
    printf("Network benchmarks for other clients:\r\n");

    for (int i = 0; i < FPSClientInterface::getNumPlayers(); i++)
    {
        fpsent* player = dynamic_cast<fpsent*>(FPSClientInterface::getPlayerByNumber(i));
        if (!player) continue; // There are empty spots
        printf("   Ping-lag: %4d\r\n", player->plag);
    }
}

}


//==================
// PositionUpdater
//==================

namespace PositionUpdater
{

int QuantizedInfo::getLifeSequence()
{
    return (misc >> 3) & 1;
}

void QuantizedInfo::generateFrom(fpsent *d)
{
    clientNumber = d->clientnum; // Kripken: Changed player1 to d, so this will work for NPCs as well

    position.x = (int)(d->o.x*DMF);              // quantize coordinates to 1/4th of a cube, between 1 and 3 bytes
    position.y = (int)(d->o.y*DMF);
    position.z = (int)(d->o.z*DMF);

    d->normalize_yaw(180.0f); // Kripken: Normalize to 0-360
    yaw = (unsigned char)(256.0f * d->yaw / 360.0f); // Kripken: Send quantized to 256 values, 1 byte

    d->normalize_pitch(0.0f); // Kripken: Normalize to -180-180
    pitch = (unsigned char)(256.0f * (d->pitch + 180.0f) / 360.0f); // Kripken: Send quantized to 256 values, 1 byte

    d->normalize_roll(0.0f); // Kripken: Normalize to -180-180
    roll = (unsigned char)(256.0f * (d->roll + 180.0f) / 360.0f); // Kripken: Send quantized to 256 values, 1 byte

    velocity.x = (int)(d->vel.x*DVELF);          // quantize to itself, almost always 1 byte
    velocity.y = (int)(d->vel.y*DVELF);
    velocity.z = (int)(d->vel.z*DVELF);

    falling.x = (int)(d->falling.x*DVELF);      // quantize to itself, almost always 1 byte
    falling.y = (int)(d->falling.y*DVELF);
    falling.z = (int)(d->falling.z*DVELF); // XXX: hasFalling is done the old Sauer way, not the new way
    hasFalling = (falling.x || falling.y || falling.z); // XXX: hasFalling is done the old Sauer way, not the new way

    assert(d->physstate < 8);

    misc = 0;
    misc |= d->physstate;
    misc |= (d->lifesequence << 3);
    misc |= ((d->move + 1) << 4);
    misc |= ((d->strafe + 1) << 6);

    mapDefinedPositionData = d->mapDefinedPositionData;
}

void QuantizedInfo::generateFrom(ucharbuf& p)
{
    clientNumber = getint(p);// printf("START GET: %d\r\n", clientNumber);

    // Indicates which fields are in fact present (the has[X] stuff)
    unsigned char indicator = p.get();// printf("GET: %u\r\n", indicator); 
////////////////////////////////////printf("generateFrom: %d (%d,%d,%d,%d,%d,%d,%d,%d)\r\n", indicator, hasPosition, hasYaw, hasPitch, hasRoll, hasVelocity, hasFallingXY, hasFallingZ, hasMisc);
    hasPosition = (indicator & 1) != 0;
    hasYaw = (indicator & 2) != 0;
    hasPitch = (indicator & 4) != 0;
    hasRoll = (indicator & 8) != 0;
    hasVelocity = (indicator & 16) != 0; 
    hasFalling = (indicator & 32) != 0;
    hasMisc = (indicator & 64) != 0;
    hasMapDefinedPositionData = (indicator & 128) != 0;

    if (hasPosition)
    {
        position.x = getuint(p);// printf("GET: %d\r\n", position.x);
        position.y = getuint(p);// printf("GET: %d\r\n", position.y);
        position.z = getuint(p);// printf("GET: %d\r\n", position.z);
    }

    if (hasYaw)
    {
        yaw = p.get();// printf("GET: %u\r\n", yaw);
    }

    if (hasPitch)
    {
        pitch = p.get();// printf("GET: %u\r\n", pitch);
    }

    if (hasRoll)
    {
        roll = p.get();// printf("GET: %u\r\n", roll);
    }

    if (hasVelocity)
    {
        velocity.x = getint(p);// printf("GET: %d\r\n", velocity.x);
        velocity.y = getint(p);// printf("GET: %d\r\n", velocity.y);
        velocity.z = getint(p);// printf("GET: %d\r\n", velocity.z);
    }

    if (hasFalling)
    {
        falling.x = getint(p);// printf("GET: %d\r\n", falling.x);
        falling.y = getint(p);// printf("GET: %d\r\n", falling.y);
        falling.z = getint(p);// printf("GET: %d\r\n", falling.z);
    } else {
        // XXX: hasFalling is done the old Sauer way, not the new way
        falling.x = 0;
        falling.y = 0;
        falling.z = 0;
    }

    if (hasMisc)
    {
        misc = p.get();// printf("GET: %u\r\n", misc);
    }

    if (hasMapDefinedPositionData)
    {
        mapDefinedPositionData = getuint(p);
    }
}

void QuantizedInfo::applyToEntity(fpsent *d)
{
    if (d == NULL)
        d = dynamic_cast<fpsent*>(FPSClientInterface::getPlayerByNumber(clientNumber));
//        fpsent *d = cl.getclient(cn);

    // Only possibly discard if we get a value for the lifesequence
    if(!d || (hasMisc && (getLifeSequence()!=(d->lifesequence&1))))
    {
        Logging::log(Logging::WARNING, "Not applying position update for client %d, reasons: %lu,%d,%d (real:%d)\r\n",
                     clientNumber, (unsigned long)d, getLifeSequence(), d ? d->lifesequence&1 : -1, d ? d->lifesequence : -1);
        return;
    } else
        Logging::log(Logging::INFO, "Applying position update for client %d\r\n", clientNumber);

    #ifdef SERVER
    if(d->serverControlled) // Server does not need to update positions of its own NPCs. TODO: Don't even send to here.
    {
        Logging::log(Logging::INFO, "Not applying position update for server NPC: (uid: %d , addr %d):\r\n", d->uniqueId, d != NULL);
        return;
    }
    #endif

    #ifdef CLIENT
    float oldyaw = d->yaw, oldpitch = d->pitch;
    #endif

    if (hasYaw)
    {
        d->yaw = 360.0f * float(yaw) / 256.0f; // Kripken: Unquantize from 256 values
        d->normalize_yaw(180.0f); // Kripken: Normalize to 0-360
    }

    if (hasPitch)
    {
        d->pitch = (360.0f * float(pitch) / 256.0f) - 180.0f; // Kripken: Unquantize from 256 values
        d->normalize_pitch(0.0f); // Kripken: Normalize to -180-180
    }

    if (hasRoll)
    {
        d->roll = (360.0f * float(roll) / 256.0f) - 180.0f; // Kripken: Unquantize from 256 values
        d->normalize_roll(0.0f); // Kripken: Normalize to -180-180
    }

    if (hasMisc)
    {
        d->lifesequence = getLifeSequence();
        d->move = ((misc >> 4) & 3) - 1;
        d->strafe = ((misc >> 6) & 3) - 1;
    }

    if (hasMapDefinedPositionData)
    {
        d->mapDefinedPositionData = mapDefinedPositionData;
    }

    vec oldpos(d->o);

    if(game::allowmove(d))
    {
        if (hasPosition)
            d->o = vec(position.x/DMF, position.y/DMF, position.z/DMF);
        if (hasVelocity)
            d->vel = vec(velocity.x/DVELF, velocity.y/DVELF, velocity.z/DVELF);
        if (hasFalling)
        {
            d->falling.x = falling.x/DVELF;
            d->falling.y = falling.y/DVELF;
            d->falling.z = falling.z/DVELF;
        } else {
            d->falling.x = 0;
            d->falling.y = 0;
            d->falling.z = 0;
        }
        if (hasMisc)
            d->physstate = misc & 7;

        updatephysstate(d);
        FPSClientInterface::updatePosition(d);
    }
    #ifdef CLIENT // No need to smooth for server, and certainly no need to double smooth before getting to other clients
    if(FPSClientInterface::smoothmove() && d->smoothmillis>=0 && oldpos.dist(d->o) < FPSClientInterface::smoothdist())
    {
        d->newpos = d->o;
        d->newyaw = d->yaw;
        d->newpitch = d->pitch;
        d->o = oldpos;
        d->yaw = oldyaw;
        d->pitch = oldpitch;
        (d->deltapos = oldpos).sub(d->newpos);
        d->deltayaw = oldyaw - d->newyaw;
        if(d->deltayaw > 180) d->deltayaw -= 360;
        else if(d->deltayaw < -180) d->deltayaw += 360;
        d->deltapitch = oldpitch - d->newpitch;
        d->smoothmillis = lastmillis;
    }
    else
    #endif
        d->smoothmillis = 0;

    if(d->state==CS_LAGGED || d->state==CS_SPAWNING) d->state = CS_ALIVE;
}

void QuantizedInfo::applyToBuffer(ucharbuf& q)
{
    putint(q, SV_POS);// printf("(PUT SV_POS): %d\r\n", SV_POS);

    putint(q, clientNumber);// printf("START PUT: %d\r\n", clientNumber);

    unsigned char indicator = 0; // Indicates which fields are in fact present (the has[X] stuff)
    indicator |= hasPosition;
    indicator |= (hasYaw << 1);
    indicator |= (hasPitch << 2);
    indicator |= (hasRoll << 3);
    indicator |= (hasVelocity << 4);
    indicator |= (hasFalling << 5);
    indicator |= (hasMisc << 6);
    indicator |= (hasMapDefinedPositionData << 7);
///////////////////////////////printf("applyToBuffer: %d (%d,%d,%d,%d,%d,%d,%d,%d)\r\n", indicator, hasPosition, hasYaw, hasPitch, hasRoll, hasVelocity, hasFallingXY, hasFallingZ, hasMisc);
    q.put(indicator);// printf("PUT: %u\r\n", indicator);

    if (hasPosition)
    {
        putuint(q, position.x);// printf("PUT: %d\r\n", position.x);
        putuint(q, position.y);// printf("PUT: %d\r\n", position.y);
        putuint(q, position.z);// printf("PUT: %d\r\n", position.z);
    }

    if (hasYaw)
    {
        q.put(yaw);// printf("PUT: %u\r\n", yaw);
    }

    if (hasPitch)
    {
        q.put(pitch);// printf("PUT: %u\r\n", pitch);
    }

    if (hasRoll)
    {
        q.put(roll);// printf("PUT: %u\r\n", roll);
    }

    if (hasVelocity)
    {
        putint(q, velocity.x);// printf("PUT: %d\r\n", velocity.x);
        putint(q, velocity.y);// printf("PUT: %d\r\n", velocity.y);
        putint(q, velocity.z);// printf("PUT: %d\r\n", velocity.z);
    }

    if (hasFalling)
    {
        putint(q, falling.x);// printf("PUT: %d\r\n", falling.x);
        putint(q, falling.y);// printf("PUT: %d\r\n", falling.y);
        putint(q, falling.z);// printf("PUT: %d\r\n", falling.z);
    }

    if (hasMisc)
    {
        q.put(misc);// printf("PUT: %u\r\n", misc);
    }

    if (hasMapDefinedPositionData)
    {
        putuint(q, mapDefinedPositionData);
    }
///////////////////////printf("***Generated size: %d\r\n", q.length());
}


//======================================
// Bandwidth optimization system for
// position updates
//======================================


//! How fast we would like to send, assuming that we are talking about
//! sending a value that has not changed. Right after a change, we still send
//! at the normal rate (clients might miss some updates), but later on we
//! gradually lower that rate (since with high probability they have already
//! received it).
//! @param sinceChanged The time since there was a change to this value
//! @param receiveLatency The average latency of receiving from this
//!                       client, which is an estimate of the 'full speed'
//!                       that would be used for a constantly-changing
//!                       value.
int calcDesiredLatency(int sinceChanged, int receiveLatency)
{
    // At this time we will decay to our slowest rate (secs)
    float maxTime = 2.0f;
    // How much weight to give the slowest possible rate
    float slowestFactor = min((float(sinceChanged)/1000.0f)/maxTime, 1.0f);
    // At least 3fps, but generally about 1/10 normal speed
    // (if normal = 30fps, then that is 3fps).
    int slowestLatency = min(receiveLatency*10, 333); 

    return int( (slowestLatency*slowestFactor) + (receiveLatency*(1-slowestFactor)) );
}

//! Decide whether to send an update about a datum, or not
//! @param sameValue Whether this value has changed or not
//! @param sinceSend The amount of time since we sent an update about this value
//! @param receiveLatencyRaw The latency in receiving updates for this client.
//!                          If latency is high we are more conservative about
//!                          our decisions, so as not to make things worse. This
//!                          is the 'raw' estimate - our best guess.
//! @param receiveLatencySafe This is a 'safe' version, taking into account some
//!                           variance.
//! @param sinceChanged The amount of time since this value has changed
//! @return Whether the decision is to send this value
bool positionDatumDecider(bool sameValue, int sinceSend, int receiveLatencyRaw, int receiveLatencySafe, int sinceChanged)
{
//////////////////////////printf("DECIDER: same: %d  last: %d   raw: %d    safe: %d      sinceChange: %d\r\n",        sameValue, sinceSend, receiveLatencyRaw, receiveLatencySafe, sinceChanged);

    if (sinceSend == -1)
        return true; // No previously sent value, so decide to send

    if (!sameValue)
        return true; // For now, always send different values

    // This is the same value as we last sent, possibly decide to not sent it.

    // How much time passed since we last sent
    int currLatency = sinceSend;
    // How much time we estimate will pass until the next sending opportunity.
    // Use a conservative 'safe' estimate of latency.
    int estimatedFutureLatency = receiveLatencySafe;
    // The latency we are shooting for. Use 'raw' best-guess estimate of latency.
    int desiredLatency = calcDesiredLatency(sinceChanged, receiveLatencyRaw);

//////////////////////////printf("Decider: curr, future: %d, %d     desired: %d       hence: %d\r\n", currLatency, estimatedFutureLatency, desiredLatency,       (currLatency + estimatedFutureLatency >= desiredLatency)    );

    // If the overall latency between messages will be as we want, or more, send now
    return (currLatency + estimatedFutureLatency >= desiredLatency);
}


//! Client-specific data about position updates. Used to optimize the protocol.
class ClientDatum
{
private:
    //! How much weight to give past values in weighted averages
    #define PAST_WEIGHT_FACTOR 0.975

    //! How much time passed between receiving a position
    //! update from this client. An online weighted average.
    float receiveLatency;

    //! The variability (standard dev^2) in latency, as an online weighted average.
    float receiveLatencyVariance;

    // The last point in time when we received a position update from this client
    int lastReceived;

    void updateReceiveStats()
    {
        int currTime = Utility::SystemInfo::currTime();
        if (lastReceived != -1)
        {
            int currLatency = currTime - lastReceived;

            if (receiveLatency == -1)
            {
                // Our initial guess is the normal latency and 0 variance
                receiveLatency = Utility::Config::getInt("Network", "rate", 33);
                receiveLatencyVariance = 0;
            }

            receiveLatency = (PAST_WEIGHT_FACTOR * receiveLatency) + ( (1-PAST_WEIGHT_FACTOR) * currLatency );
            float delta = currLatency - receiveLatency;
            receiveLatencyVariance = (PAST_WEIGHT_FACTOR * receiveLatencyVariance) + ( (1-PAST_WEIGHT_FACTOR) * delta * delta);

///////////////////////////////printf("STATS: %d ==> AVGs: %f,%f\r\n", currLatency, receiveLatency, sqrtf(receiveLatencyVariance));
        }
        lastReceived = currTime;
    }

    //! Estimate the latency, with a safety factor of a given number of standard deviations
    int estimateReceiveLatency(float standardDevs)
    {
        if (receiveLatency != -1)
            return int( receiveLatency + (standardDevs*sqrtf(receiveLatencyVariance)) );
        else
            return Utility::Config::getInt("Network", "rate", 33);; // Meaningless
    }

    #define DATUMINFO(name, type)                    \
        struct _last##name {                         \
            int lastSend;                            \
            int lastChange;                          \
            type value;                              \
            _last##name()                            \
                { lastSend = -1; lastChange = -1; }; \
        } last##name;

    DATUMINFO(position, ivec);
    DATUMINFO(yaw, unsigned char);
    DATUMINFO(pitch, unsigned char);
    DATUMINFO(roll, unsigned char);
    DATUMINFO(velocity, ivec);
    DATUMINFO(falling, ivec);
    DATUMINFO(misc, unsigned char);
    DATUMINFO(mapDefinedPositionData, unsigned int);

    #define SAFE_LATENCY_STANDARD_DEVS 1

    // Simple processing system:
    //      If no indicator, then nothing to send
    //      Else use algorithm to decide if to send
    //      If send, remember this last value and time
    #define PROCESSDATUM(name, indicator)                                 \
        {                                                                 \
            int currTime = Utility::SystemInfo::currTime();               \
            if (!indicator) return;                                       \
            bool sameValue = (info.name == last##name.value);             \
            indicator = positionDatumDecider(                             \
                sameValue,                                                \
                last##name.lastSend != -1                                 \
                    ? currTime - last##name.lastSend                      \
                    : -1,                                                 \
                estimateReceiveLatency(0),                                \
                estimateReceiveLatency(SAFE_LATENCY_STANDARD_DEVS),       \
                last##name.lastChange != -1                               \
                    ? currTime - last##name.lastChange                    \
                    : -1                                                  \
            );                                                            \
            if (indicator)                                                \
            {                                                             \
               last##name.value = info.name;                              \
               last##name.lastSend = currTime;                            \
            }                                                             \
            if (!sameValue)                                               \
                last##name.lastChange = currTime;                         \
        }

public:
    ClientDatum() : receiveLatency(-1), receiveLatencyVariance(-1), lastReceived(-1)
        { };

    //! Called upon reception of an Info for this client.
    //! Updates reception states, and based on that info and previous
    //! sent infos decides what to send.
    //!
    //! XXX Note that we might be tempted to completely not send a
    //! message if all the fields are !has[X]. However, due to falling
    //! using the original Sauer compression method, !hasFalling means
    //! that falling is 0, not that it hasn't changed. So not sending
    //! the entire message in this case will lead to not sending
    //! notifications for the change to 0.
    //!
    //! That said, note that it would be good to 'sync' updates,
    //! to prevent sending two halves instead of whole,*no send*,
    //! as the overhead for sending at all means the two halves
    //! are biger than a whole and a not sending. So, should be
    //! done, but carefully.
    void process(QuantizedInfo& info)
    {
        updateReceiveStats();

        PROCESSDATUM(position, info.hasPosition);
        PROCESSDATUM(yaw, info.hasYaw);
        PROCESSDATUM(pitch, info.hasPitch);
        PROCESSDATUM(roll, info.hasRoll);
        PROCESSDATUM(velocity, info.hasVelocity); // TODO: Should probably be done like falling, as it tends to zero out
        // XXX falling is not done here, as  it is already compressed elsewhere //        PROCESSDATUM(falling, info.hasFalling);
        PROCESSDATUM(misc, info.hasMisc);
        PROCESSDATUM(mapDefinedPositionData, info.hasMapDefinedPositionData);
///////////////////////////////////printf("%d Has: %d,%d,%d,%d,%d,%d,%d,%d\r\n", info.clientNumber, info.hasPosition, info.hasYaw, info.hasPitch, info.hasRoll, info.hasVelocity, info.hasFallingXY, info.hasFallingZ, info.hasMisc);
    }
};

std::map<int, ClientDatum> clientData;

void processServerPositionReception(QuantizedInfo& info)
{
    ClientDatum& data = clientData[info.clientNumber];
    data.process(info);
}

}

}

