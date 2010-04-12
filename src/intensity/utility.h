
// Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
// This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

//! General utilities

struct Utility
{
    //! SHA256-hash. We use Python for convenience to implement this
    static std::string SHA256(std::string text);

    //! Convert to std::string (this one is a dummy, exists just so we can call toString on anything)
    static std::string toString(std::string val);
    //! Convert to std::string
    static std::string toString(int         val);
    //! Convert to std::string
    static std::string toString(long        val);
    //! Convert to std::string
    static std::string toString(double      val);

    //! Ensures that a string contains only characters and numbers
    static bool validateAlphaNumeric(std::string input, std::string alsoAllow="");

    //! Ensures that a string does NOT contain some substring
    static bool validateNotContaining(std::string input, std::string disallow);

    //! Ensures that a relative path (created by e.g. makerelpath) does not 'break out'
    //! here/../some.file is breaking out
    static bool validateRelativePath(std::string input);

    //! Read the contents of a file and return them, in a cross-platform manner
    //! Will halt on error (what we want for now, but FIXME)
    static std::string readFile(std::string name);

    //! Configuration file management
    struct Config
    {
        //! Gets a string configuration variable, or 'defaultVal' if doesn't exist
        static std::string getString(std::string section, std::string option, std::string defaultVal);
        //! Gets an int configuration variable, or 'defaultVal' if doesn't exist
        static int         getInt   (std::string section, std::string option, int         defaultVal);
        //! Gets a float configuration variable, or 'defaultVal' if doesn't exist
        static float       getFloat (std::string section, std::string option, float       defaultVal);

        //! Sets a string configuration variable
        static void setString(std::string section, std::string option, std::string value);
        //! Sets an int configuration variable
        static void setInt   (std::string section, std::string option, int         value);
        //! Sets a float configuration variable
        static void setFloat (std::string section, std::string option, float       value);
    };

    //! System information

    struct SystemInfo
    {
        //! The current time in the local machine
        static int currTime();
    };
};

struct Timer
{
    int startTime;

    Timer() { reset(); };
    virtual ~Timer() { };

    int totalPassed() { return Utility::SystemInfo::currTime() - startTime; };
    virtual void reset() { startTime = Utility::SystemInfo::currTime(); };
};

struct Benchmarker : Timer
{
    //! The last time start() was called
    int currStartTime;

    //! The total amount of time between start() and stop() calls
    int totalTime;

    void start() { currStartTime = Utility::SystemInfo::currTime(); };
    void stop() { totalTime += Utility::SystemInfo::currTime() - currStartTime; currStartTime = -1; };
    float percentage() { return 100.0f * float(totalTime) / totalPassed(); };
    virtual void reset() { Timer::reset(); currStartTime = -1; totalTime = 0; };
};

