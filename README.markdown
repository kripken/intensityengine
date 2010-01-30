Intensity Engine
================

The Intensity Engine is the open source project powering Syntensity,

    http://www.syntensity.com
    http://www.syntensity.com/toplevel/intensityengine/


Running - Syntensity
--------------------

To run the client and connect to Syntensity, run

    (Windows) intensity_client.bat
    (Linux)   ./intensity_client.sh

You need to sign up for a free user account on Syntensity,

    http://www.syntensity.com

Notes:

  - On Windows, Python25 appears in the batch file for the
    client (and also the server, etc.). This is because the
    binary libraries provided (boost, etc.) are linked against
    Python 2.5. So, you should install Python 2.5 (it can
    coexist alongside other versions), or edit the batch
    files. (Note that only 2.5 was tested extensively on Windows,
    so there may be issues with other versions, but, 2.6 is known
    to work fine on Linux.) Also, if you have Python 2.5
    installed at a non-default location (i.e., not
    C:\Python25) then you should edit the batch files
    accordingly.


Compiling
---------

See COMPILE.txt for how to compile the source code, if this
isn't a binary distribution.


Running - Locally (i.e., without Syntensity)
--------------------------------------------

See README-standalone.txt.


Settings
--------

You can change some settings in the settings.cfg file. This file is
in

    ~/.intensityengine_client/settings.cfg

or

    ~/.intensityengine_client/settings_server.cfg

on Linux, for the client and server, respectively - note that these
are purposefully two separate files. On Windows, the files are in

    C:\Documents and Settings\USERNAME\intensityengine_client\settings.cfg
    C:\Documents and Settings\USERNAME\intensityengine_client\settings_server.cfg

where USERNAME is your username. On Vista, the locations will be

    C:\Users\USERNAME\intensityengine_client\settings.cfg
    C:\Users\USERNAME\intensityengine_client\settings_server.cfg

Note that the files are auto-generated when the client exits, so if
you modify it while the client or server is running, then exit, your
changes will be overwritten.

You can also tell the client and server to use other home directories.
They will then use the files present there, and in particular the
settings.cfg file there, so this can be an easy way to switch between
various setups (e.g., between connecting to Syntensity or to your
own infrastructure). To specify a home directory, simply pass it
as the first commandline argument to the client or server.


Problems
--------

See the TROUBLESHOOTING.txt file if you are having trouble running or
compiling the Intensity Engine. You can also ask for help on IRC,

    #syntensity on FreeNode

See also

    http://wiki.syntensity.com/troubleshooting


Documentation
-------------

See the included README-* files, the contents of docs/,  and the
Syntensity wiki:

    http://wiki.syntensity.com


Reporting Bugs
--------------

Please do so here:

    http://github.com/kripken/intensityengine/issues

Notes on reporting crashing bugs:

  - Set the logging level to DEBUG (or INFO, if feasible), and
    reproduce the crash. This will give more output than the
    default setting of WARNING. (This is set in the settings.cfg
    file in the home directory you are using.) On Windows,
    output should appear in out_client (or out_server); on other
    platforms, it goes to the console by default, so you
    should redirect it to a file (or copy it from the console.
  - If you can, try to run the program in the debugger, to find
    the line number of the crash.
  - In the bug report, please state if the crash is consistent
    (always happens in a particular case), and what exactly
    appears to trigger the crash - what you were doing just
    before, etc.


Packaging / Systemwide Install
------------------------------

The Intensity Engine will work fine if installed for all users. It
only writes to the user's home directory, not anywhere else, so it
can be installed in a read-only central (systemwide) location. The
only thing you should do in such a case is change directory to
where it is installed and run it from there.


Notes
=====

1. The Intensity Engine is licensed under the GNU Affero General
   Public License (GNU Affero GPL), see LICENSE.txt. For more about
   licensing, visit our website, www.intensityengine.com.
   (Some directories contain files under other licenses. See the
   license descriptions in each such directory.)
2. For more information, including links to the mailing list,
   bugs, wiki, etc., see the main website,

        http://www.syntensity.com/toplevel/intensityengine/

3. This distribution may contain sources or binaries of other open
   source projects (in /build, /src, or /windows/dll), supplied for
   convenience here instead of requiring you to get them yourself.
   The following is a list of licenses and where to get the source
   code for each one:

    - ENet (MIT) http://enet.bespin.org/
    - Simple DirectMedia Layer, or SDL (LGPL) http://www.libsdl.org/
    - Google V8 (BSD) http://code.google.com/p/v8/
    - The Python programming language (PSFL) http://python.org/
    - Boost (Boost Software License) http://www.boost.org/
    - MochiKit (MIT) http://mochikit.com/
    - zlib (zlib) http://www.zlib.net/

   The Intensity Engine uses all of these project as-is, no changes
   have been made.

