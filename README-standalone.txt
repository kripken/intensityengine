Running the Intensity Engine in Standalone mode
===============================================


The simplest way to use the Intensity Engine is to run
the client and connect to remote servers, like syntensity.com.
That process is explained in README.txt. However, the Intensity
Engine is very flexible, and you can also use the engine in
other ways. One is to run a local server (see README-server.txt),
and another is to run 'standalone', which this document explains.

'Standalone' means to run *without* depending on other servers -
you will be in charge of everything yourself. There are several
options here: You can either run your own servers locally,
or you can run the client without any servers at all. Some
reasons you might want to do these sort of things include:

  * You just finished compiling the engine, and you just
      want to quickly see something neat.

      See 'Running games using only the client', below.

  * You are creating a singleplayer game, and don't need
      remote servers, user accounts, asset systems, etc.

      Ditto: See 'Running games using only the client', below.

  * You want to run a demo on your laptop, and you might
      not have network connectivity.

      Ditto: See 'Running games using only the client', below.

  * You want to set up your own syntensity.com-like system,
      with your own user accounts, assets, etc.

      In this case you can set up your own master server
      and game servers, as explained in 'Running your own servers',
      below.


Running games using only the client
===================================
a.k.a
Running a local server using the server_runner plugin
=====================================================

This is an easy way to run a map, without manually setting up
a server - the plugin will do it all for you. If you just finished
compiling the engine and want to try stuff out, this is a good
way to do that.

    * Activate the plugin: The simplest thing is to run the client as
        follows:

            ./intensity_client.sh -config:Components:list:intensity.components.server_runner

        or on Windows,

            intensity_client.bat -config:Components:list:intensity.components.server_runner

        Alternatively, you can add intensity.components.server_runner to
        "[Components] list" in your settings.cfg, which is in your
        home directory, which is intensityengine_client or .intensityengine_client
        under your operating system user's home directory - Users on Vista,
        or under Documents and Settings on XP, or ~ on Linux, etc.

        Note that the directory and file will be created the first time
        you run the client. So, run it once and close it, then edit the file
        as explained here.

        The relevant lines in your settings.cfg should look like this:

            [Components]
            list = intensity.components.server_runner

        (without the indentation).

    * (OPTIONALLY, log in to the master. If you don't have a reason
        to, don't do this - explanations are below if you're curious.)

    * Click on 'plugins...' in the main menu. You will then see the
        status of the local server in the plugin GUI.

    * Tell the plugin which map to run (simply by writing the location
        of the map, e.g. "storming_test" for the test map). Then click start,
        and wait a bit while the server is started up for you. As soon as
        it is ready you will automatically connect to it.

        The storming_test map is included with the source code, so it is
        ready for you to run it, and a good first map to try out. Note
        the "_" (underscore) in storming_test when you type it in the GUI.

        Aside from that map, you can also try the maps on syntensity.com.
        For them you will need to log in to the master. After doing that,
        the following maps can be run:

            sketchworld
            racetrack
            welcome
            gk/ctf
            etc.


    Master vs. Masterless
    ---------------------

    If you *do* log in to the master, you can tell the plugin to run
    any map and it will fetch it from the master if it isn't present,
    and if it is, then update it to the latest version (if necessary).

    Whereas if you do *not* log in to the master, whatever map you ask
    to run will be run directly from the disk - whatever files are
    in that map directory will be run, and likewise all map content
    (models, textures, etc.) will simply be loaded and run, without
    using the asset system to check for dependencies, updates, etc.


    Working on maps
    ---------------

    There is no convenient GUI way to create a new map at the moment, when
    working locally. Instead, just create a folder in

         ~/.intensityengine_client/packages/base

    for your map, and copy-paste files from another map into there. For
    example, files for an empty map are in the archive at 

        ./master_django/intensity/tracker/fixtures/emptymap.tar.gz

    Once that is set up, run the map normally, giving the name of the folder
    you just created.

    To save the map, do 'save map' in the server runner plugin GUI.


    Notes
    -----

    The server will be shut down automatically when you close the client,
    or connect to another server, so no need to worry about that. You
    can also shut it down in the GUI if you want (which is necessary to
    start it up with a different map).


Running your own servers
========================

To run locally, for example right after compiling,
run the master server and a server instance,
 
       ./intensity_master.sh local/master_server/

       (when asked, press enter to create the default 'test' user.
        If, separately, Django offers to create a superuser,
        that is optional)

       ./intensity_server.sh local/server/

and leave both of them running. Note that for the
master server you will need to have Django 1.0.x installed,
which is bundled in master_django/django.
(Note that Django 1.1+ currently does *not* work).
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Then run

       ./intensity_client.sh local/client/

and log in with username 'test' and password 'secret'
(without "'"s). Then select "local connection..." and
you should see a map load and run, which you can then
explore.

On Windows, the same commands work, but remove "./" from
the start of each command, reverse the slashes from "/"
to "\", and replace .py and .sh with .bat, that is,

       intensity_master.bat local\master_server\

       intensity_server.bat local\server\

       intensity_client.bat local\client\


Full Overview
=============

This document details how to run a map locally, without an external
master server like Syntensity. If you just downloaded (and possibly
compiled) the Intensity Engine, this might be the first thing you
want to do, to test it (or, you can sign into Syntensity).

The following commands should work out-of-the-box given the
files in the Intensity Engine code repository. Perform all these
commands from the directory in which you installed the code.


Steps:
======

1. Start the master development server, using

       python intensity_master.py local/master_server/

   or, on Windows,

       python intensity_master.py local\master_server\

   The master server will use the parameters specified in
   local/master_server/settings.cfg.

   When asked, press enter to create the default user,
   which you will use later to log in with. You may also
   be prompted to create a Django superuser, doing so is
   not necessary.

   The master server is a Django application, and requires
   that Django 1.0 or later be installed, e.g., on Ubuntu
   this can be done with

        sudo apt-get install python-django

   Or, more generally you can download Django, and either
   install it system-wide, or place it in a directory
   called 'django' under master_django.

2. Start the server instance (i.e., the 'game server' that runs a particular
   game/activity/scenario), using

       ./intensity_server.sh local/server/

   or, on Windows,

       intensity_server.bat local\server\

   This starts the server instance, with its home directory set to
   local/server, which already contains an appropriate settings.cfg
   file, which in particular tells it to look for the master server
   on the same machine (as we are currently setting up).

3. Start the client, using

       ./intensity_client.sh local/client/

   On Windows, do

       intensity_client.bat local\client\

4. Log in with

       username: test
       password: secret

   This user was created automatically when running the master server
   for the first time.

5. Do 'localconnect' to connect to the server.


You should now have entered the map running on the server. Have fun!


Other Clients
=============

Other people can also connect to the infrastructure you set up. To
do so, they need to

1. Edit settings.cfg in their client's home directory, so that it
   refers to the right master server.

2. Instead of doing 'localconnect', they should select
   'manual connection' and enter the correct server information.
   Or, this can be done in a more manual manner by pressing '/',
   which allows a command to be entered in the client, and do

       /connect IP PORT

   where IP is the IP address of the server instance, and PORT
   is the port (which is 28787 by default). For example, if
   the server instance is on a machine with address 42.4.2.42
   and the port is left at the default value, then

       /connect 42.4.2.42 28787

   will work.
 

Editing
=======

You can edit the map, even in collaboration with others. Note
the following matters:

    - Clients can edit the map, and do 'upload map',
      which sends it to the master server, and from there
      will be downloaded by the server instance and the
      connected clients. If you want to save backups of
      your map, or have a version control system on it,
      etc., then the best place to do that is on the
      asset server, where it stores its files (see the
      master server documentation).


Production
==========

   The example above runs the master on the Django development
   server. This is *NOT* suitable for production, for
   several reasons: It is single-threaded, which will
   both lead to poor performance and prevent operations
   like requisitioning server instances from working, and
   it also hasn't been security audited (Django is, as they
   say, in the business of web development, not web
   servers). Instead, you can run the Intensity Engine
   master server on a production-ready webserver
   like Apache, lighttpd or CherryPy. A script for
   CherryPy is included, master_django/prod_server.py,
   which is very simple to run (whereas Apache, lighttpd
   etc. will require system-wide configuration, root user
   access, etc.).

   To run the CherryPy production server, do

       python intensity_master.py local/master_server/ prod

   (i.e., add the parameter 'prod'). If in addition you
   want to serve the website using SSL (https), then
   edit the parameter 'auth' in the settings.cfg. This will
   look for cert.crt and cert.key files in master_django/ssl/.

   You should also change the settings to the production
   settings. In the master server's settings.cfg, set it to

        [Django]
        settings = settings_production

   Note that you must change both this, *AND* add 'prod'
   as a parameter when running the master. The former sets
   up the django parameters for production mode, and the
   latter runs Django using CherryPy for production purposes.


Requisitioning servers
======================

You can requisition servers if there is a pool of available
servers. Those should be servers running against your
master server, that are identified as being in the pool -
otherwise, they are 'independent', and not available for
requisitioning.

To be identified as being in the pool, they must authorize
themselves with the master (this prevents just anybody
from adding a server to the pool, since those servers
might not behave like a pooled server should). For that,
set

    [Network]
    instance_validation = CODE

in the server's settings.cfg, where CODE is something
that will be sent to the master. On the master, you must
connect to the validate_instance signal, with something
that returns True if the instance_validation code is
valid. For example, this would work, if put into
a file called master_django/intensity/components/instance_validator__models.py

    __COMPONENT_PRECEDENCE__ = 100
    from intensity.tracker.signals import validate_instance

    def do_validate(sender, **kwargs):
        instance = kwargs['instance']
        validation = kwargs['validation']
        if validation is None: return False
        return validation == 'CODE'
    validate_instance.connect(do_validate)

(This is a simple plugin that hooks into validate_instance.)
Note that __models.py is important in the filename. Note also
that the indentation appearing in this README file will cause
errors in an actual Python file (i.e., 'def' should not
be indented at all, etc.).

Note that for server requisitioning to work, the master
must *not* have [Instances] force_asset_location set (if
it does, then all servers will be told to run that
asset, instead of requisitioning being able to control
them).


Running without a master
========================

It is also possible to run without a master at all. To do so,
in settings.cfg fill in [Activity] force_location to be equal to
the location of the map to load. The map will be loaded from
/packages directly, and the server will tell the client to do
the same.

For example,

[Activity]
force_location=base/mymap.tar.gz

