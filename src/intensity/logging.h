
// Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
// This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

// Windows
#undef ERROR

//! Simple logging functionality. TODO: Interface Python with this

#define INDENT_LOG(level) Logging::Indent ind(level)

namespace Logging
{
    //! INFO: low-importance messages that may also appear very often (e.g., once/frame)
    //! DEBUG: low-importance warnings that do not appear too often, and may be nice to see during debugging
    //! WARNING: possible problems
    //! ERROR: problems, that generally imply we cannot recover
    //! OFF: no logging will be done
    enum Level { INFO, DEBUG, WARNING, ERROR, OFF };

    //! Sets the current debugging level
    void setCurrLevel(Level level);

    //! Test whether a level would be shown (useful to know if we are in debug mode)
    bool shouldShow(Level level);

    //! Prepare logging system. Reads currLevel from config file
    void init();

    //! Log a message, using standard printf format
    void log(Level level, const char *fmt, ...);

    //! The logging function Python will call
    void log_noformat(int level, std::string);

    //! Handy way to indent the logging. Create an instance of this, and it will indent all further logs (if the level
    //! is relevant) until the destructor is called, when it goes of scope, i.e. the de-indenting is done automatically
    //! for us.
    struct Indent
    {
        Indent(Level level);
        ~Indent();
    private:
        bool done;
    };
};

