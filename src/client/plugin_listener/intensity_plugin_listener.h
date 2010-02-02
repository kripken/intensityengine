
/*
 *=============================================================================
 * Copyright (C) 2010 Alon Zakai ('Kripken') kripkensteiner@gmail.com
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


#include <vector>

#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/allocators/allocator.hpp>

#define INTENSITY_CHANNEL "IntensityChannel"
#define CHANNEL_SIZE 65530

typedef boost::interprocess::allocator<char, boost::interprocess::managed_shared_memory::segment_manager> ShmemAllocator;
typedef boost::interprocess::vector<char, ShmemAllocator> ChannelVector;

class SimpleChannel
{
    unsigned int index;
protected:
    ChannelVector *data;
    boost::interprocess::managed_shared_memory *segment;
public:
    SimpleChannel() : index(0), data(NULL), segment(NULL) { };
    void write(std::string message)
    {
        printf("Write from %d\r\n", index);
        for (unsigned int i = 0; i < message.size(); i++)
        {
            assert((*data)[index] == '\0');
            (*data)[index] = message.c_str()[i];
            index++;
            index = index % CHANNEL_SIZE;
        }
        assert((*data)[index] == '\0');
        index++;
        index = index % CHANNEL_SIZE;
    }
    std::string read()
    {
        if ((*data)[index] == '\0') return ""; // Nothing new

        printf("Read from %d\r\n", index);
        std::string message = "";
        while ((*data)[index] != '\0')
        {
            message += (*data)[index];
            (*data)[index] = '\0';
            index++;
            index = index % CHANNEL_SIZE;
        }
        index++; // Skip last \0 symbol in this message
        index = index % CHANNEL_SIZE;
        return message;
    }
    std::vector<std::string> readParsed()
    {
        std::vector<std::string> parsed;
        std::string message = read();
        if (message == "") return parsed;
        printf("Read for parsing: %s\r\n", message.c_str());
        message = message + "|"; // Final barrier
        std::string curr = "";
        for (unsigned int i = 0; i < message.size(); i++)
        {
            if (message[i] == '|')
            {
                parsed.push_back(curr);
                curr = "";
            } else
                curr += message[i];
        }
        return parsed;
    }
};

class ServerChannel : public SimpleChannel
{
public:
    ServerChannel() : SimpleChannel()
    {
        boost::interprocess::shared_memory_object::remove(INTENSITY_CHANNEL);
        segment = new boost::interprocess::managed_shared_memory(
            boost::interprocess::create_only,
            INTENSITY_CHANNEL,
            CHANNEL_SIZE*10
        );

        const ShmemAllocator alloc_inst (segment->get_segment_manager());

        data = segment->construct<ChannelVector>("MyVector")(alloc_inst);

        for(int i = 0; i < CHANNEL_SIZE; ++i){
            data->push_back('\0');
        }
    }
};

class ClientChannel : public SimpleChannel
{
public:
    ClientChannel() : SimpleChannel()
    {
        segment = new boost::interprocess::managed_shared_memory(
            boost::interprocess::open_only,
            INTENSITY_CHANNEL
        );

        data = segment->find<ChannelVector>("MyVector").first;
    }
};

namespace PluginListener
{
    void initialize();
    void frameTrigger();
}

