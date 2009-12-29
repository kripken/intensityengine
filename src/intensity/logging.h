
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

