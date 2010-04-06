Running an Intensity Engine server
==================================


***
Note that the easiest way to run a server for personal, local use is with the server_runner
plugin, which is documented in README-standalone.txt. This document
gets into much more detail, about how to do it manually.
***


Why run your own server?
------------------------

It is sometimes useful to run a local server instance, that is, a server instance on the same machine you are running the game client on. Reasons you might want to do this include:

    * Lets you change the map scripts without uploading them to a remote server. This is useful because you may need to edit the scripts quite a lot during debugging and so forth, and because uploads take time.
    * You have more direct control and contact with the server - so you can see how much CPU it's using, stuff it logs to the console (like errors and warnings), and so forth.
    * You don't need to find a free remote server instance.
    * You can use some editing commands that require that you be the only client logged into the server, like heightmaps. However, you can also do this on a remote server, by selecting 'request private edit mode...' in the client menu.

There are some disadvantages, however:

    * With the client and server on the same machine, you will not notice network delays, because communication is pretty much instant. This means that it is not possible to debug 'responsiveness' issues in your maps, that is, stuff that makes the game responsive even though some of the logic is on remote machines.
    * You need to set up the server, perhaps telling your firewall to let it use the ports it needs, and so forth.


Running the server
------------------

Usage:

    ./intensity_server.sh [HOME_DIR]

or

    intensity_server.bat [HOME_DIR]

on Windows.

HOME_DIR is the directory for transient files, that are not
part of the main installation. The settings/config file goes there,
as do downloaded assets. If not specified, the config file
and HOME_DIR are:

    ~/.intensityengine_client/settings_server.cfg

On Windows XP, it would be under C:\Documents and Settings and then
your user, in Vista it would be under C:\Users and then your user, 
in both cases without a '.' before intensityengine.

Note that the directory is
the same as the client home dir, because that way the client
and server can share asset files, while still having separate
config files (settings.cfg and settings_server.cfg).


Settings
========


    * After running the server for the first time, a settings file will appear in the home directory. There are several parameters of interest there (we list them as [Category] / Element):
          o [Logging] / level: How much output to log to the console. Use DEBUG for more information, INFO for a lot more information. Note that these make the server run slower as well.
          o Versions 1.0.2 and earlier:
                + [Activity] / force_map_asset_id: The UUID (identifier) of the map to run. To find it, you can view the map asset (not the activity) on the main website. The last part of the URL is the UUID, for example,

                  http://www.syntensity.com:8888/tracker/asset/view/06c9b70b_14d7_4afc_bd63_56588670c1ef/

                  (so, 06c9b70b_14d7_4afc_bd63_56588670c1ef in this case).

                + [Activity] / force_activity_id: Setting this is optional; if set, the activity the server is running will be listed correctly on the master's list of server instances. If you do set it, this should be a UUID for the activity, which you can find like finding the UUID for the map asset (but when viewing the activity).

                + Note: If there isn't an [Activity] section, simply add one.

          o Versions 1.0.3, trunk, and later:
                + [Activity] / force_activity_id: You can either place an actual activity ID (as described above), or simply paste the full URL to the activity, from the website. Such URLs look something like

                  http://www.syntensity.com:8888/tracker/activity/view/afe3c7e1_caeb_4320_9116_6e2ae8ad6242/

                  You can find the URL by clicking on the activity on the activities page.

                + Note that you don't need to set force_map_asset_id, it will be discovered automatically based on the activity ID. (However, you can still define it, and not define force_activity_id, for backwards compatibility.)

                + Note: If there isn't an [Activity] section, simply add one.

    * After setting the activity information, run the server. Then run the client, log into the master, and select 'local connection...' to connect to your server.
    * You should now be able to work on your map.

Notes:

    * While the server is local, you are still logged in to the Syntensity master server. This means that if you do 'Upload map...' it will be uploaded to the master.
          o Note that uploading the map will update it for everyone, as you are uploading it to the master server. The master server won't let you do that if you aren't the owner of the asset, in that case the upload will fail with an error.
    * For smaller edits, you can do the following:
          o Change the map files for both the client and the server, then run them. For version 1.1 and above, by default they will use the same map files (both use the client home directory, as mentioned previously), so this is done automatically. For older versions, see the following:
                + The client's map files are in the client's home directory, under packages/base/mapname/, and the server's are in the server's home directory, also under packages/base/mapname/, where 'mapname' is the name of the map.
                + Any changes made to both those map directories will take effect when you work locally. This lets you be 'out of sync' with the remote master server, when you are testing local changes.
                + Note that if you change just the client's files, or just the server's - or change both, but don't restart the server - then, with the client and server not running the same code, strange things may happen.
                + Note also that if you change the server's map.js, and do 'upload map' from the client, then the client will upload the old version. After the upload to the master server, both client and server will get the latest version from the master, which will have the old map.js - overriding what you changed in the server's map.js.
                + One helpful approach here is to use a symlink (on operating systems that support that), so that the client and server are really using the same /packages directory, even though they have different home directories.
    * If you do 'restart map', the following happens:
          o The scripting engine running the map scripts is destroyed and re-created. The current map.js and other scripts will then be used, so if you just edited them, you will be running that version (probably what you want). Note that you do not need to shut down the entire server and start it up again to run that latest code - 'restart map' is enough.
          o Note that 'restart map' doesn't save entities or any changes to the map geometry (it warns about that). Doing so will lose all those changes, if there are any.



Manual setting
==============

To manually start the map on the server, in the server console do

    set_map('?', MAP_ASSET_ID)

The first parameter is the activity ID, which is unimportant in local
connections (anything will work there). The second should be the asset
ID of the map you want to load (you can find this by looking on the
master server for the asset - select 'Edit' on that asset, and it
will show the asset details, including the asset ID. It will also
appear in the config.cfg file of the client, if you last uploaded to that asset,
by name 'last_uploaded_map_asset').


Running a server for many people to connect to
==============================================

The following config settings are important:

*  [Network] / address: The IP address or hostname (for example, www.yoursite.com). If you do not specify this, only people on the same physical machine will be able to connect to you. So you must set the right value here for the server to let other people play on it.
* [Clients] / limit: The maximum number of clients you want to allow to play at one time.
* [Components] / list: A optional list of components (plugins) to run. See src/python/intensity/components for those available; you can also write your own. For example, if you add intensity.components.bandwidth_watcher to the list, the server will monitor itself for excessive bandwidth use. See src/python/intensity/components/ for some available components, which are often documented in the source code. You can also easily write your own. Some existing useful components (in src/python/intensity/components) are:
  * shutdown_if_empty: Shuts down the server if, after it has been entered, it ends up empty. In combination with run_server_forever, will let the server restart, ready for new requisitionings.
  * shutdown_logger: With run_server_forever, will mark when it is shut down gracefully, and if not, when next started it will upload a crash log to the master.
  * cpulimiter/cpu_watcher/memory_watcher/bandwidth_watcher: Can limit resource usage. Useful if you let people run their own activities. Some of these are Linux-only, sorry.
  * irc: Sends and receives data from IRC into the game.
  * local_storage: Save data in a persistent local database.
  * map_control: Lets maps restart themselves from scripts.
  * client_validator: Only let certain people login to the server.
* [Network] / instance_validation: A code sent to the master, that 'proves' it is an official server, and can be added to the list of 'Pooled' servers. If invalid, or not provided, the server will be listed as 'Independent'.


run_server_forever.sh
=====================

Usage:

    ./run_server_forever.sh HOME_DIR

or

    run_server_forever.bat HOME_DIR

on Windows.

HOME_DIR is passed to intensity_server.sh, see above for use.

run_server_forever.sh is a Bash script that will run the server in
a continuous loop. This is useful as if the server crashes, on
its next run it will place itself in 'standby mode' (during which
it won't load maps until manually repurposed - because perhaps the
last map causes the crash), and it uploads the crash log to the
master server.

Server output will be logged to HOME_DIR/out.log, and will
be saved in HOME_DIR/out.log.old when the server restarts.



Running without a master
========================

It is also possible to run a server without a master at all. To do so,
in the settings file fill in [Activity] force_location to be equal to
the location of the map to load. The map will be loaded from
/packages directly, and the server will tell the client to do
the same.

For example,

[Activity]
force_location=base/mymap.tar.gz

(This is used internally by the server_runner plugin.)

