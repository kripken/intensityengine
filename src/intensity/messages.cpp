
// Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
// This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

// Automatically generated from messages.template - DO NOT MODIFY THIS FILE!


#include "cube.h"
#include "engine.h"
#include "game.h"

#include "fpsclient_interface.h"

#ifdef CLIENT
    #include "targeting.h"
    #include "intensity_gui.h"
#endif

#include "client_system.h"
#include "server_system.h"
#include "message_system.h"
#include "editing_system.h"
#include "script_engine_manager.h"
#include "utility.h"
#include "world_system.h"

using namespace boost;

namespace MessageSystem
{

// PersonalServerMessage

    void send_PersonalServerMessage(int clientNumber, int originClientNumber, std::string title, std::string content)
    {
        int exclude = -1; // Set this to clientNumber to not send to

        Logging::log(Logging::DEBUG, "Sending a message of type PersonalServerMessage (1001)\r\n");
        INDENT_LOG(Logging::DEBUG);

         

        int start, finish;
        if (clientNumber == -1)
        {
            // Send to all clients
            start  = 0;
            finish = getnumclients() - 1;
        } else {
            start  = clientNumber;
            finish = clientNumber;
        }

#ifdef SERVER
        int testUniqueId;
#endif
        for (clientNumber = start; clientNumber <= finish; clientNumber++)
        {
            if (clientNumber == exclude) continue;
#ifdef SERVER
            fpsent* fpsEntity = dynamic_cast<fpsent*>( FPSClientInterface::getPlayerByNumber(clientNumber) );
            bool serverControlled = fpsEntity ? fpsEntity->serverControlled : false;

            testUniqueId = FPSServerInterface::getUniqueId(clientNumber);
            if ( (!serverControlled && testUniqueId != DUMMY_SINGLETON_CLIENT_UNIQUE_ID) || // If a remote client, send even if negative (during login process)
                 (false && testUniqueId == DUMMY_SINGLETON_CLIENT_UNIQUE_ID) || // If need to send to dummy server, send there
                 (false && testUniqueId != DUMMY_SINGLETON_CLIENT_UNIQUE_ID && serverControlled) )  // If need to send to npcs, send there
#endif
            {
                #ifdef SERVER
                    Logging::log(Logging::DEBUG, "Sending to %d (%d) ((%d))\r\n", clientNumber, testUniqueId, serverControlled);
                #endif
                sendf(clientNumber, MAIN_CHANNEL, "riiss", 1001, originClientNumber, title.c_str(), content.c_str());

            }
        }
    }

#ifdef CLIENT
    void PersonalServerMessage::receive(int receiver, int sender, ucharbuf &p)
    {
        bool is_npc;
        is_npc = false;
        Logging::log(Logging::DEBUG, "MessageSystem: Receiving a message of type PersonalServerMessage (1001)\r\n");

        int originClientNumber = getint(p);
        char tmp_title[MAXTRANS];
        getstring(tmp_title, p);
        std::string title = tmp_title;
        char tmp_content[MAXTRANS];
        getstring(tmp_content, p);
        std::string content = tmp_content;

        IntensityGUI::showMessage(title, content, originClientNumber);
    }
#endif


// RequestServerMessageToAll

    void send_RequestServerMessageToAll(std::string message)
    {
        Logging::log(Logging::DEBUG, "Sending a message of type RequestServerMessageToAll (1002)\r\n");
        INDENT_LOG(Logging::DEBUG);

        game::addmsg(1002, "rs", message.c_str());
    }

#ifdef SERVER
    void RequestServerMessageToAll::receive(int receiver, int sender, ucharbuf &p)
    {
        Logging::log(Logging::DEBUG, "MessageSystem: Receiving a message of type RequestServerMessageToAll (1002)\r\n");

        char tmp_message[MAXTRANS];
        getstring(tmp_message, p);
        std::string message = tmp_message;

        send_PersonalServerMessage(-1, sender, "Message from Client", message);
    }
#endif

// LoginRequest

    void send_LoginRequest(std::string code)
    {
        Logging::log(Logging::DEBUG, "Sending a message of type LoginRequest (1003)\r\n");
        INDENT_LOG(Logging::DEBUG);

        game::addmsg(1003, "rs", code.c_str());
    }

#ifdef SERVER
    void LoginRequest::receive(int receiver, int sender, ucharbuf &p)
    {
        Logging::log(Logging::DEBUG, "MessageSystem: Receiving a message of type LoginRequest (1003)\r\n");

        char tmp_code[MAXTRANS];
        getstring(tmp_code, p);
        std::string code = tmp_code;

                     // identity of this user
        #ifdef SERVER
            REFLECT_PYTHON( do_login );
            do_login(code, sender, getclientip(sender));
        #else // CLIENT, during a localconnect
            ClientSystem::uniqueId = 9999; // Dummy safe uniqueId value for localconnects. Just set it here, brute force
            // Notify client of results of login
            send_LoginResponse(sender, true, true);
        #endif
    }
#endif

// YourUniqueId

    void send_YourUniqueId(int clientNumber, int uniqueId)
    {
        int exclude = -1; // Set this to clientNumber to not send to

        Logging::log(Logging::DEBUG, "Sending a message of type YourUniqueId (1004)\r\n");
        INDENT_LOG(Logging::DEBUG);

                 // Remember this client's unique ID. Done here so always in sync with the client's belief about its uniqueId.
        FPSServerInterface::getUniqueId(clientNumber) = uniqueId;


        int start, finish;
        if (clientNumber == -1)
        {
            // Send to all clients
            start  = 0;
            finish = getnumclients() - 1;
        } else {
            start  = clientNumber;
            finish = clientNumber;
        }

#ifdef SERVER
        int testUniqueId;
#endif
        for (clientNumber = start; clientNumber <= finish; clientNumber++)
        {
            if (clientNumber == exclude) continue;
#ifdef SERVER
            fpsent* fpsEntity = dynamic_cast<fpsent*>( FPSClientInterface::getPlayerByNumber(clientNumber) );
            bool serverControlled = fpsEntity ? fpsEntity->serverControlled : false;

            testUniqueId = FPSServerInterface::getUniqueId(clientNumber);
            if ( (!serverControlled && testUniqueId != DUMMY_SINGLETON_CLIENT_UNIQUE_ID) || // If a remote client, send even if negative (during login process)
                 (false && testUniqueId == DUMMY_SINGLETON_CLIENT_UNIQUE_ID) || // If need to send to dummy server, send there
                 (false && testUniqueId != DUMMY_SINGLETON_CLIENT_UNIQUE_ID && serverControlled) )  // If need to send to npcs, send there
#endif
            {
                #ifdef SERVER
                    Logging::log(Logging::DEBUG, "Sending to %d (%d) ((%d))\r\n", clientNumber, testUniqueId, serverControlled);
                #endif
                sendf(clientNumber, MAIN_CHANNEL, "rii", 1004, uniqueId);

            }
        }
    }

#ifdef CLIENT
    void YourUniqueId::receive(int receiver, int sender, ucharbuf &p)
    {
        bool is_npc;
        is_npc = false;
        Logging::log(Logging::DEBUG, "MessageSystem: Receiving a message of type YourUniqueId (1004)\r\n");

        int uniqueId = getint(p);

        Logging::log(Logging::DEBUG, "Told my unique ID: %d\r\n", uniqueId);
        ClientSystem::uniqueId = uniqueId;
    }
#endif


// LoginResponse

    void send_LoginResponse(int clientNumber, bool success, bool local)
    {
        int exclude = -1; // Set this to clientNumber to not send to

        Logging::log(Logging::DEBUG, "Sending a message of type LoginResponse (1005)\r\n");
        INDENT_LOG(Logging::DEBUG);

                 // If logged in OK, this is the time to create a scripting logic entity for the client. Also adds to internal FPSClient
        if (success)
            server::createScriptingEntity(clientNumber);


        int start, finish;
        if (clientNumber == -1)
        {
            // Send to all clients
            start  = 0;
            finish = getnumclients() - 1;
        } else {
            start  = clientNumber;
            finish = clientNumber;
        }

#ifdef SERVER
        int testUniqueId;
#endif
        for (clientNumber = start; clientNumber <= finish; clientNumber++)
        {
            if (clientNumber == exclude) continue;
#ifdef SERVER
            fpsent* fpsEntity = dynamic_cast<fpsent*>( FPSClientInterface::getPlayerByNumber(clientNumber) );
            bool serverControlled = fpsEntity ? fpsEntity->serverControlled : false;

            testUniqueId = FPSServerInterface::getUniqueId(clientNumber);
            if ( (!serverControlled && testUniqueId != DUMMY_SINGLETON_CLIENT_UNIQUE_ID) || // If a remote client, send even if negative (during login process)
                 (false && testUniqueId == DUMMY_SINGLETON_CLIENT_UNIQUE_ID) || // If need to send to dummy server, send there
                 (false && testUniqueId != DUMMY_SINGLETON_CLIENT_UNIQUE_ID && serverControlled) )  // If need to send to npcs, send there
#endif
            {
                #ifdef SERVER
                    Logging::log(Logging::DEBUG, "Sending to %d (%d) ((%d))\r\n", clientNumber, testUniqueId, serverControlled);
                #endif
                sendf(clientNumber, MAIN_CHANNEL, "riii", 1005, success, local);

            }
        }
    }

#ifdef CLIENT
    void LoginResponse::receive(int receiver, int sender, ucharbuf &p)
    {
        bool is_npc;
        is_npc = false;
        Logging::log(Logging::DEBUG, "MessageSystem: Receiving a message of type LoginResponse (1005)\r\n");

        bool success = getint(p);
        bool local = getint(p);

        if (success)
        {
            ClientSystem::finishLogin(local); // This player will be known as 'uniqueID' in the current module
            conoutf("Login was successful.\r\n");
            send_RequestCurrentScenario();
        } else {
            conoutf("Login failure. Please check your username and password.\r\n");
            disconnect();
        }
    }
#endif


// PrepareForNewScenario

    void send_PrepareForNewScenario(int clientNumber, std::string scenarioCode)
    {
        int exclude = -1; // Set this to clientNumber to not send to

        Logging::log(Logging::DEBUG, "Sending a message of type PrepareForNewScenario (1006)\r\n");
        INDENT_LOG(Logging::DEBUG);

         

        int start, finish;
        if (clientNumber == -1)
        {
            // Send to all clients
            start  = 0;
            finish = getnumclients() - 1;
        } else {
            start  = clientNumber;
            finish = clientNumber;
        }

#ifdef SERVER
        int testUniqueId;
#endif
        for (clientNumber = start; clientNumber <= finish; clientNumber++)
        {
            if (clientNumber == exclude) continue;
#ifdef SERVER
            fpsent* fpsEntity = dynamic_cast<fpsent*>( FPSClientInterface::getPlayerByNumber(clientNumber) );
            bool serverControlled = fpsEntity ? fpsEntity->serverControlled : false;

            testUniqueId = FPSServerInterface::getUniqueId(clientNumber);
            if ( (!serverControlled && testUniqueId != DUMMY_SINGLETON_CLIENT_UNIQUE_ID) || // If a remote client, send even if negative (during login process)
                 (false && testUniqueId == DUMMY_SINGLETON_CLIENT_UNIQUE_ID) || // If need to send to dummy server, send there
                 (false && testUniqueId != DUMMY_SINGLETON_CLIENT_UNIQUE_ID && serverControlled) )  // If need to send to npcs, send there
#endif
            {
                #ifdef SERVER
                    Logging::log(Logging::DEBUG, "Sending to %d (%d) ((%d))\r\n", clientNumber, testUniqueId, serverControlled);
                #endif
                sendf(clientNumber, MAIN_CHANNEL, "ris", 1006, scenarioCode.c_str());

            }
        }
    }

#ifdef CLIENT
    void PrepareForNewScenario::receive(int receiver, int sender, ucharbuf &p)
    {
        bool is_npc;
        is_npc = false;
        Logging::log(Logging::DEBUG, "MessageSystem: Receiving a message of type PrepareForNewScenario (1006)\r\n");

        char tmp_scenarioCode[MAXTRANS];
        getstring(tmp_scenarioCode, p);
        std::string scenarioCode = tmp_scenarioCode;

        IntensityGUI::showMessage("Server", "Map being prepared on server, please wait");
        ClientSystem::prepareForNewScenario(scenarioCode);
    }
#endif


// RequestCurrentScenario

    void send_RequestCurrentScenario()
    {
        Logging::log(Logging::DEBUG, "Sending a message of type RequestCurrentScenario (1007)\r\n");
        INDENT_LOG(Logging::DEBUG);

        game::addmsg(1007, "r");
    }

#ifdef SERVER
    void RequestCurrentScenario::receive(int receiver, int sender, ucharbuf &p)
    {
        Logging::log(Logging::DEBUG, "MessageSystem: Receiving a message of type RequestCurrentScenario (1007)\r\n");


        if (!ServerSystem::isRunningMap()) return;
        REFLECT_PYTHON( send_curr_map );
        send_curr_map(sender);
    }
#endif

// NotifyAboutCurrentScenario

    void send_NotifyAboutCurrentScenario(int clientNumber, std::string mapAssetId, std::string scenarioCode)
    {
        int exclude = -1; // Set this to clientNumber to not send to

        Logging::log(Logging::DEBUG, "Sending a message of type NotifyAboutCurrentScenario (1008)\r\n");
        INDENT_LOG(Logging::DEBUG);

         

        int start, finish;
        if (clientNumber == -1)
        {
            // Send to all clients
            start  = 0;
            finish = getnumclients() - 1;
        } else {
            start  = clientNumber;
            finish = clientNumber;
        }

#ifdef SERVER
        int testUniqueId;
#endif
        for (clientNumber = start; clientNumber <= finish; clientNumber++)
        {
            if (clientNumber == exclude) continue;
#ifdef SERVER
            fpsent* fpsEntity = dynamic_cast<fpsent*>( FPSClientInterface::getPlayerByNumber(clientNumber) );
            bool serverControlled = fpsEntity ? fpsEntity->serverControlled : false;

            testUniqueId = FPSServerInterface::getUniqueId(clientNumber);
            if ( (!serverControlled && testUniqueId != DUMMY_SINGLETON_CLIENT_UNIQUE_ID) || // If a remote client, send even if negative (during login process)
                 (false && testUniqueId == DUMMY_SINGLETON_CLIENT_UNIQUE_ID) || // If need to send to dummy server, send there
                 (false && testUniqueId != DUMMY_SINGLETON_CLIENT_UNIQUE_ID && serverControlled) )  // If need to send to npcs, send there
#endif
            {
                #ifdef SERVER
                    Logging::log(Logging::DEBUG, "Sending to %d (%d) ((%d))\r\n", clientNumber, testUniqueId, serverControlled);
                #endif
                sendf(clientNumber, MAIN_CHANNEL, "riss", 1008, mapAssetId.c_str(), scenarioCode.c_str());

            }
        }
    }

#ifdef CLIENT
    void NotifyAboutCurrentScenario::receive(int receiver, int sender, ucharbuf &p)
    {
        bool is_npc;
        is_npc = false;
        Logging::log(Logging::DEBUG, "MessageSystem: Receiving a message of type NotifyAboutCurrentScenario (1008)\r\n");

        char tmp_mapAssetId[MAXTRANS];
        getstring(tmp_mapAssetId, p);
        std::string mapAssetId = tmp_mapAssetId;
        char tmp_scenarioCode[MAXTRANS];
        getstring(tmp_scenarioCode, p);
        std::string scenarioCode = tmp_scenarioCode;

        ClientSystem::currScenarioCode = scenarioCode;
        REFLECT_PYTHON( AssetManager );
        REFLECT_PYTHON( set_map );
        AssetManager.attr("clear_cache")();
        set_map("", mapAssetId);
    }
#endif


// RestartMap

    void send_RestartMap()
    {
        Logging::log(Logging::DEBUG, "Sending a message of type RestartMap (1009)\r\n");
        INDENT_LOG(Logging::DEBUG);

        game::addmsg(1009, "r");
    }

#ifdef SERVER
    void RestartMap::receive(int receiver, int sender, ucharbuf &p)
    {
        Logging::log(Logging::DEBUG, "MessageSystem: Receiving a message of type RestartMap (1009)\r\n");


        if (!ServerSystem::isRunningMap()) return;
        if (!server::isAdmin(sender))
        {
            Logging::log(Logging::WARNING, "Non-admin tried to restart the map\r\n");
            send_PersonalServerMessage(sender, -1, "Server", "You are not an administrator, and cannot restart the map");
            return;
        }
        REFLECT_PYTHON( restart_map );
        restart_map();
    }
#endif

// NewEntityRequest

    void send_NewEntityRequest(std::string _class, float x, float y, float z, std::string stateData)
    {        EditingSystem::madeChanges = true;

        Logging::log(Logging::DEBUG, "Sending a message of type NewEntityRequest (1010)\r\n");
        INDENT_LOG(Logging::DEBUG);

        game::addmsg(1010, "rsiiis", _class.c_str(), int(x*DMF), int(y*DMF), int(z*DMF), stateData.c_str());
    }

#ifdef SERVER
    void NewEntityRequest::receive(int receiver, int sender, ucharbuf &p)
    {
        Logging::log(Logging::DEBUG, "MessageSystem: Receiving a message of type NewEntityRequest (1010)\r\n");

        char tmp__class[MAXTRANS];
        getstring(tmp__class, p);
        std::string _class = tmp__class;
        float x = float(getint(p))/DMF;
        float y = float(getint(p))/DMF;
        float z = float(getint(p))/DMF;
        char tmp_stateData[MAXTRANS];
        getstring(tmp_stateData, p);
        std::string stateData = tmp_stateData;

        if (!ServerSystem::isRunningMap()) return;
        if (!server::isAdmin(sender))
        {
            Logging::log(Logging::WARNING, "Non-admin tried to add an entity\r\n");
            send_PersonalServerMessage(sender, -1, "Server", "You are not an administrator, and cannot create entities");
            return;
        }
        // Validate class
        if (!EditingSystem::validateEntityClass(_class))
        {
            Logging::log(Logging::WARNING, "User tried to add an invalid entity: %s\r\n", _class.c_str());
            send_PersonalServerMessage(
                sender,
                -1,
                "Invalid entity class: " + _class,
                "Reminder: Create entities using F8, not /newent. See the wiki for more."
            );
            return;
        }
        // Add entity
        Logging::log(Logging::DEBUG, "Creating new entity, %s   %f,%f,%f   %s\r\n", _class.c_str(), x, y, z, stateData.c_str());
        if ( !server::isRunningCurrentScenario(sender) ) return; // Silently ignore info from previous scenario
        std::string sauerType = ScriptEngineManager::getGlobal()->call("getEntitySauerType", _class)->getString();
        Logging::log(Logging::DEBUG, "Sauer type: %s\r\n", sauerType.c_str());
        python::list params;
        if (sauerType != "dynent")
            params.append(findtype((char*)sauerType.c_str()));
        params.append(x);
        params.append(y);
        params.append(z);
        // Create
        ScriptValuePtr kwargs = ScriptEngineManager::createScriptObject();
        ScriptValuePtr position = ScriptEngineManager::createScriptObject();
        kwargs->setProperty("position", position);
        position->setProperty("x", x);
        position->setProperty("y", y);
        position->setProperty("z", z);
        kwargs->setProperty("stateData", stateData);
        ScriptValuePtr scriptEntity = ScriptEngineManager::getGlobal()->call("newEntity",
            ScriptValueArgs().append(_class).append(kwargs)
        );
        int newUniqueId = scriptEntity->getPropertyInt("uniqueId");
        Logging::log(Logging::DEBUG, "Created Entity: %d - %s  (%f,%f,%f) \r\n",
                                      newUniqueId, _class.c_str(), x, y, z);
    }
#endif

// StateDataUpdate

    void send_StateDataUpdate(int clientNumber, int uniqueId, int keyProtocolId, std::string value, int originalClientNumber)
    {
        int exclude = -1; // Set this to clientNumber to not send to

        Logging::log(Logging::DEBUG, "Sending a message of type StateDataUpdate (1011)\r\n");
        INDENT_LOG(Logging::DEBUG);

                 exclude = originalClientNumber;


        int start, finish;
        if (clientNumber == -1)
        {
            // Send to all clients
            start  = 0;
            finish = getnumclients() - 1;
        } else {
            start  = clientNumber;
            finish = clientNumber;
        }

#ifdef SERVER
        int testUniqueId;
#endif
        for (clientNumber = start; clientNumber <= finish; clientNumber++)
        {
            if (clientNumber == exclude) continue;
#ifdef SERVER
            fpsent* fpsEntity = dynamic_cast<fpsent*>( FPSClientInterface::getPlayerByNumber(clientNumber) );
            bool serverControlled = fpsEntity ? fpsEntity->serverControlled : false;

            testUniqueId = FPSServerInterface::getUniqueId(clientNumber);
            if ( (!serverControlled && testUniqueId != DUMMY_SINGLETON_CLIENT_UNIQUE_ID) || // If a remote client, send even if negative (during login process)
                 (false && testUniqueId == DUMMY_SINGLETON_CLIENT_UNIQUE_ID) || // If need to send to dummy server, send there
                 (true && testUniqueId != DUMMY_SINGLETON_CLIENT_UNIQUE_ID && serverControlled) )  // If need to send to npcs, send there
#endif
            {
                #ifdef SERVER
                    Logging::log(Logging::DEBUG, "Sending to %d (%d) ((%d))\r\n", clientNumber, testUniqueId, serverControlled);
                #endif
                sendf(clientNumber, MAIN_CHANNEL, "riiisi", 1011, uniqueId, keyProtocolId, value.c_str(), originalClientNumber);

            }
        }
    }

    void StateDataUpdate::receive(int receiver, int sender, ucharbuf &p)
    {
        bool is_npc;
#ifdef CLIENT
        is_npc = false;
#else // SERVER
        is_npc = true;
#endif
        Logging::log(Logging::DEBUG, "MessageSystem: Receiving a message of type StateDataUpdate (1011)\r\n");

        int uniqueId = getint(p);
        int keyProtocolId = getint(p);
        char tmp_value[MAXTRANS];
        getstring(tmp_value, p);
        std::string value = tmp_value;
        int originalClientNumber = getint(p);

        #ifdef SERVER
            #define STATE_DATA_UPDATE \
                uniqueId = uniqueId;  /* Prevent warnings */ \
                keyProtocolId = keyProtocolId; \
                originalClientNumber = originalClientNumber; \
                return; /* We do send this to the NPCs sometimes, as it is sent during their creation (before they are fully */ \
                        /* registered even). But we have no need to process it on the server. */
        #else
            #define STATE_DATA_UPDATE \
                assert(originalClientNumber == -1 || ClientSystem::playerNumber != originalClientNumber); /* Can be -1, or else cannot be us */ \
                \
                Logging::log(Logging::DEBUG, "StateDataUpdate: %d, %d, %s \r\n", uniqueId, keyProtocolId, value.c_str()); \
                \
                if (!ScriptEngineManager::hasEngine()) \
                    return; \
                \
                ScriptEngineManager::getGlobal()->call("setStateDatum", \
                    ScriptValueArgs().append(uniqueId).append(keyProtocolId).append(value) \
                );
        #endif
        STATE_DATA_UPDATE
    }


// StateDataChangeRequest

    void send_StateDataChangeRequest(int uniqueId, int keyProtocolId, std::string value)
    {        // This isn't a perfect way to differentiate transient state data changes from permanent ones
        // that justify saying 'changes were made', but for now it will do. Note that even checking
        // for changes to persistent entities is not enough - transient changes on them are generally
        // not expected to count as 'changes'. So this check, of editmode, is the best simple solution
        // there is - if you're in edit mode, the change counts as a 'real change', that you probably
        // want saved.
        // Note: We don't do this with unreliable messages, meaningless anyhow.
        if (editmode)
            EditingSystem::madeChanges = true;

        Logging::log(Logging::DEBUG, "Sending a message of type StateDataChangeRequest (1012)\r\n");
        INDENT_LOG(Logging::DEBUG);

        game::addmsg(1012, "riis", uniqueId, keyProtocolId, value.c_str());
    }

#ifdef SERVER
    void StateDataChangeRequest::receive(int receiver, int sender, ucharbuf &p)
    {
        Logging::log(Logging::DEBUG, "MessageSystem: Receiving a message of type StateDataChangeRequest (1012)\r\n");

        int uniqueId = getint(p);
        int keyProtocolId = getint(p);
        char tmp_value[MAXTRANS];
        getstring(tmp_value, p);
        std::string value = tmp_value;

        if (!ServerSystem::isRunningMap()) return;
        #define STATE_DATA_REQUEST \
        int actorUniqueId = FPSServerInterface::getUniqueId(sender); \
        \
        Logging::log(Logging::DEBUG, "client %d requests to change %d to value: %s\r\n", actorUniqueId, keyProtocolId, value.c_str()); \
        \
        if ( !server::isRunningCurrentScenario(sender) ) return; /* Silently ignore info from previous scenario */ \
        \
        ScriptEngineManager::getGlobal()->call("setStateDatum", \
            ScriptValueArgs().append(uniqueId).append(keyProtocolId).append(value).append(actorUniqueId) \
        );
        STATE_DATA_REQUEST
    }
#endif

// UnreliableStateDataUpdate

    void send_UnreliableStateDataUpdate(int clientNumber, int uniqueId, int keyProtocolId, std::string value, int originalClientNumber)
    {
        int exclude = -1; // Set this to clientNumber to not send to

        Logging::log(Logging::DEBUG, "Sending a message of type UnreliableStateDataUpdate (1013)\r\n");
        INDENT_LOG(Logging::DEBUG);

                 exclude = originalClientNumber;


        int start, finish;
        if (clientNumber == -1)
        {
            // Send to all clients
            start  = 0;
            finish = getnumclients() - 1;
        } else {
            start  = clientNumber;
            finish = clientNumber;
        }

#ifdef SERVER
        int testUniqueId;
#endif
        for (clientNumber = start; clientNumber <= finish; clientNumber++)
        {
            if (clientNumber == exclude) continue;
#ifdef SERVER
            fpsent* fpsEntity = dynamic_cast<fpsent*>( FPSClientInterface::getPlayerByNumber(clientNumber) );
            bool serverControlled = fpsEntity ? fpsEntity->serverControlled : false;

            testUniqueId = FPSServerInterface::getUniqueId(clientNumber);
            if ( (!serverControlled && testUniqueId != DUMMY_SINGLETON_CLIENT_UNIQUE_ID) || // If a remote client, send even if negative (during login process)
                 (false && testUniqueId == DUMMY_SINGLETON_CLIENT_UNIQUE_ID) || // If need to send to dummy server, send there
                 (true && testUniqueId != DUMMY_SINGLETON_CLIENT_UNIQUE_ID && serverControlled) )  // If need to send to npcs, send there
#endif
            {
                #ifdef SERVER
                    Logging::log(Logging::DEBUG, "Sending to %d (%d) ((%d))\r\n", clientNumber, testUniqueId, serverControlled);
                #endif
                sendf(clientNumber, MAIN_CHANNEL, "iiisi", 1013, uniqueId, keyProtocolId, value.c_str(), originalClientNumber);

            }
        }
    }

    void UnreliableStateDataUpdate::receive(int receiver, int sender, ucharbuf &p)
    {
        bool is_npc;
#ifdef CLIENT
        is_npc = false;
#else // SERVER
        is_npc = true;
#endif
        Logging::log(Logging::DEBUG, "MessageSystem: Receiving a message of type UnreliableStateDataUpdate (1013)\r\n");

        int uniqueId = getint(p);
        int keyProtocolId = getint(p);
        char tmp_value[MAXTRANS];
        getstring(tmp_value, p);
        std::string value = tmp_value;
        int originalClientNumber = getint(p);

        STATE_DATA_UPDATE
    }


// UnreliableStateDataChangeRequest

    void send_UnreliableStateDataChangeRequest(int uniqueId, int keyProtocolId, std::string value)
    {
        Logging::log(Logging::DEBUG, "Sending a message of type UnreliableStateDataChangeRequest (1014)\r\n");
        INDENT_LOG(Logging::DEBUG);

        game::addmsg(1014, "iis", uniqueId, keyProtocolId, value.c_str());
    }

#ifdef SERVER
    void UnreliableStateDataChangeRequest::receive(int receiver, int sender, ucharbuf &p)
    {
        Logging::log(Logging::DEBUG, "MessageSystem: Receiving a message of type UnreliableStateDataChangeRequest (1014)\r\n");

        int uniqueId = getint(p);
        int keyProtocolId = getint(p);
        char tmp_value[MAXTRANS];
        getstring(tmp_value, p);
        std::string value = tmp_value;

        if (!ServerSystem::isRunningMap()) return;
        STATE_DATA_REQUEST
    }
#endif

// NotifyNumEntities

    void send_NotifyNumEntities(int clientNumber, int num)
    {
        int exclude = -1; // Set this to clientNumber to not send to

        Logging::log(Logging::DEBUG, "Sending a message of type NotifyNumEntities (1015)\r\n");
        INDENT_LOG(Logging::DEBUG);

         

        int start, finish;
        if (clientNumber == -1)
        {
            // Send to all clients
            start  = 0;
            finish = getnumclients() - 1;
        } else {
            start  = clientNumber;
            finish = clientNumber;
        }

#ifdef SERVER
        int testUniqueId;
#endif
        for (clientNumber = start; clientNumber <= finish; clientNumber++)
        {
            if (clientNumber == exclude) continue;
#ifdef SERVER
            fpsent* fpsEntity = dynamic_cast<fpsent*>( FPSClientInterface::getPlayerByNumber(clientNumber) );
            bool serverControlled = fpsEntity ? fpsEntity->serverControlled : false;

            testUniqueId = FPSServerInterface::getUniqueId(clientNumber);
            if ( (!serverControlled && testUniqueId != DUMMY_SINGLETON_CLIENT_UNIQUE_ID) || // If a remote client, send even if negative (during login process)
                 (false && testUniqueId == DUMMY_SINGLETON_CLIENT_UNIQUE_ID) || // If need to send to dummy server, send there
                 (false && testUniqueId != DUMMY_SINGLETON_CLIENT_UNIQUE_ID && serverControlled) )  // If need to send to npcs, send there
#endif
            {
                #ifdef SERVER
                    Logging::log(Logging::DEBUG, "Sending to %d (%d) ((%d))\r\n", clientNumber, testUniqueId, serverControlled);
                #endif
                sendf(clientNumber, MAIN_CHANNEL, "rii", 1015, num);

            }
        }
    }

#ifdef CLIENT
    void NotifyNumEntities::receive(int receiver, int sender, ucharbuf &p)
    {
        bool is_npc;
        is_npc = false;
        Logging::log(Logging::DEBUG, "MessageSystem: Receiving a message of type NotifyNumEntities (1015)\r\n");

        int num = getint(p);

        WorldSystem::setNumExpectedEntities(num);
    }
#endif


// AllActiveEntitiesSent

    void send_AllActiveEntitiesSent(int clientNumber)
    {
        int exclude = -1; // Set this to clientNumber to not send to

        Logging::log(Logging::DEBUG, "Sending a message of type AllActiveEntitiesSent (1016)\r\n");
        INDENT_LOG(Logging::DEBUG);

         

        int start, finish;
        if (clientNumber == -1)
        {
            // Send to all clients
            start  = 0;
            finish = getnumclients() - 1;
        } else {
            start  = clientNumber;
            finish = clientNumber;
        }

#ifdef SERVER
        int testUniqueId;
#endif
        for (clientNumber = start; clientNumber <= finish; clientNumber++)
        {
            if (clientNumber == exclude) continue;
#ifdef SERVER
            fpsent* fpsEntity = dynamic_cast<fpsent*>( FPSClientInterface::getPlayerByNumber(clientNumber) );
            bool serverControlled = fpsEntity ? fpsEntity->serverControlled : false;

            testUniqueId = FPSServerInterface::getUniqueId(clientNumber);
            if ( (!serverControlled && testUniqueId != DUMMY_SINGLETON_CLIENT_UNIQUE_ID) || // If a remote client, send even if negative (during login process)
                 (false && testUniqueId == DUMMY_SINGLETON_CLIENT_UNIQUE_ID) || // If need to send to dummy server, send there
                 (false && testUniqueId != DUMMY_SINGLETON_CLIENT_UNIQUE_ID && serverControlled) )  // If need to send to npcs, send there
#endif
            {
                #ifdef SERVER
                    Logging::log(Logging::DEBUG, "Sending to %d (%d) ((%d))\r\n", clientNumber, testUniqueId, serverControlled);
                #endif
                sendf(clientNumber, MAIN_CHANNEL, "ri", 1016);

            }
        }
    }

#ifdef CLIENT
    void AllActiveEntitiesSent::receive(int receiver, int sender, ucharbuf &p)
    {
        bool is_npc;
        is_npc = false;
        Logging::log(Logging::DEBUG, "MessageSystem: Receiving a message of type AllActiveEntitiesSent (1016)\r\n");


        ClientSystem::finishLoadWorld();
    }
#endif


// ActiveEntitiesRequest

    void send_ActiveEntitiesRequest(std::string scenarioCode)
    {
        Logging::log(Logging::DEBUG, "Sending a message of type ActiveEntitiesRequest (1017)\r\n");
        INDENT_LOG(Logging::DEBUG);

        game::addmsg(1017, "rs", scenarioCode.c_str());
    }

#ifdef SERVER
    void ActiveEntitiesRequest::receive(int receiver, int sender, ucharbuf &p)
    {
        Logging::log(Logging::DEBUG, "MessageSystem: Receiving a message of type ActiveEntitiesRequest (1017)\r\n");

        char tmp_scenarioCode[MAXTRANS];
        getstring(tmp_scenarioCode, p);
        std::string scenarioCode = tmp_scenarioCode;

        #ifdef SERVER
            if (!ServerSystem::isRunningMap()) return;
            // Mark the client as running the current scenario, if indeed doing so
            server::setClientScenario(sender, scenarioCode);
            if ( !server::isRunningCurrentScenario(sender) )
            {
                Logging::log(Logging::WARNING, "Client %d requested active entities for an invalid scenario: %s\r\n",
                    sender, scenarioCode.c_str()
                );
                send_PersonalServerMessage(sender, -1, "Invalid scenario", "An error occured in synchronizing scenarios");
                return;
            }
            ScriptEngineManager::getGlobal()->call("sendEntities", sender);
            MessageSystem::send_AllActiveEntitiesSent(sender);
            ScriptEngineManager::getGlobal()->getProperty("ApplicationManager")->getProperty("instance")->call(
                "onPlayerLogin",
                ScriptEngineManager::getGlobal()->call("getEntity", FPSServerInterface::getUniqueId(sender))
            );
        #else // CLIENT
            // Send just enough info for the player's LE
            send_LogicEntityCompleteNotification( sender,
                                                  sender,
                                                  9999, // TODO: this same constant appears in multiple places
                                                  "Player",
                                                  "{}" );
            MessageSystem::send_AllActiveEntitiesSent(sender);
        #endif
    }
#endif

// LogicEntityCompleteNotification

    void send_LogicEntityCompleteNotification(int clientNumber, int otherClientNumber, int otherUniqueId, std::string otherClass, std::string stateData)
    {
        int exclude = -1; // Set this to clientNumber to not send to

        Logging::log(Logging::DEBUG, "Sending a message of type LogicEntityCompleteNotification (1018)\r\n");
        INDENT_LOG(Logging::DEBUG);

         

        int start, finish;
        if (clientNumber == -1)
        {
            // Send to all clients
            start  = 0;
            finish = getnumclients() - 1;
        } else {
            start  = clientNumber;
            finish = clientNumber;
        }

#ifdef SERVER
        int testUniqueId;
#endif
        for (clientNumber = start; clientNumber <= finish; clientNumber++)
        {
            if (clientNumber == exclude) continue;
#ifdef SERVER
            fpsent* fpsEntity = dynamic_cast<fpsent*>( FPSClientInterface::getPlayerByNumber(clientNumber) );
            bool serverControlled = fpsEntity ? fpsEntity->serverControlled : false;

            testUniqueId = FPSServerInterface::getUniqueId(clientNumber);
            if ( (!serverControlled && testUniqueId != DUMMY_SINGLETON_CLIENT_UNIQUE_ID) || // If a remote client, send even if negative (during login process)
                 (false && testUniqueId == DUMMY_SINGLETON_CLIENT_UNIQUE_ID) || // If need to send to dummy server, send there
                 (true && testUniqueId != DUMMY_SINGLETON_CLIENT_UNIQUE_ID && serverControlled) )  // If need to send to npcs, send there
#endif
            {
                #ifdef SERVER
                    Logging::log(Logging::DEBUG, "Sending to %d (%d) ((%d))\r\n", clientNumber, testUniqueId, serverControlled);
                #endif
                sendf(clientNumber, MAIN_CHANNEL, "riiiss", 1018, otherClientNumber, otherUniqueId, otherClass.c_str(), stateData.c_str());

            }
        }
    }

    void LogicEntityCompleteNotification::receive(int receiver, int sender, ucharbuf &p)
    {
        bool is_npc;
#ifdef CLIENT
        is_npc = false;
#else // SERVER
        is_npc = true;
#endif
        Logging::log(Logging::DEBUG, "MessageSystem: Receiving a message of type LogicEntityCompleteNotification (1018)\r\n");

        int otherClientNumber = getint(p);
        int otherUniqueId = getint(p);
        char tmp_otherClass[MAXTRANS];
        getstring(tmp_otherClass, p);
        std::string otherClass = tmp_otherClass;
        char tmp_stateData[MAXTRANS];
        getstring(tmp_stateData, p);
        std::string stateData = tmp_stateData;

        #ifdef SERVER
            return; // We do send this to the NPCs sometimes, as it is sent during their creation (before they are fully
                    // registered even). But we have no need to process it on the server.
        #endif
        if (!ScriptEngineManager::hasEngine())
            return;
        Logging::log(Logging::DEBUG, "RECEIVING LE: %d,%d,%s\r\n", otherClientNumber, otherUniqueId, otherClass.c_str());
        INDENT_LOG(Logging::DEBUG);
        // If a logic entity does not yet exist, create one
        LogicEntityPtr entity = LogicSystem::getLogicEntity(otherUniqueId);
        if (entity.get() == NULL)
        {
            Logging::log(Logging::DEBUG, "Creating new active LogicEntity\r\n");
            ScriptValuePtr kwargs = ScriptEngineManager::createScriptObject();
            if (otherClientNumber >= 0) // If this is another client, NPC, etc., then send the clientnumber, critical for setup
            {
                #ifdef CLIENT
                    // If this is the player, validate it is the clientNumber we already have
                    if (otherUniqueId == ClientSystem::uniqueId)
                    {
                        Logging::log(Logging::DEBUG, "This is the player's entity (%d), validating client num: %d,%d\r\n",
                            otherUniqueId, otherClientNumber, ClientSystem::playerNumber);
                        assert(otherClientNumber == ClientSystem::playerNumber);
                    }
                #endif
                kwargs->setProperty("clientNumber", otherClientNumber);
            }
            ScriptEngineManager::getGlobal()->call("addEntity",
                ScriptValueArgs().append(otherClass).append(otherUniqueId).append(kwargs)
            );
            entity = LogicSystem::getLogicEntity(otherUniqueId);
            if (!entity.get())
            {
                Logging::log(Logging::ERROR, "Received a LogicEntityCompleteNotification for a LogicEntity that cannot be created: %d - %s. Ignoring\r\n", otherUniqueId, otherClass.c_str());
                return;
            }
        } else
            Logging::log(Logging::DEBUG, "Existing LogicEntity %d,%d,%d, no need to create\r\n", entity.get() != NULL, entity->getUniqueId(),
                                            otherUniqueId);
        // A logic entity now exists (either one did before, or we created one), we now update the stateData, if we
        // are remotely connected (TODO: make this not segfault for localconnect)
        Logging::log(Logging::DEBUG, "Updating stateData with: %s\r\n", stateData.c_str());
        ScriptValuePtr sd = ScriptEngineManager::createScriptValue(stateData);
        entity.get()->scriptEntity->call("_updateCompleteStateData", sd);
        #ifdef CLIENT
            // If this new entity is in fact the Player's entity, then we finally have the player's LE, and can link to it.
            if (otherUniqueId == ClientSystem::uniqueId)
            {
                Logging::log(Logging::DEBUG, "Linking player information, uid: %d\r\n", otherUniqueId);
                // Note in C++
                ClientSystem::playerLogicEntity = LogicSystem::getLogicEntity(ClientSystem::uniqueId);
                // Note in Scripting
                ScriptEngineManager::getGlobal()->call("setPlayerUniqueId",ClientSystem::uniqueId);
            }
        #endif
        // Events post-reception
        WorldSystem::triggerReceivedEntity();
    }


// RequestLogicEntityRemoval

    void send_RequestLogicEntityRemoval(int uniqueId)
    {        EditingSystem::madeChanges = true;

        Logging::log(Logging::DEBUG, "Sending a message of type RequestLogicEntityRemoval (1019)\r\n");
        INDENT_LOG(Logging::DEBUG);

        game::addmsg(1019, "ri", uniqueId);
    }

#ifdef SERVER
    void RequestLogicEntityRemoval::receive(int receiver, int sender, ucharbuf &p)
    {
        Logging::log(Logging::DEBUG, "MessageSystem: Receiving a message of type RequestLogicEntityRemoval (1019)\r\n");

        int uniqueId = getint(p);

        if (!ServerSystem::isRunningMap()) return;
        if (!server::isAdmin(sender))
        {
            Logging::log(Logging::WARNING, "Non-admin tried to remove an entity\r\n");
            send_PersonalServerMessage(sender, -1, "Server", "You are not an administrator, and cannot remove entities");
            return;
        }
        if ( !server::isRunningCurrentScenario(sender) ) return; // Silently ignore info from previous scenario
        ScriptEngineManager::getGlobal()->call("removeEntity", uniqueId);
    }
#endif

// LogicEntityRemoval

    void send_LogicEntityRemoval(int clientNumber, int uniqueId)
    {
        int exclude = -1; // Set this to clientNumber to not send to

        Logging::log(Logging::DEBUG, "Sending a message of type LogicEntityRemoval (1020)\r\n");
        INDENT_LOG(Logging::DEBUG);

         

        int start, finish;
        if (clientNumber == -1)
        {
            // Send to all clients
            start  = 0;
            finish = getnumclients() - 1;
        } else {
            start  = clientNumber;
            finish = clientNumber;
        }

#ifdef SERVER
        int testUniqueId;
#endif
        for (clientNumber = start; clientNumber <= finish; clientNumber++)
        {
            if (clientNumber == exclude) continue;
#ifdef SERVER
            fpsent* fpsEntity = dynamic_cast<fpsent*>( FPSClientInterface::getPlayerByNumber(clientNumber) );
            bool serverControlled = fpsEntity ? fpsEntity->serverControlled : false;

            testUniqueId = FPSServerInterface::getUniqueId(clientNumber);
            if ( (!serverControlled && testUniqueId != DUMMY_SINGLETON_CLIENT_UNIQUE_ID) || // If a remote client, send even if negative (during login process)
                 (false && testUniqueId == DUMMY_SINGLETON_CLIENT_UNIQUE_ID) || // If need to send to dummy server, send there
                 (false && testUniqueId != DUMMY_SINGLETON_CLIENT_UNIQUE_ID && serverControlled) )  // If need to send to npcs, send there
#endif
            {
                #ifdef SERVER
                    Logging::log(Logging::DEBUG, "Sending to %d (%d) ((%d))\r\n", clientNumber, testUniqueId, serverControlled);
                #endif
                sendf(clientNumber, MAIN_CHANNEL, "rii", 1020, uniqueId);

            }
        }
    }

#ifdef CLIENT
    void LogicEntityRemoval::receive(int receiver, int sender, ucharbuf &p)
    {
        bool is_npc;
        is_npc = false;
        Logging::log(Logging::DEBUG, "MessageSystem: Receiving a message of type LogicEntityRemoval (1020)\r\n");

        int uniqueId = getint(p);

        if (!ScriptEngineManager::hasEngine())
            return;
        ScriptEngineManager::getGlobal()->call("removeEntity", uniqueId);
    }
#endif


// ExtentCompleteNotification

    void send_ExtentCompleteNotification(int clientNumber, int otherUniqueId, std::string otherClass, std::string stateData, float x, float y, float z, int attr1, int attr2, int attr3, int attr4)
    {
        int exclude = -1; // Set this to clientNumber to not send to

        Logging::log(Logging::DEBUG, "Sending a message of type ExtentCompleteNotification (1021)\r\n");
        INDENT_LOG(Logging::DEBUG);

         

        int start, finish;
        if (clientNumber == -1)
        {
            // Send to all clients
            start  = 0;
            finish = getnumclients() - 1;
        } else {
            start  = clientNumber;
            finish = clientNumber;
        }

#ifdef SERVER
        int testUniqueId;
#endif
        for (clientNumber = start; clientNumber <= finish; clientNumber++)
        {
            if (clientNumber == exclude) continue;
#ifdef SERVER
            fpsent* fpsEntity = dynamic_cast<fpsent*>( FPSClientInterface::getPlayerByNumber(clientNumber) );
            bool serverControlled = fpsEntity ? fpsEntity->serverControlled : false;

            testUniqueId = FPSServerInterface::getUniqueId(clientNumber);
            if ( (!serverControlled && testUniqueId != DUMMY_SINGLETON_CLIENT_UNIQUE_ID) || // If a remote client, send even if negative (during login process)
                 (false && testUniqueId == DUMMY_SINGLETON_CLIENT_UNIQUE_ID) || // If need to send to dummy server, send there
                 (false && testUniqueId != DUMMY_SINGLETON_CLIENT_UNIQUE_ID && serverControlled) )  // If need to send to npcs, send there
#endif
            {
                #ifdef SERVER
                    Logging::log(Logging::DEBUG, "Sending to %d (%d) ((%d))\r\n", clientNumber, testUniqueId, serverControlled);
                #endif
                sendf(clientNumber, MAIN_CHANNEL, "riissiiiiiii", 1021, otherUniqueId, otherClass.c_str(), stateData.c_str(), int(x*DMF), int(y*DMF), int(z*DMF), attr1, attr2, attr3, attr4);

            }
        }
    }

#ifdef CLIENT
    void ExtentCompleteNotification::receive(int receiver, int sender, ucharbuf &p)
    {
        bool is_npc;
        is_npc = false;
        Logging::log(Logging::DEBUG, "MessageSystem: Receiving a message of type ExtentCompleteNotification (1021)\r\n");

        int otherUniqueId = getint(p);
        char tmp_otherClass[MAXTRANS];
        getstring(tmp_otherClass, p);
        std::string otherClass = tmp_otherClass;
        char tmp_stateData[MAXTRANS];
        getstring(tmp_stateData, p);
        std::string stateData = tmp_stateData;
        float x = float(getint(p))/DMF;
        float y = float(getint(p))/DMF;
        float z = float(getint(p))/DMF;
        int attr1 = getint(p);
        int attr2 = getint(p);
        int attr3 = getint(p);
        int attr4 = getint(p);

        if (!ScriptEngineManager::hasEngine())
            return;
        #if 0
        Something like this:
            extentity &e = *et->getents()[i];
            removeentity(i);
            int oldtype = e.type;
            if(oldtype!=type) detachentity(e);
            e.type = type;
            e.o = o;
            e.attr1 = attr1; e.attr2 = attr2; e.attr3 = attr3; e.attr4 = attr4;
            addentity(i);
        #endif
        Logging::log(Logging::DEBUG, "RECEIVING Extent: %d,%s - %f,%f,%f  %d,%d,%d\r\n", otherUniqueId, otherClass.c_str(),
            x, y, z, attr1, attr2, attr3, attr4);
        INDENT_LOG(Logging::DEBUG);
        // If a logic entity does not yet exist, create one
        LogicEntityPtr entity = LogicSystem::getLogicEntity(otherUniqueId);
        if (entity.get() == NULL)
        {
            Logging::log(Logging::DEBUG, "Creating new active LogicEntity\r\n");
            std::string sauerType = ScriptEngineManager::getGlobal()->call("getEntitySauerType", otherClass)->getString();
            ScriptValuePtr kwargs = ScriptEngineManager::createScriptObject();
            kwargs->setProperty("_type", findtype((char*)sauerType.c_str()));
            kwargs->setProperty("x", x);
            kwargs->setProperty("y", y);
            kwargs->setProperty("z", z);
            kwargs->setProperty("attr1", attr1);
            kwargs->setProperty("attr2", attr2);
            kwargs->setProperty("attr3", attr3);
            kwargs->setProperty("attr4", attr4);
            ScriptEngineManager::getGlobal()->call("addEntity",
                    ScriptValueArgs().append(otherClass).append(otherUniqueId).append(kwargs)
            );
            entity = LogicSystem::getLogicEntity(otherUniqueId);
            assert(entity.get() != NULL);
        } else
            Logging::log(Logging::DEBUG, "Existing LogicEntity %d,%d,%d, no need to create\r\n", entity.get() != NULL, entity->getUniqueId(),
                                            otherUniqueId);
        // A logic entity now exists (either one did before, or we created one), we now update the stateData, if we
        // are remotely connected (TODO: make this not segfault for localconnect)
        Logging::log(Logging::DEBUG, "Updating stateData\r\n");
        ScriptValuePtr sd = ScriptEngineManager::createScriptValue(stateData);
        entity.get()->scriptEntity->call("_updateCompleteStateData", sd);
        // Events post-reception
        WorldSystem::triggerReceivedEntity();
    }
#endif


// InitS2C

    void send_InitS2C(int clientNumber, int explicitClientNumber, int protocolVersion)
    {
        int exclude = -1; // Set this to clientNumber to not send to

        Logging::log(Logging::DEBUG, "Sending a message of type InitS2C (1022)\r\n");
        INDENT_LOG(Logging::DEBUG);

         

        int start, finish;
        if (clientNumber == -1)
        {
            // Send to all clients
            start  = 0;
            finish = getnumclients() - 1;
        } else {
            start  = clientNumber;
            finish = clientNumber;
        }

#ifdef SERVER
        int testUniqueId;
#endif
        for (clientNumber = start; clientNumber <= finish; clientNumber++)
        {
            if (clientNumber == exclude) continue;
#ifdef SERVER
            fpsent* fpsEntity = dynamic_cast<fpsent*>( FPSClientInterface::getPlayerByNumber(clientNumber) );
            bool serverControlled = fpsEntity ? fpsEntity->serverControlled : false;

            testUniqueId = FPSServerInterface::getUniqueId(clientNumber);
            if ( (!serverControlled && testUniqueId != DUMMY_SINGLETON_CLIENT_UNIQUE_ID) || // If a remote client, send even if negative (during login process)
                 (false && testUniqueId == DUMMY_SINGLETON_CLIENT_UNIQUE_ID) || // If need to send to dummy server, send there
                 (false && testUniqueId != DUMMY_SINGLETON_CLIENT_UNIQUE_ID && serverControlled) )  // If need to send to npcs, send there
#endif
            {
                #ifdef SERVER
                    Logging::log(Logging::DEBUG, "Sending to %d (%d) ((%d))\r\n", clientNumber, testUniqueId, serverControlled);
                #endif
                sendf(clientNumber, MAIN_CHANNEL, "riii", 1022, explicitClientNumber, protocolVersion);

            }
        }
    }

#ifdef CLIENT
    void InitS2C::receive(int receiver, int sender, ucharbuf &p)
    {
        bool is_npc;
        is_npc = false;
        Logging::log(Logging::DEBUG, "MessageSystem: Receiving a message of type InitS2C (1022)\r\n");

        int explicitClientNumber = getint(p);
        int protocolVersion = getint(p);

        if (!is_npc)
        {
            Logging::log(Logging::DEBUG, "client.h: SV_INITS2C gave us cn/protocol: %d/%d\r\n", explicitClientNumber, protocolVersion);
            if(protocolVersion != PROTOCOL_VERSION)
            {
                conoutf(CON_ERROR, "You are using a different network protocol (you: %d, server: %d)", PROTOCOL_VERSION, protocolVersion);
                disconnect();
                return;
            }
            fpsent *player1 = dynamic_cast<fpsent*>( FPSClientInterface::getClientPlayer() );
            player1->clientnum = explicitClientNumber; // we are now fully connected
                                                       // Kripken: Well, sauer would be, we still need more...
            #ifdef CLIENT
            ClientSystem::login(explicitClientNumber); // Finish the login process, send server our user/pass. NPCs need not do this.
            #endif
        } else {
            // NPC
            Logging::log(Logging::INFO, "client.h (npc): SV_INITS2C gave us cn/protocol: %d/%d\r\n", explicitClientNumber, protocolVersion);
            assert(0); //does this ever occur?
        }
    }
#endif


// MapVote

    void send_MapVote(std::string name)
    {
        Logging::log(Logging::DEBUG, "Sending a message of type MapVote (1023)\r\n");
        INDENT_LOG(Logging::DEBUG);

        game::addmsg(1023, "rs", name.c_str());
    }

#ifdef SERVER
    void MapVote::receive(int receiver, int sender, ucharbuf &p)
    {
        Logging::log(Logging::DEBUG, "MessageSystem: Receiving a message of type MapVote (1023)\r\n");

        char tmp_name[MAXTRANS];
        getstring(tmp_name, p);
        std::string name = tmp_name;

        assert(0); // DEPRECATED XXX
    }
#endif

// MapChange

    void send_MapChange(int clientNumber, std::string name)
    {
        int exclude = -1; // Set this to clientNumber to not send to

        Logging::log(Logging::DEBUG, "Sending a message of type MapChange (1024)\r\n");
        INDENT_LOG(Logging::DEBUG);

         

        int start, finish;
        if (clientNumber == -1)
        {
            // Send to all clients
            start  = 0;
            finish = getnumclients() - 1;
        } else {
            start  = clientNumber;
            finish = clientNumber;
        }

#ifdef SERVER
        int testUniqueId;
#endif
        for (clientNumber = start; clientNumber <= finish; clientNumber++)
        {
            if (clientNumber == exclude) continue;
#ifdef SERVER
            fpsent* fpsEntity = dynamic_cast<fpsent*>( FPSClientInterface::getPlayerByNumber(clientNumber) );
            bool serverControlled = fpsEntity ? fpsEntity->serverControlled : false;

            testUniqueId = FPSServerInterface::getUniqueId(clientNumber);
            if ( (!serverControlled && testUniqueId != DUMMY_SINGLETON_CLIENT_UNIQUE_ID) || // If a remote client, send even if negative (during login process)
                 (false && testUniqueId == DUMMY_SINGLETON_CLIENT_UNIQUE_ID) || // If need to send to dummy server, send there
                 (false && testUniqueId != DUMMY_SINGLETON_CLIENT_UNIQUE_ID && serverControlled) )  // If need to send to npcs, send there
#endif
            {
                #ifdef SERVER
                    Logging::log(Logging::DEBUG, "Sending to %d (%d) ((%d))\r\n", clientNumber, testUniqueId, serverControlled);
                #endif
                sendf(clientNumber, MAIN_CHANNEL, "ris", 1024, name.c_str());

            }
        }
    }

#ifdef CLIENT
    void MapChange::receive(int receiver, int sender, ucharbuf &p)
    {
        bool is_npc;
        is_npc = false;
        Logging::log(Logging::DEBUG, "MessageSystem: Receiving a message of type MapChange (1024)\r\n");

        char tmp_name[MAXTRANS];
        getstring(tmp_name, p);
        std::string name = tmp_name;

        // If loading the new map fails, return to login screen. TODO: Add error message in CEGUI
        if ( !load_world(name.c_str()) )
            ClientSystem::gotoLoginScreen();
    }
#endif


// SoundToServer

    void send_SoundToServer(int soundId)
    {
        Logging::log(Logging::DEBUG, "Sending a message of type SoundToServer (1025)\r\n");
        INDENT_LOG(Logging::DEBUG);

        game::addmsg(1025, "i", soundId);
    }

#ifdef SERVER
    void SoundToServer::receive(int receiver, int sender, ucharbuf &p)
    {
        Logging::log(Logging::DEBUG, "MessageSystem: Receiving a message of type SoundToServer (1025)\r\n");

        int soundId = getint(p);

        if (!ServerSystem::isRunningMap()) return;
        if ( !server::isRunningCurrentScenario(sender) ) return; // Silently ignore info from previous scenario
        dynent* otherEntity = FPSClientInterface::getPlayerByNumber(sender);
        if (otherEntity)
            send_SoundToClients(-1, soundId, sender);
    }
#endif

// SoundToClients

    void send_SoundToClients(int clientNumber, int soundId, int originalClientNumber)
    {
        int exclude = -1; // Set this to clientNumber to not send to

        Logging::log(Logging::DEBUG, "Sending a message of type SoundToClients (1026)\r\n");
        INDENT_LOG(Logging::DEBUG);

                 exclude = originalClientNumber; // This is how to ensure we do not send back to the client who originally sent it


        int start, finish;
        if (clientNumber == -1)
        {
            // Send to all clients
            start  = 0;
            finish = getnumclients() - 1;
        } else {
            start  = clientNumber;
            finish = clientNumber;
        }

#ifdef SERVER
        int testUniqueId;
#endif
        for (clientNumber = start; clientNumber <= finish; clientNumber++)
        {
            if (clientNumber == exclude) continue;
#ifdef SERVER
            fpsent* fpsEntity = dynamic_cast<fpsent*>( FPSClientInterface::getPlayerByNumber(clientNumber) );
            bool serverControlled = fpsEntity ? fpsEntity->serverControlled : false;

            testUniqueId = FPSServerInterface::getUniqueId(clientNumber);
            if ( (!serverControlled && testUniqueId != DUMMY_SINGLETON_CLIENT_UNIQUE_ID) || // If a remote client, send even if negative (during login process)
                 (false && testUniqueId == DUMMY_SINGLETON_CLIENT_UNIQUE_ID) || // If need to send to dummy server, send there
                 (false && testUniqueId != DUMMY_SINGLETON_CLIENT_UNIQUE_ID && serverControlled) )  // If need to send to npcs, send there
#endif
            {
                #ifdef SERVER
                    Logging::log(Logging::DEBUG, "Sending to %d (%d) ((%d))\r\n", clientNumber, testUniqueId, serverControlled);
                #endif
                sendf(clientNumber, MAIN_CHANNEL, "iii", 1026, soundId, originalClientNumber);

            }
        }
    }

#ifdef CLIENT
    void SoundToClients::receive(int receiver, int sender, ucharbuf &p)
    {
        bool is_npc;
        is_npc = false;
        Logging::log(Logging::DEBUG, "MessageSystem: Receiving a message of type SoundToClients (1026)\r\n");

        int soundId = getint(p);
        int originalClientNumber = getint(p);

        assert(ClientSystem::playerNumber != originalClientNumber);
        dynent* player = FPSClientInterface::getPlayerByNumber(originalClientNumber);
        if (!player)
        {
            if (originalClientNumber == -1) // Do not play sounds from nonexisting clients - would be odd
                playsound(soundId);
        }
        else
        {
            LogicEntityPtr entity = LogicSystem::getLogicEntity( player );
            if (entity.get())
            {
                vec where = entity->getOrigin();
                playsound(soundId, &where);
            } // If no entity - but there should be, there is a player - do not play at all.
        }
    }
#endif


// SoundToClientsByName

    void send_SoundToClientsByName(int clientNumber, float x, float y, float z, std::string soundName, int originalClientNumber)
    {
        int exclude = -1; // Set this to clientNumber to not send to

        Logging::log(Logging::DEBUG, "Sending a message of type SoundToClientsByName (1027)\r\n");
        INDENT_LOG(Logging::DEBUG);

                 exclude = originalClientNumber; // This is how to ensure we do not send back to the client who originally sent it


        int start, finish;
        if (clientNumber == -1)
        {
            // Send to all clients
            start  = 0;
            finish = getnumclients() - 1;
        } else {
            start  = clientNumber;
            finish = clientNumber;
        }

#ifdef SERVER
        int testUniqueId;
#endif
        for (clientNumber = start; clientNumber <= finish; clientNumber++)
        {
            if (clientNumber == exclude) continue;
#ifdef SERVER
            fpsent* fpsEntity = dynamic_cast<fpsent*>( FPSClientInterface::getPlayerByNumber(clientNumber) );
            bool serverControlled = fpsEntity ? fpsEntity->serverControlled : false;

            testUniqueId = FPSServerInterface::getUniqueId(clientNumber);
            if ( (!serverControlled && testUniqueId != DUMMY_SINGLETON_CLIENT_UNIQUE_ID) || // If a remote client, send even if negative (during login process)
                 (false && testUniqueId == DUMMY_SINGLETON_CLIENT_UNIQUE_ID) || // If need to send to dummy server, send there
                 (false && testUniqueId != DUMMY_SINGLETON_CLIENT_UNIQUE_ID && serverControlled) )  // If need to send to npcs, send there
#endif
            {
                #ifdef SERVER
                    Logging::log(Logging::DEBUG, "Sending to %d (%d) ((%d))\r\n", clientNumber, testUniqueId, serverControlled);
                #endif
                sendf(clientNumber, MAIN_CHANNEL, "iiiisi", 1027, int(x*DMF), int(y*DMF), int(z*DMF), soundName.c_str(), originalClientNumber);

            }
        }
    }

#ifdef CLIENT
    void SoundToClientsByName::receive(int receiver, int sender, ucharbuf &p)
    {
        bool is_npc;
        is_npc = false;
        Logging::log(Logging::DEBUG, "MessageSystem: Receiving a message of type SoundToClientsByName (1027)\r\n");

        float x = float(getint(p))/DMF;
        float y = float(getint(p))/DMF;
        float z = float(getint(p))/DMF;
        char tmp_soundName[MAXTRANS];
        getstring(tmp_soundName, p);
        std::string soundName = tmp_soundName;
        int originalClientNumber = getint(p);

        assert(ClientSystem::playerNumber != originalClientNumber);
        vec pos(x,y,z);
        if (pos.x || pos.y || pos.z)
            playsoundname(soundName.c_str(), &pos);
        else
            playsoundname(soundName.c_str());
    }
#endif


// EditModeC2S

    void send_EditModeC2S(int mode)
    {
        Logging::log(Logging::DEBUG, "Sending a message of type EditModeC2S (1028)\r\n");
        INDENT_LOG(Logging::DEBUG);

        game::addmsg(1028, "ri", mode);
    }

#ifdef SERVER
    void EditModeC2S::receive(int receiver, int sender, ucharbuf &p)
    {
        Logging::log(Logging::DEBUG, "MessageSystem: Receiving a message of type EditModeC2S (1028)\r\n");

        int mode = getint(p);

        if (!ServerSystem::isRunningMap()) return;
        #if 0 // The old sauer code from fpsserver.h
            case SV_EDITMODE:
            {
                int val = getint(p);
                if(!ci->local && gamemode!=1) break;
                if(val ? ci->state.state!=CS_ALIVE && ci->state.state!=CS_DEAD : ci->state.state!=CS_EDITING) break;
                if(smode)
                {
                    if(val) smode->leavegame(ci);
                    else smode->entergame(ci);
                }
                if(val)
                {
                    ci->state.editstate = ci->state.state;
                    ci->state.state = CS_EDITING;
                }
                else ci->state.state = ci->state.editstate;
                if(val)
                {
                    ci->events.setsizenodelete(0);
                    ci->state.rockets.reset();
                    ci->state.grenades.reset();
                }
                QUEUE_MSG;
                break;
            }
        #endif
        if ( !server::isRunningCurrentScenario(sender) ) return; // Silently ignore info from previous scenario
        send_EditModeS2C(-1, sender, mode); // Relay
    }
#endif

// EditModeS2C

    void send_EditModeS2C(int clientNumber, int otherClientNumber, int mode)
    {
        int exclude = -1; // Set this to clientNumber to not send to

        Logging::log(Logging::DEBUG, "Sending a message of type EditModeS2C (1029)\r\n");
        INDENT_LOG(Logging::DEBUG);

                 exclude = otherClientNumber;


        int start, finish;
        if (clientNumber == -1)
        {
            // Send to all clients
            start  = 0;
            finish = getnumclients() - 1;
        } else {
            start  = clientNumber;
            finish = clientNumber;
        }

#ifdef SERVER
        int testUniqueId;
#endif
        for (clientNumber = start; clientNumber <= finish; clientNumber++)
        {
            if (clientNumber == exclude) continue;
#ifdef SERVER
            fpsent* fpsEntity = dynamic_cast<fpsent*>( FPSClientInterface::getPlayerByNumber(clientNumber) );
            bool serverControlled = fpsEntity ? fpsEntity->serverControlled : false;

            testUniqueId = FPSServerInterface::getUniqueId(clientNumber);
            if ( (!serverControlled && testUniqueId != DUMMY_SINGLETON_CLIENT_UNIQUE_ID) || // If a remote client, send even if negative (during login process)
                 (true && testUniqueId == DUMMY_SINGLETON_CLIENT_UNIQUE_ID) || // If need to send to dummy server, send there
                 (false && testUniqueId != DUMMY_SINGLETON_CLIENT_UNIQUE_ID && serverControlled) )  // If need to send to npcs, send there
#endif
            {
                #ifdef SERVER
                    Logging::log(Logging::DEBUG, "Sending to %d (%d) ((%d))\r\n", clientNumber, testUniqueId, serverControlled);
                #endif
                sendf(clientNumber, MAIN_CHANNEL, "riii", 1029, otherClientNumber, mode);

            }
        }
    }

    void EditModeS2C::receive(int receiver, int sender, ucharbuf &p)
    {
        bool is_npc;
#ifdef CLIENT
        is_npc = false;
#else // SERVER
        is_npc = true;
#endif
        Logging::log(Logging::DEBUG, "MessageSystem: Receiving a message of type EditModeS2C (1029)\r\n");

        int otherClientNumber = getint(p);
        int mode = getint(p);

        dynent* d = FPSClientInterface::getPlayerByNumber(otherClientNumber);
        // Code from sauer's client.h
        if (d)
        {
            if (mode) 
            {
                d->editstate = d->state;
                d->state     = CS_EDITING;
            }
            else 
            {
                d->state = d->editstate;
            }
        }
    }


// RequestMap

    void send_RequestMap()
    {
        Logging::log(Logging::DEBUG, "Sending a message of type RequestMap (1030)\r\n");
        INDENT_LOG(Logging::DEBUG);

        game::addmsg(1030, "r");
    }

#ifdef SERVER
    void RequestMap::receive(int receiver, int sender, ucharbuf &p)
    {
        Logging::log(Logging::DEBUG, "MessageSystem: Receiving a message of type RequestMap (1030)\r\n");


        if (!ServerSystem::isRunningMap()) return;
        REFLECT_PYTHON( send_curr_map );
        send_curr_map(sender);
    }
#endif

// DoClick

    void send_DoClick(int button, int down, float x, float y, float z, int uniqueId)
    {
        Logging::log(Logging::DEBUG, "Sending a message of type DoClick (1031)\r\n");
        INDENT_LOG(Logging::DEBUG);

        game::addmsg(1031, "riiiiii", button, down, int(x*DMF), int(y*DMF), int(z*DMF), uniqueId);
    }

#ifdef SERVER
    void DoClick::receive(int receiver, int sender, ucharbuf &p)
    {
        Logging::log(Logging::DEBUG, "MessageSystem: Receiving a message of type DoClick (1031)\r\n");

        int button = getint(p);
        int down = getint(p);
        float x = float(getint(p))/DMF;
        float y = float(getint(p))/DMF;
        float z = float(getint(p))/DMF;
        int uniqueId = getint(p);

        if (!ServerSystem::isRunningMap()) return;
        if ( !server::isRunningCurrentScenario(sender) ) return; // Silently ignore info from previous scenario
        ScriptValuePtr position = ScriptEngineManager::runScript(
            "new Vector3(" + 
                Utility::toString(x) + "," + 
                Utility::toString(y) + "," + 
                Utility::toString(z) +
            ")"
        );
        ScriptValueArgs args;
        args.append(button).append(down).append(position);
        if (uniqueId != -1)
        {
            LogicEntityPtr entity = LogicSystem::getLogicEntity(uniqueId);
            if (entity.get())
                args.append(entity->scriptEntity);
            else
                return; // No need to do a click that was on an entity that vanished meanwhile/does not yet exist!
        }
        ScriptEngineManager::getGlobal()->getProperty("ApplicationManager")->getProperty("instance")->call("click", args);
    }
#endif

// MapUpdated

    void send_MapUpdated(int clientNumber, int updatingClientNumber)
    {
        int exclude = -1; // Set this to clientNumber to not send to

        Logging::log(Logging::DEBUG, "Sending a message of type MapUpdated (1032)\r\n");
        INDENT_LOG(Logging::DEBUG);

         

        int start, finish;
        if (clientNumber == -1)
        {
            // Send to all clients
            start  = 0;
            finish = getnumclients() - 1;
        } else {
            start  = clientNumber;
            finish = clientNumber;
        }

#ifdef SERVER
        int testUniqueId;
#endif
        for (clientNumber = start; clientNumber <= finish; clientNumber++)
        {
            if (clientNumber == exclude) continue;
#ifdef SERVER
            fpsent* fpsEntity = dynamic_cast<fpsent*>( FPSClientInterface::getPlayerByNumber(clientNumber) );
            bool serverControlled = fpsEntity ? fpsEntity->serverControlled : false;

            testUniqueId = FPSServerInterface::getUniqueId(clientNumber);
            if ( (!serverControlled && testUniqueId != DUMMY_SINGLETON_CLIENT_UNIQUE_ID) || // If a remote client, send even if negative (during login process)
                 (false && testUniqueId == DUMMY_SINGLETON_CLIENT_UNIQUE_ID) || // If need to send to dummy server, send there
                 (false && testUniqueId != DUMMY_SINGLETON_CLIENT_UNIQUE_ID && serverControlled) )  // If need to send to npcs, send there
#endif
            {
                #ifdef SERVER
                    Logging::log(Logging::DEBUG, "Sending to %d (%d) ((%d))\r\n", clientNumber, testUniqueId, serverControlled);
                #endif
                sendf(clientNumber, MAIN_CHANNEL, "rii", 1032, updatingClientNumber);

            }
        }
    }

#ifdef CLIENT
    void MapUpdated::receive(int receiver, int sender, ucharbuf &p)
    {
        bool is_npc;
        is_npc = false;
        Logging::log(Logging::DEBUG, "MessageSystem: Receiving a message of type MapUpdated (1032)\r\n");

        int updatingClientNumber = getint(p);

        updatingClientNumber = updatingClientNumber; // warning
        assert(0);
        #if 0
        if (updatingClientNumber != ClientSystem::playerNumber)
            IntensityCEGUI::showMessage("Map Updated", "Another player has updated the map on the server. To receive this update, do File->Receive map from server in edit mode.");
        else
            IntensityCEGUI::showMessage("Map Updated", "Your update to the map was received by the server successfully.");
        #endif
    }
#endif


// ParticleSplashToClients

    void send_ParticleSplashToClients(int clientNumber, int _type, int num, int fade, float x, float y, float z)
    {
        int exclude = -1; // Set this to clientNumber to not send to

        Logging::log(Logging::DEBUG, "Sending a message of type ParticleSplashToClients (1033)\r\n");
        INDENT_LOG(Logging::DEBUG);

         

        int start, finish;
        if (clientNumber == -1)
        {
            // Send to all clients
            start  = 0;
            finish = getnumclients() - 1;
        } else {
            start  = clientNumber;
            finish = clientNumber;
        }

#ifdef SERVER
        int testUniqueId;
#endif
        for (clientNumber = start; clientNumber <= finish; clientNumber++)
        {
            if (clientNumber == exclude) continue;
#ifdef SERVER
            fpsent* fpsEntity = dynamic_cast<fpsent*>( FPSClientInterface::getPlayerByNumber(clientNumber) );
            bool serverControlled = fpsEntity ? fpsEntity->serverControlled : false;

            testUniqueId = FPSServerInterface::getUniqueId(clientNumber);
            if ( (!serverControlled && testUniqueId != DUMMY_SINGLETON_CLIENT_UNIQUE_ID) || // If a remote client, send even if negative (during login process)
                 (false && testUniqueId == DUMMY_SINGLETON_CLIENT_UNIQUE_ID) || // If need to send to dummy server, send there
                 (false && testUniqueId != DUMMY_SINGLETON_CLIENT_UNIQUE_ID && serverControlled) )  // If need to send to npcs, send there
#endif
            {
                #ifdef SERVER
                    Logging::log(Logging::DEBUG, "Sending to %d (%d) ((%d))\r\n", clientNumber, testUniqueId, serverControlled);
                #endif
                sendf(clientNumber, MAIN_CHANNEL, "iiiiiii", 1033, _type, num, fade, int(x*DMF), int(y*DMF), int(z*DMF));

            }
        }
    }

#ifdef CLIENT
    void ParticleSplashToClients::receive(int receiver, int sender, ucharbuf &p)
    {
        bool is_npc;
        is_npc = false;
        Logging::log(Logging::DEBUG, "MessageSystem: Receiving a message of type ParticleSplashToClients (1033)\r\n");

        int _type = getint(p);
        int num = getint(p);
        int fade = getint(p);
        float x = float(getint(p))/DMF;
        float y = float(getint(p))/DMF;
        float z = float(getint(p))/DMF;

        vec pos(x,y,z);
        particle_splash(_type, num, fade, pos);
    }
#endif


// RequestPrivateEditMode

    void send_RequestPrivateEditMode()
    {
        Logging::log(Logging::DEBUG, "Sending a message of type RequestPrivateEditMode (1034)\r\n");
        INDENT_LOG(Logging::DEBUG);

        game::addmsg(1034, "r");
    }

#ifdef SERVER
    void RequestPrivateEditMode::receive(int receiver, int sender, ucharbuf &p)
    {
        Logging::log(Logging::DEBUG, "MessageSystem: Receiving a message of type RequestPrivateEditMode (1034)\r\n");


        if (!ServerSystem::isRunningMap()) return;
        REFLECT_PYTHON( request_private_edit );
        request_private_edit(sender);
    }
#endif

// NotifyPrivateEditMode

    void send_NotifyPrivateEditMode(int clientNumber)
    {
        int exclude = -1; // Set this to clientNumber to not send to

        Logging::log(Logging::DEBUG, "Sending a message of type NotifyPrivateEditMode (1035)\r\n");
        INDENT_LOG(Logging::DEBUG);

         

        int start, finish;
        if (clientNumber == -1)
        {
            // Send to all clients
            start  = 0;
            finish = getnumclients() - 1;
        } else {
            start  = clientNumber;
            finish = clientNumber;
        }

#ifdef SERVER
        int testUniqueId;
#endif
        for (clientNumber = start; clientNumber <= finish; clientNumber++)
        {
            if (clientNumber == exclude) continue;
#ifdef SERVER
            fpsent* fpsEntity = dynamic_cast<fpsent*>( FPSClientInterface::getPlayerByNumber(clientNumber) );
            bool serverControlled = fpsEntity ? fpsEntity->serverControlled : false;

            testUniqueId = FPSServerInterface::getUniqueId(clientNumber);
            if ( (!serverControlled && testUniqueId != DUMMY_SINGLETON_CLIENT_UNIQUE_ID) || // If a remote client, send even if negative (during login process)
                 (false && testUniqueId == DUMMY_SINGLETON_CLIENT_UNIQUE_ID) || // If need to send to dummy server, send there
                 (false && testUniqueId != DUMMY_SINGLETON_CLIENT_UNIQUE_ID && serverControlled) )  // If need to send to npcs, send there
#endif
            {
                #ifdef SERVER
                    Logging::log(Logging::DEBUG, "Sending to %d (%d) ((%d))\r\n", clientNumber, testUniqueId, serverControlled);
                #endif
                sendf(clientNumber, MAIN_CHANNEL, "ri", 1035);

            }
        }
    }

#ifdef CLIENT
    void NotifyPrivateEditMode::receive(int receiver, int sender, ucharbuf &p)
    {
        bool is_npc;
        is_npc = false;
        Logging::log(Logging::DEBUG, "MessageSystem: Receiving a message of type NotifyPrivateEditMode (1035)\r\n");


        IntensityGUI::showMessage("", "Server: You are now in private edit mode");
        ClientSystem::editingAlone = true;
    }
#endif


// Register all messages

void MessageManager::registerAll()
{
    registerMessageType( new PersonalServerMessage() );
    registerMessageType( new RequestServerMessageToAll() );
    registerMessageType( new LoginRequest() );
    registerMessageType( new YourUniqueId() );
    registerMessageType( new LoginResponse() );
    registerMessageType( new PrepareForNewScenario() );
    registerMessageType( new RequestCurrentScenario() );
    registerMessageType( new NotifyAboutCurrentScenario() );
    registerMessageType( new RestartMap() );
    registerMessageType( new NewEntityRequest() );
    registerMessageType( new StateDataUpdate() );
    registerMessageType( new StateDataChangeRequest() );
    registerMessageType( new UnreliableStateDataUpdate() );
    registerMessageType( new UnreliableStateDataChangeRequest() );
    registerMessageType( new NotifyNumEntities() );
    registerMessageType( new AllActiveEntitiesSent() );
    registerMessageType( new ActiveEntitiesRequest() );
    registerMessageType( new LogicEntityCompleteNotification() );
    registerMessageType( new RequestLogicEntityRemoval() );
    registerMessageType( new LogicEntityRemoval() );
    registerMessageType( new ExtentCompleteNotification() );
    registerMessageType( new InitS2C() );
    registerMessageType( new MapVote() );
    registerMessageType( new MapChange() );
    registerMessageType( new SoundToServer() );
    registerMessageType( new SoundToClients() );
    registerMessageType( new SoundToClientsByName() );
    registerMessageType( new EditModeC2S() );
    registerMessageType( new EditModeS2C() );
    registerMessageType( new RequestMap() );
    registerMessageType( new DoClick() );
    registerMessageType( new MapUpdated() );
    registerMessageType( new ParticleSplashToClients() );
    registerMessageType( new RequestPrivateEditMode() );
    registerMessageType( new NotifyPrivateEditMode() );
}

}

