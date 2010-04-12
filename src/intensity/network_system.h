
// Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
// This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

namespace NetworkSystem
{
    namespace Cataloger
    {
        //! Register the sending of a packet via a channel
        void packetSent(int channel, int size);

        //! Register the sending of a message by its code
        void messageSent(int code);

        //! Shows the network activity cataloged since the last show(), and
        //! resets the counters afterwards.
        //! @param seconds Over how many seconds the network activity has been,
        //!                since the last show()
        void show(float seconds);

        std::string briefSummary(float seconds);
    }

    namespace Benchmarker
    {
        //! Show the pingjump (the lag between position updates from other clients)
        void showOtherClients();
    }

    namespace PositionUpdater
    {
        //! A non-optimized storage structure for a position update; in convenient
        //! form to read and write to in C++; the actual protocol message is different.
        //! This structure is used to gather all the info we are considering sending,
        //! all of that in quantized form. It does *not* contain further network message
        //! optimizations like bitfields, packing, unsent fields, etc.
        //! etc.
        //!
        //! For compression, we allow some of the fields here to not be present. Indicators
        //! for the fields are called has[X]; thus, hasPosition is true if this QuantizedInfo
        //! contains position info.
        struct QuantizedInfo
        {
        private:
            //! Internal utility, as this recurs a few times
            int getLifeSequence();

        public:
            int clientNumber;

            bool hasPosition; // Bit 1
            ivec position; // XXX: Currently we store these here as
                           // ints, but over the wire we send tham
                           // as unsigned. This is significantly
                           // better for bandwidth - 20%, even.
                           // This may be a problem if we let people
                           // move into positions with negative X,Y,Z

            bool hasYaw, hasPitch, hasRoll; // Bits 2-4
            unsigned char yaw, pitch, roll;

            bool hasVelocity; // Bit 5
            ivec velocity;

            bool hasFalling; // Bit 6
            ivec falling;

            bool hasMisc; // Bit 7 - represents physicsState (3 bits), lifeSequence (1 bit), move (2 bits), strafe (2 bits)
            unsigned char misc;

            bool hasMapDefinedPositionData; // Bit 8 - see class fpsent
            unsigned int mapDefinedPositionData;

            QuantizedInfo() : hasPosition(true), hasYaw(true), hasPitch(true), hasRoll(true),
                              hasVelocity(true), hasFalling(true), hasMisc(true),
                              hasMapDefinedPositionData(true)
                { };

            //! Fills the fields with data from the given entity. Applies quantization
            //! as appropriate to each field, but nothing more.
            void generateFrom(fpsent *d);

            //! Fills the fields with data from the given buffer, whose source is the network.
            //! This is used both on the client and the server (the server just needs to
            //! read the appropriate number of bytes, and this is discarded).
            //! We receive quantized data, and leave it quantized here, but we do deal with
            //! compression like bitfields, packing, unsent fields, etc.
            void generateFrom(ucharbuf& p);

            //! Applies the fields to the appropriate entity (found using the clientNumber).
            //! This does the opposite of generateFrom(entity), i.e., it unquantizes the info.
            //! If the entity is not supplied, we look it up using its client number.
            void applyToEntity(fpsent *d = NULL);

            //! Applies the fields to a buffer, which can be send over the network. Leaves
            //! fields in quantized form. Applies compression of bitfields, packing, unsent
            //! fields, etc., i.e., the opposite of generateFrom(buffer).
            void applyToBuffer(ucharbuf& q);
        };

        //! Process a position updated which is received by the server, in preparation for
        //! sending it out to the other clients.
        //! The naive approach simply leaves it as-is. A more sophisticated solution
        //! optimizes bandwidth in various ways. Our current model is as follows:
        //!     - Clients send data at full speed, all the time
        //!     - The server reduces the size of those updates, and their frequency,
        //!       in order to save bandwidth
        void processServerPositionReception(QuantizedInfo& info);
    }
}
