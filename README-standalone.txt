Running the Intensity Engine in Standalone mode
===============================================

Brief Overview:
===============

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


Notes
=====

1. The example above runs the master on the Django development
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

