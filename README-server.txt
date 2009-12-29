Running an Intensity Engine server
==================================


intensity_server.sh
-------------------

Usage:

    ./intensity_server.sh HOME_DIR

or

    intensity_server.bat HOME_DIR

on Windows.

HOME_DIR is the directory for transient files, that are not
part of the main installation. The settings.cfg file goes there,
as do downloaded assets. If not specified, HOME_DIR is equal
to ~/.intensityengine_server on Linux and an appropriate
subdirectory (typically under DOCUMENTS/ etc.) with the name
intensityengine_server in Windows.

Notes:

    - Without an instance ID in the settings.cfg file, the server will
      assume it can't register itself to a master server anyhow, and not
      even try.


run_server_forever.sh
---------------------

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


Running a local server
======================

(See also README-standalone.txt)

If a server instance is started without an instance_id, it places itself
in 'local mode'. In this mode it will only accept a login from the
same machine. That single client can then edit the map using commands
not possible in multiplayer, like heightmap editing.

To start the map on the server, in the server console do

    set_map('?', MAP_ASSET_ID)

The first parameter is the activity ID, which is unimportant in local
connections (anything will work there). The second should be the asset
ID of the map you want to load (you can find this by looking on the
master server for the asset - select 'Edit' on that asset, and it
will show the asset details, including the asset ID. It will also
appear in the config.cfg file of the client, if you last uploaded to that asset,
by name 'last_uploaded_map_asset').

To connect using the client, first login to the master. Then select
'local connection...' from the menu,
or do '/' to start entering a command, and enter

    /connect 127.0.0.1 28787

If you changed the port from 28787, enter the correct port. If
you changed the port, you will need to connect manually using
the /connect command instead of the menu option.

Note that clients should connect *after* calling set_map.

