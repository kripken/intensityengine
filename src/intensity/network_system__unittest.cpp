
// Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
// This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

#include "cube.h"
#include "engine.h"
#include "game.h"

#include "network_system.h"


void compareEntities(std::string title, fpsent& d, fpsent& d2)
{
    if (d.o.dist(d2.o) >= 0.2f) // Tolerate diff of a bit over ||(1/32,1/32,1/32)||
    {
        printf("o failure: %f,%f,%f vs. %f,%f,%f\r\n", d.o.x, d.o.y, d.o.z, d2.o.x, d2.o.y, d2.o.z);
        exit(0);
    }
    if (fabs(d.yaw - d2.yaw) >= 361.0f/256.0f && fabs(d.yaw - d2.yaw) != 360.0f) // 361, for a little safety
    {
        printf("yaw failure: %f, %f\r\n", d.yaw, d2.yaw);
        exit(0);
    }
    if (fabs(d.pitch - d2.pitch) >= 361.0f/256.0f && fabs(d.pitch - d2.pitch) != 360.0f) // 361, for a little safety
    {
        printf("pitch failure: %f, %f\r\n", d.pitch, d2.pitch);
        exit(0);
    }
    if (fabs(d.roll - d2.roll) >= 361.0f/256.0f && fabs(d.roll - d2.roll) != 360.0f) // 361, for a little safety
    {
        printf("roll failure\r\n");
        exit(0);
    }
    if (d.vel.dist(d2.vel) >= 1.75) // Tolerate diff of a bit over ||(1,1,1)||
    {
        printf("vel failure: %f,%f,%f vs. %f,%f,%f\r\n", d.vel.x, d.vel.y, d.vel.z, d2.vel.x, d2.vel.y, d2.vel.z);
        exit(0);
    }
    if (!(d.physstate == d2.physstate))
    {
        printf("%s physstate failure: %d, %d\r\n", title.c_str(), d.physstate, d2.physstate);
        exit(0);
    }
    if (!((d.lifesequence&1) == (d2.lifesequence&1)))
    {
        printf("%s lifesequence failure\r\n", title.c_str());
        exit(0);
    }
    if (d.falling.dist(d2.falling) >= 1.75) // Tolerate diff of a bit over ||(1,1,1)||
    {
        printf("%s falling failure: %f,%f,%f vs. %f,%f,%f\r\n",title.c_str(), 
               d.falling.x, d.falling.y, d.falling.z, d2.falling.x, d2.falling.y, d2.falling.z);
        exit(0);
    }
    if (!(d.move == d2.move))
    {
        printf("%s move failure: %d, %d\r\n", title.c_str(), d.move, d2.move);
        exit(0);
    }
    if (!(d.strafe == d2.strafe))
    {
        printf("%s strafe failure: %d, %d\r\n", title.c_str(), d.strafe, d2.strafe);
        exit(0);
    }
}

void compareInfos(NetworkSystem::PositionUpdater::QuantizedInfo& info, NetworkSystem::PositionUpdater::QuantizedInfo& info2)
{
    if (info2.clientNumber != info.clientNumber)
    {
        printf("Info clientnumber failure: %d,%d\r\n", info.clientNumber, info2.clientNumber);
        exit(0);
    }

    // TODO: Other fields
}

void testPositionUpdater()
{
    for (int i = 0; i < 20000; i++)
    {
        // Create an entity with some random data

        fpsent d;

        d.clientnum = i;

        #define RANDCOORD ( float( (i % 2048*32) - rnd(2048*32) ) / 32.0f )
        d.o = vec(RANDCOORD, RANDCOORD, RANDCOORD);

        d.yaw = rnd(720);
        d.pitch = rnd(720) - 360;
        d.roll = rnd(720) - 360;

        #define RANDVELCOORD ( float( (i % 2048) - rnd(2048) ) / 128.0f )
        d.vel = vec(RANDVELCOORD, RANDVELCOORD, RANDVELCOORD);

        d.physstate = rnd(8);

        d.lifesequence = rnd(2);

        d.falling = vec(RANDVELCOORD, RANDVELCOORD, RANDVELCOORD);

        d.move = rnd(3) - 1;

        d.strafe = rnd(3) - 1;

        // Generate Info

        NetworkSystem::PositionUpdater::QuantizedInfo info;
        info.generateFrom(&d);

        // Extract from Info into an appropriate other entity

        fpsent d2;
        d2.clientnum = i;
        d2.lifesequence = d.lifesequence;

        info.applyToEntity(&d2);

        // Ensure the extracted info is equal to the original

        compareEntities("original to Info", d, d2);

        // Write out to a buffer

        int maxLength = 200; // TODO: Sync with production code
        unsigned char data[maxLength];
        ucharbuf q(data, maxLength);
        info.applyToBuffer(q);

        // Initialize the the buffer and read the protocol code, which should be SV_POS
        q.len = 0;
        assert(getint(q) == SV_POS);

        // Read into another Info, and apply to another entity

        NetworkSystem::PositionUpdater::QuantizedInfo info2;
        info2.generateFrom(q);

        // Ensure Infos are the same after reconstruction
        compareInfos(info, info2);

        fpsent d3;
        d3.clientnum = i;
        d3.lifesequence = d.lifesequence;

        info2.applyToEntity(&d3);

        // Ensure the extracted info is equal to the original

        compareEntities("original to network reconstruction", d, d3);
    }
}

int main()
{
    // TODO: Test with missing values

    printf("Initializing...\r\n");

    Logging::setCurrLevel(Logging::INFO); // Show everything

    printf("You should now see an INFO message shown for testing purposes\r\n");

    Logging::log(Logging::INFO, "Testing showing of an INFO message\r\n");

    initserver(false, true);

    printf("Running PositionUpdater tests\r\n");

    testPositionUpdater();

    printf("All tests completed successfully\r\n");
    return 1;
};

