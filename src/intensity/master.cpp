
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

#include "utility.h"

#include "master.h"

namespace MasterServer
{

#ifdef CLIENT

SVARP(entered_username, ""); // Persisted - uses "-" instead of "@", to get around sauer issue
SVAR(true_username, "");  // Has "@", can be sent to server to login
SVAR(entered_password, "");
SVARP(hashed_password, "");

VAR(have_master, 0, 1, 1);
VAR(logged_into_master, 0, 0, 1);

SVAR(error_message, ""); // TODO: Move

//! Log in to the master server
void do_login(char *username, char *password)
{
    Logging::log(Logging::DEBUG, "Preparing to log in to master server with: %s / ----\r\n", username);

    std::string _username = username;
    std::string _password = password;

    if (_password == "--------") // If a password not entered, use the old (hashed) one
    {
        _password = hashed_password;
    }

    REFLECT_PYTHON( login_to_master );

    boost::python::list ret = boost::python::list(login_to_master(_username, _password));
    bool success = boost::python::extract<bool>(ret[0]);
    if (success) {
        // Save password
        _password = boost::python::extract<std::string>(ret[1]);
        setsvar("hashed_password", _password.c_str());

        // Mark as logged in, and continue
        setvar("logged_into_master", 1);
        execute("setup_main_menu");
        conoutf("Logged in successfully");
    } else {
        setsvar("hashed_password", ""); // Remove the old saved password, for security
    }
}

COMMAND(do_login, "ss");

void logout()
{
    logged_into_master = 0;
    execute("setup_main_menu");
}

#endif

}

