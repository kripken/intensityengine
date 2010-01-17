
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

