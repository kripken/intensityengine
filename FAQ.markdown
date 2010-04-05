TODO
====

Creating
========

Running a local server works, but local servers fail...?
--------------------------------------------------------

There must be a difference between the two. First check that they are running the same engine version. Otherwise, most likely you have some files locally that are available for local servers, but remote servers are missing. Those files should be packaged as assets, so remote servers - and other people's clients - can access them. To test if that is the case, try running a local server with a **NEW** home directory, one which does not contain your usual local asset files, with something like

    ./intensity_server.sh test_home_dir

That will create test_home_dir/ and run the server with those files. Since it is empty, it will download new asset data from the master server, exactly as a remote server would. You can then see the errors it encounters and fix those.


