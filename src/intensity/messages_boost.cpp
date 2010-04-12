
// Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
// This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

// Automatically generated from messages.template - DO NOT MODIFY THIS FILE!


    // PersonalServerMessage
    exposeToPython("PersonalServerMessage", &MessageSystem::send_PersonalServerMessage);

#ifdef CLIENT
    // RequestServerMessageToAll
    exposeToPython("RequestServerMessageToAll", &MessageSystem::send_RequestServerMessageToAll);
#endif

#ifdef CLIENT
    // LoginRequest
    exposeToPython("LoginRequest", &MessageSystem::send_LoginRequest);
#endif

    // YourUniqueId
    exposeToPython("YourUniqueId", &MessageSystem::send_YourUniqueId);

    // LoginResponse
    exposeToPython("LoginResponse", &MessageSystem::send_LoginResponse);

    // PrepareForNewScenario
    exposeToPython("PrepareForNewScenario", &MessageSystem::send_PrepareForNewScenario);

#ifdef CLIENT
    // RequestCurrentScenario
    exposeToPython("RequestCurrentScenario", &MessageSystem::send_RequestCurrentScenario);
#endif

    // NotifyAboutCurrentScenario
    exposeToPython("NotifyAboutCurrentScenario", &MessageSystem::send_NotifyAboutCurrentScenario);

#ifdef CLIENT
    // RestartMap
    exposeToPython("RestartMap", &MessageSystem::send_RestartMap);
#endif

#ifdef CLIENT
    // NewEntityRequest
    exposeToPython("NewEntityRequest", &MessageSystem::send_NewEntityRequest);
#endif

#ifdef CLIENT
    // StateDataUpdate
    exposeToPython("StateDataUpdate", &MessageSystem::send_StateDataUpdate);
#endif

#ifdef CLIENT
    // StateDataChangeRequest
    exposeToPython("StateDataChangeRequest", &MessageSystem::send_StateDataChangeRequest);
#endif

#ifdef CLIENT
    // UnreliableStateDataUpdate
    exposeToPython("UnreliableStateDataUpdate", &MessageSystem::send_UnreliableStateDataUpdate);
#endif

#ifdef CLIENT
    // UnreliableStateDataChangeRequest
    exposeToPython("UnreliableStateDataChangeRequest", &MessageSystem::send_UnreliableStateDataChangeRequest);
#endif

    // NotifyNumEntities
    exposeToPython("NotifyNumEntities", &MessageSystem::send_NotifyNumEntities);

    // AllActiveEntitiesSent
    exposeToPython("AllActiveEntitiesSent", &MessageSystem::send_AllActiveEntitiesSent);

#ifdef CLIENT
    // ActiveEntitiesRequest
    exposeToPython("ActiveEntitiesRequest", &MessageSystem::send_ActiveEntitiesRequest);
#endif

#ifdef CLIENT
    // LogicEntityCompleteNotification
    exposeToPython("LogicEntityCompleteNotification", &MessageSystem::send_LogicEntityCompleteNotification);
#endif

#ifdef CLIENT
    // RequestLogicEntityRemoval
    exposeToPython("RequestLogicEntityRemoval", &MessageSystem::send_RequestLogicEntityRemoval);
#endif

    // LogicEntityRemoval
    exposeToPython("LogicEntityRemoval", &MessageSystem::send_LogicEntityRemoval);

    // ExtentCompleteNotification
    exposeToPython("ExtentCompleteNotification", &MessageSystem::send_ExtentCompleteNotification);

    // InitS2C
    exposeToPython("InitS2C", &MessageSystem::send_InitS2C);

#ifdef CLIENT
    // MapVote
    exposeToPython("MapVote", &MessageSystem::send_MapVote);
#endif

    // MapChange
    exposeToPython("MapChange", &MessageSystem::send_MapChange);

#ifdef CLIENT
    // SoundToServer
    exposeToPython("SoundToServer", &MessageSystem::send_SoundToServer);
#endif

    // SoundToClients
    exposeToPython("SoundToClients", &MessageSystem::send_SoundToClients);

    // SoundToClientsByName
    exposeToPython("SoundToClientsByName", &MessageSystem::send_SoundToClientsByName);

#ifdef CLIENT
    // EditModeC2S
    exposeToPython("EditModeC2S", &MessageSystem::send_EditModeC2S);
#endif

#ifdef CLIENT
    // EditModeS2C
    exposeToPython("EditModeS2C", &MessageSystem::send_EditModeS2C);
#endif

#ifdef CLIENT
    // RequestMap
    exposeToPython("RequestMap", &MessageSystem::send_RequestMap);
#endif

#ifdef CLIENT
    // DoClick
    exposeToPython("DoClick", &MessageSystem::send_DoClick);
#endif

    // MapUpdated
    exposeToPython("MapUpdated", &MessageSystem::send_MapUpdated);

    // ParticleSplashToClients
    exposeToPython("ParticleSplashToClients", &MessageSystem::send_ParticleSplashToClients);

#ifdef CLIENT
    // RequestPrivateEditMode
    exposeToPython("RequestPrivateEditMode", &MessageSystem::send_RequestPrivateEditMode);
#endif

    // NotifyPrivateEditMode
    exposeToPython("NotifyPrivateEditMode", &MessageSystem::send_NotifyPrivateEditMode);

