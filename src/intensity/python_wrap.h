
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


// Python wrapping utilities: Uses boost::python, not all the fancy stuff
// there because it doesn't work (sent a message to boost.users), but using
// these macros means we are immune to changes there.
//
// In the future we may consider using Jython or PyPy, and we need to change
// only this wrapper file

#include <boost/python.hpp>

extern bool pythonInitialized;

extern boost::python::object python_main_module;
extern boost::python::object python_main_namespace;

//! Initialize use of Python, as if it received certain
//! commandline parameters
extern void initPython(int argc, char **argv);

//! Run a python command
#define EXEC_PYTHON(command)                                                                                                      \
try                                                                                                                               \
{                                                                                                                                 \
    assert(pythonInitialized);                                                                                                    \
                                                                                                                                  \
    boost::python::object dummy_ignore_wasdy = boost::python::exec(std::string(command).c_str(), python_main_namespace, python_main_namespace); \
}                                                                                                                                 \
catch(boost::python::error_already_set const &)                                                                                          \
{                                                                                                                                 \
    printf("Error in Python execution of: \r\n%s\r\n", std::string(command).c_str());                                             \
    PyErr_Print();                                                                                                                \
    assert(0 && "Halting on Python error");                                                                                       \
}

//! Store the output of command in var_name, which has type output_type
#define EVAL_PYTHON(output_type, var_name, command)                                         \
output_type var_name;                                                                       \
try                                                                                         \
{                                                                                           \
    assert(pythonInitialized);                                                              \
                                                                                            \
    std::string full_command_wasdy = std::string("result_wasdy = ") + std::string(command); \
    EXEC_PYTHON(full_command_wasdy.c_str());                                                \
    boost::python::object python_result_wasdy = python_main_namespace["result_wasdy"];             \
    var_name = boost::python::extract<output_type>(python_result_wasdy);                           \
}                                                                                           \
catch(boost::python::error_already_set const &)                                                    \
{                                                                                           \
    printf("Error in Python execution of: \r\n%s\r\n", std::string(command).c_str());       \
    PyErr_Print();                                                                          \
    assert(0 && "Halting on Python error");                                                 \
}

//! Get a value from python, of a particular type. Can be a variable name, or even a complete expression
template<class T>
T GET_PYTHON(std::string var_name)
{
    EVAL_PYTHON(T, result_wasdy2, var_name);
    return result_wasdy2;
};


//! Reflect a python object in a boost::python object. Used for security reasons to not enter parameters into strings and
//! falling prey to injection attacks

#define REFLECT_PYTHON(name) static boost::python::object name = GET_PYTHON<boost::python::object>(#name);
#define REFLECT_PYTHON_ALTNAME(name, altname) static boost::python::object altname = GET_PYTHON<boost::python::object>(#name);


#define PYTHON_SCRIPT_DIR "src/python/"

//! Run a python file
void EXEC_PYTHON_FILE(std::string filename);


//! Expose a function to Python, so that it may call it through CModule.X
#define exposeToPython(python_name, c_func) \
try                             \
{ \
    REFLECT_PYTHON( expose_function ); \
    expose_function( python_name, boost::python::make_function(c_func) ); \
} \
catch(boost::python::error_already_set const &)                \
{                                                              \
    printf("Error in Python execution of exposeToPython\r\n"); \
    PyErr_Print();                                             \
    assert(0 && "Halting on Python error");                    \
}

