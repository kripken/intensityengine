
// Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
// This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

//! General system-wide management

struct SystemManager
{
    //! Calls all initializations for all of our systems
    static void init();

    //! Calls all quittings for all of our systems
    static void quit();

    static bool benchmarking;

    static void showBenchmark(std::string title, Benchmarker& benchmark);

    //! Stuff done on each frame
    static void frameTrigger(int curtime);
};

