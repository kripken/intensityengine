Getting started with your own Intensity Engine server infrastructure
====================================================================

This document details how to run your own server infrastructure:
master server, asset server(s) and server instance(s).

First, check that you can run the standalone setup as described
in README-standalone.txt. Once you have that working, you can
proceed to set up your own infrastructure as follows.

It is *not* a good idea to modify the files in local/, which are
used for the standalone demo in README-standalone.txt, because
these are version controlled. In other words, they are part of
the Intensity Engine source code, and may change. Instead, you
can make a copy of local/, and work with that.

To run the copy, run all the components with the home directory
set to the new directory you created when you duplicated local/
- simply replace 'local' in each command, for example,

    python minimaster/minimaster.py local/master_server

would become

    python minimaster/minimaster.py YOUR_DIR/master_server

where YOUR_DIR is the duplicate you created.

You can now edit the settings.cfg files in each of these
directories, to set things up the way you want. Some tips:

  * For the master server, make sure that map_location is
    the name of the asset URL, without the path and
    .tar.gz. For example,

        map_location = storming

    will work with

        asset_url = http://localhost:8090/files/storming.tar.gz

    The reason for this is that the asset server assumes
    that connection by default, with the mini-master. If
    you are using a more complex master then this might not
    be needed.

  * Setting 'localhost' as the address of a server (master,
    asset or server instance, including URLs of assets) means
    that only local users will be able to access them (as they
    will try to find them at 'localhost'). Enter an appropriate
    web address (IP address or host name) if you want remote
    users to be able to connect.

  * Make sure not to use the same ports in the master server,
    asset server and server instances. They should of course
    also not conflict with ports already being used by other
    programs on the same machine, and should be accessible
    (not blocked by your firewall, appropriately forwarded
    if you want remote people to be able to connect, etc.)

  * You can have lots of different 'profiles' present on
    the same machine, using separate home directories. For
    example, you can have one directory set up so your client
    can connect to one master server, and another directory for
    another. Simply create separate directories for each and
    edit the settings.cfg files in them accordingly.

