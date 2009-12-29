
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

// Automatically generated from messages.template - DO NOT MODIFY THIS FILE!


// PersonalServerMessage

struct PersonalServerMessage : MessageType
{
    PersonalServerMessage() : MessageType(1001, "PersonalServerMessage") { };

#ifdef CLIENT
    void receive(int receiver, int sender, ucharbuf &p);
#endif
};

void send_PersonalServerMessage(int clientNumber, int originClientNumber, std::string title, std::string content);


// RequestServerMessageToAll

struct RequestServerMessageToAll : MessageType
{
    RequestServerMessageToAll() : MessageType(1002, "RequestServerMessageToAll") { };

#ifdef SERVER
    void receive(int receiver, int sender, ucharbuf &p);
#endif
};

void send_RequestServerMessageToAll(std::string message);


// LoginRequest

struct LoginRequest : MessageType
{
    LoginRequest() : MessageType(1003, "LoginRequest") { };

#ifdef SERVER
    void receive(int receiver, int sender, ucharbuf &p);
#endif
};

void send_LoginRequest(std::string code);


// YourUniqueId

struct YourUniqueId : MessageType
{
    YourUniqueId() : MessageType(1004, "YourUniqueId") { };

#ifdef CLIENT
    void receive(int receiver, int sender, ucharbuf &p);
#endif
};

void send_YourUniqueId(int clientNumber, int uniqueId);


// LoginResponse

struct LoginResponse : MessageType
{
    LoginResponse() : MessageType(1005, "LoginResponse") { };

#ifdef CLIENT
    void receive(int receiver, int sender, ucharbuf &p);
#endif
};

void send_LoginResponse(int clientNumber, bool success, bool local);


// PrepareForNewScenario

struct PrepareForNewScenario : MessageType
{
    PrepareForNewScenario() : MessageType(1006, "PrepareForNewScenario") { };

#ifdef CLIENT
    void receive(int receiver, int sender, ucharbuf &p);
#endif
};

void send_PrepareForNewScenario(int clientNumber, std::string scenarioCode);


// RequestCurrentScenario

struct RequestCurrentScenario : MessageType
{
    RequestCurrentScenario() : MessageType(1007, "RequestCurrentScenario") { };

#ifdef SERVER
    void receive(int receiver, int sender, ucharbuf &p);
#endif
};

void send_RequestCurrentScenario();


// NotifyAboutCurrentScenario

struct NotifyAboutCurrentScenario : MessageType
{
    NotifyAboutCurrentScenario() : MessageType(1008, "NotifyAboutCurrentScenario") { };

#ifdef CLIENT
    void receive(int receiver, int sender, ucharbuf &p);
#endif
};

void send_NotifyAboutCurrentScenario(int clientNumber, std::string mapAssetId, std::string scenarioCode);


// RestartMap

struct RestartMap : MessageType
{
    RestartMap() : MessageType(1009, "RestartMap") { };

#ifdef SERVER
    void receive(int receiver, int sender, ucharbuf &p);
#endif
};

void send_RestartMap();


// NewEntityRequest

struct NewEntityRequest : MessageType
{
    NewEntityRequest() : MessageType(1010, "NewEntityRequest") { };

#ifdef SERVER
    void receive(int receiver, int sender, ucharbuf &p);
#endif
};

void send_NewEntityRequest(std::string _class, float x, float y, float z, std::string stateData);


// StateDataUpdate

struct StateDataUpdate : MessageType
{
    StateDataUpdate() : MessageType(1011, "StateDataUpdate") { };

    void receive(int receiver, int sender, ucharbuf &p);
};

void send_StateDataUpdate(int clientNumber, int uniqueId, int keyProtocolId, std::string value, int originalClientNumber);


// StateDataChangeRequest

struct StateDataChangeRequest : MessageType
{
    StateDataChangeRequest() : MessageType(1012, "StateDataChangeRequest") { };

#ifdef SERVER
    void receive(int receiver, int sender, ucharbuf &p);
#endif
};

void send_StateDataChangeRequest(int uniqueId, int keyProtocolId, std::string value);


// UnreliableStateDataUpdate

struct UnreliableStateDataUpdate : MessageType
{
    UnreliableStateDataUpdate() : MessageType(1013, "UnreliableStateDataUpdate") { };

    void receive(int receiver, int sender, ucharbuf &p);
};

void send_UnreliableStateDataUpdate(int clientNumber, int uniqueId, int keyProtocolId, std::string value, int originalClientNumber);


// UnreliableStateDataChangeRequest

struct UnreliableStateDataChangeRequest : MessageType
{
    UnreliableStateDataChangeRequest() : MessageType(1014, "UnreliableStateDataChangeRequest") { };

#ifdef SERVER
    void receive(int receiver, int sender, ucharbuf &p);
#endif
};

void send_UnreliableStateDataChangeRequest(int uniqueId, int keyProtocolId, std::string value);


// NotifyNumEntities

struct NotifyNumEntities : MessageType
{
    NotifyNumEntities() : MessageType(1015, "NotifyNumEntities") { };

#ifdef CLIENT
    void receive(int receiver, int sender, ucharbuf &p);
#endif
};

void send_NotifyNumEntities(int clientNumber, int num);


// AllActiveEntitiesSent

struct AllActiveEntitiesSent : MessageType
{
    AllActiveEntitiesSent() : MessageType(1016, "AllActiveEntitiesSent") { };

#ifdef CLIENT
    void receive(int receiver, int sender, ucharbuf &p);
#endif
};

void send_AllActiveEntitiesSent(int clientNumber);


// ActiveEntitiesRequest

struct ActiveEntitiesRequest : MessageType
{
    ActiveEntitiesRequest() : MessageType(1017, "ActiveEntitiesRequest") { };

#ifdef SERVER
    void receive(int receiver, int sender, ucharbuf &p);
#endif
};

void send_ActiveEntitiesRequest(std::string scenarioCode);


// LogicEntityCompleteNotification

struct LogicEntityCompleteNotification : MessageType
{
    LogicEntityCompleteNotification() : MessageType(1018, "LogicEntityCompleteNotification") { };

    void receive(int receiver, int sender, ucharbuf &p);
};

void send_LogicEntityCompleteNotification(int clientNumber, int otherClientNumber, int otherUniqueId, std::string otherClass, std::string stateData);


// RequestLogicEntityRemoval

struct RequestLogicEntityRemoval : MessageType
{
    RequestLogicEntityRemoval() : MessageType(1019, "RequestLogicEntityRemoval") { };

#ifdef SERVER
    void receive(int receiver, int sender, ucharbuf &p);
#endif
};

void send_RequestLogicEntityRemoval(int uniqueId);


// LogicEntityRemoval

struct LogicEntityRemoval : MessageType
{
    LogicEntityRemoval() : MessageType(1020, "LogicEntityRemoval") { };

#ifdef CLIENT
    void receive(int receiver, int sender, ucharbuf &p);
#endif
};

void send_LogicEntityRemoval(int clientNumber, int uniqueId);


// ExtentCompleteNotification

struct ExtentCompleteNotification : MessageType
{
    ExtentCompleteNotification() : MessageType(1021, "ExtentCompleteNotification") { };

#ifdef CLIENT
    void receive(int receiver, int sender, ucharbuf &p);
#endif
};

void send_ExtentCompleteNotification(int clientNumber, int otherUniqueId, std::string otherClass, std::string stateData, float x, float y, float z, int attr1, int attr2, int attr3, int attr4);


// InitS2C

struct InitS2C : MessageType
{
    InitS2C() : MessageType(1022, "InitS2C") { };

#ifdef CLIENT
    void receive(int receiver, int sender, ucharbuf &p);
#endif
};

void send_InitS2C(int clientNumber, int explicitClientNumber, int protocolVersion);


// MapVote

struct MapVote : MessageType
{
    MapVote() : MessageType(1023, "MapVote") { };

#ifdef SERVER
    void receive(int receiver, int sender, ucharbuf &p);
#endif
};

void send_MapVote(std::string name);


// MapChange

struct MapChange : MessageType
{
    MapChange() : MessageType(1024, "MapChange") { };

#ifdef CLIENT
    void receive(int receiver, int sender, ucharbuf &p);
#endif
};

void send_MapChange(int clientNumber, std::string name);


// SoundToServer

struct SoundToServer : MessageType
{
    SoundToServer() : MessageType(1025, "SoundToServer") { };

#ifdef SERVER
    void receive(int receiver, int sender, ucharbuf &p);
#endif
};

void send_SoundToServer(int soundId);


// SoundToClients

struct SoundToClients : MessageType
{
    SoundToClients() : MessageType(1026, "SoundToClients") { };

#ifdef CLIENT
    void receive(int receiver, int sender, ucharbuf &p);
#endif
};

void send_SoundToClients(int clientNumber, int soundId, int originalClientNumber);


// SoundToClientsByName

struct SoundToClientsByName : MessageType
{
    SoundToClientsByName() : MessageType(1027, "SoundToClientsByName") { };

#ifdef CLIENT
    void receive(int receiver, int sender, ucharbuf &p);
#endif
};

void send_SoundToClientsByName(int clientNumber, float x, float y, float z, std::string soundName, int originalClientNumber);


// EditModeC2S

struct EditModeC2S : MessageType
{
    EditModeC2S() : MessageType(1028, "EditModeC2S") { };

#ifdef SERVER
    void receive(int receiver, int sender, ucharbuf &p);
#endif
};

void send_EditModeC2S(int mode);


// EditModeS2C

struct EditModeS2C : MessageType
{
    EditModeS2C() : MessageType(1029, "EditModeS2C") { };

    void receive(int receiver, int sender, ucharbuf &p);
};

void send_EditModeS2C(int clientNumber, int otherClientNumber, int mode);


// RequestMap

struct RequestMap : MessageType
{
    RequestMap() : MessageType(1030, "RequestMap") { };

#ifdef SERVER
    void receive(int receiver, int sender, ucharbuf &p);
#endif
};

void send_RequestMap();


// DoClick

struct DoClick : MessageType
{
    DoClick() : MessageType(1031, "DoClick") { };

#ifdef SERVER
    void receive(int receiver, int sender, ucharbuf &p);
#endif
};

void send_DoClick(int button, int down, float x, float y, float z, int uniqueId);


// MapUpdated

struct MapUpdated : MessageType
{
    MapUpdated() : MessageType(1032, "MapUpdated") { };

#ifdef CLIENT
    void receive(int receiver, int sender, ucharbuf &p);
#endif
};

void send_MapUpdated(int clientNumber, int updatingClientNumber);


// ParticleSplashToClients

struct ParticleSplashToClients : MessageType
{
    ParticleSplashToClients() : MessageType(1033, "ParticleSplashToClients") { };

#ifdef CLIENT
    void receive(int receiver, int sender, ucharbuf &p);
#endif
};

void send_ParticleSplashToClients(int clientNumber, int _type, int num, int fade, float x, float y, float z);


// RequestPrivateEditMode

struct RequestPrivateEditMode : MessageType
{
    RequestPrivateEditMode() : MessageType(1034, "RequestPrivateEditMode") { };

#ifdef SERVER
    void receive(int receiver, int sender, ucharbuf &p);
#endif
};

void send_RequestPrivateEditMode();


// NotifyPrivateEditMode

struct NotifyPrivateEditMode : MessageType
{
    NotifyPrivateEditMode() : MessageType(1035, "NotifyPrivateEditMode") { };

#ifdef CLIENT
    void receive(int receiver, int sender, ucharbuf &p);
#endif
};

void send_NotifyPrivateEditMode(int clientNumber);

