
// Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
// This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

#include "cube.h"
#include "engine.h"
#include "game.h"

#include "message_system.h"
#include "script_engine_manager.h"
#include "utility.h"

#ifdef CLIENT
    #include "client_system.h"
#endif

#include "system_manager.h"


//==================================
// System
//==================================

void SystemManager::init()
{
    printf("SystemManager::init()\r\n");

    printf("SystemManager::MessageSystem setup\r\n");
    MessageSystem::MessageManager::registerAll();

    #ifdef CLIENT
        int haveMaster = (Utility::Config::getString("Network", "master_server", "") != "");
        setvar("have_master", haveMaster);
        if (!haveMaster)
        {
            setvar("logged_into_master", 1);
            execute("setup_main_menu");
            Logging::log(Logging::DEBUG, "No master server; working entirely remotely\r\n");
        }
    #endif
}

void SystemManager::quit()
{
    ScriptEngineManager::destroyEngine();
}

// XXX Not used, deprecated
bool SystemManager::benchmarking = false;

void SystemManager::showBenchmark(std::string title, Benchmarker& benchmark)
{
    int benchmarkingSeconds = Utility::Config::getInt("System", "benchmarking", 0);

    if (benchmarkingSeconds)
    {
        if (benchmark.totalPassed() > benchmarkingSeconds*1000)
        {
            printf("[[ last %d secs ]]   %s : %3.1f%% \r\n", benchmarkingSeconds, title.c_str(), benchmark.percentage());
            benchmark.reset();
        }
    }
}

void SystemManager::frameTrigger(int curtime)
{
    #ifdef CLIENT
        ClientSystem::frameTrigger(curtime);
    #endif
}

