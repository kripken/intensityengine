
// Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
// This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

// In the future, we might interface with Python in other ways, apart from Boost::Python. PyPy in particular
// would be interesting, for speed reasons. They are however not ready yet to interface with C++ in general:
//   http://morepypy.blogspot.com/2008/10/sprint-discussions-c-library-bindings.html


#include "cube.h"
#include "engine.h"


using namespace boost;

bool pythonInitialized = false;

python::object python_main_module;
python::object python_main_namespace;

void initPython(int argc, char **argv)
{
    assert(!pythonInitialized);

    try
    {
        Py_Initialize();
        python_main_module    = python::import("__main__");
        python_main_namespace = python_main_module.attr("__dict__");
    }
    catch(python::error_already_set const &)
    {
        printf("Error in Python initialization\r\n");
        PyErr_Print();
        assert(0 && "Halting on Python error");
    }

    pythonInitialized = true;

    // Set up general stuff to allow Python to be used
    EXEC_PYTHON("import sys, os");
    // TODO: remove sys.path in binary builds, leave only a tiny subset of all of CPy
    #ifdef INTENSITY_INSTALL_ROOT
        printf("Changing directory to install root: %s\r\n", INTENSITY_INSTALL_ROOT);
        EXEC_PYTHON("os.chdir('" + std::string(INTENSITY_INSTALL_ROOT) + "')");
    #endif
    EXEC_PYTHON("sys.path = ['', 'src', os.path.join('src', 'python') ] + sys.path");
    EXEC_PYTHON("print 'Python path:', sys.path");
    EXEC_PYTHON("from intensity.c_module import *");

    #if 0 //def WINDOWS - TODO: Probably need to place in BAT file
        // For Windows, use pre-prepared DLL files in windows/dll
        EXEC_PYTHON("os.environ['PATH'] = os.path.join(os.getcwd(), 'windows', 'dll') + ';' + os.environ['PATH']");
    #endif

    // Pass C commandline arguments untouched to Python
    try
    {
        boost::python::list args;
        for (int i = 0; i < argc; i++)
            args.append(std::string(argv[i]));
        REFLECT_PYTHON( set_python_args )
        set_python_args(args);
    } catch(boost::python::error_already_set const &)
    {
        printf("Error in Python execution of setting args\r\n");
        PyErr_Print();
        assert(0 && "Halting on Python error");
    }
}

void EXEC_PYTHON_FILE(std::string filename)
{
    assert(pythonInitialized);

    std::string fullname = std::string(PYTHON_SCRIPT_DIR) + filename;
    try
    {
        python::object dummy_ignore_wasdy = python::exec_file(fullname.c_str(), python_main_namespace, python_main_namespace);
    }
    catch(python::error_already_set const &)
    {
        printf("Error in Python execution of file: \r\n%s\r\n", fullname.c_str());
        PyErr_Print();
        assert(0 && "Halting on Python error");
    }
}


void run_python(char *code)
{
    EXEC_PYTHON(code);
}
COMMAND(run_python, "s");


// TODO: The following shows how we might embed much faster:
// from examples in the boost::python reference for exec (/execfile)
// - basically, instead of parsing "greet()" again and again, save an object pointing to it. Can thus do
//   source code parsing once, and work much faster later
/*
#include <iostream>
#include <string>

using namespace boost::python;

void greet()
{ 
  // Retrieve the main module.
  object main = import("__main__");
  
  // Retrieve the main module's namespace
  object global(main.attr("__dict__"));

  // Define greet function in Python.
  object result = exec(
    "def greet():                   \n"
    "   return 'Hello from Python!' \n",
    global, global);

  // Create a reference to it.
  object greet = global["greet"];

  // Call it.
  std::string message = extract<std::string>(greet());
  std::cout << message << std::endl;
}
*/
