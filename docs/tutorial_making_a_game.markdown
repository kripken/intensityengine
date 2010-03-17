Tutorial: Making a Game, Quickly (Locally/Standalone)
=====================================================

The Intensity Engine lets you play multiplayer games, and even **create** them in a multiplayer, networked way. However, if your goal is to make a singleplayer game, or you don't want to run with a master server, or you just want to create your game locally, then this tutorial will show you how to get up and running fast.

(The other way to do this is to use a master server, which lets multiple people work together on creating content easily, has a user account system, assets, etc. For that, see README-standalone.txt)

Preparations:

* Compile the engine - see COMPILE.txt for instructions.
* It's a good idea to get familiar with the server_runner plugin. It is explained in README-standalone.txt. For example, run the storming_test map mentioned there, to see that the engine has been built properly, and runs correctly.

Now let's get starting with creating a game!


Home Directory
--------------

Our game content will be stored in a packages/ directory, inside a home directory. For simplicity for the home directory let's use mygame/ inside the install dir. That means that, when you are distributing your game to people, you need to give them both the engine, and the contents of mygame/.

To run the engine using that directory, run the following command from the install dir:

    ./intensity_client.sh mygame -config:Components:list:intensity.components.server_runner

or on Windows,

    intensity_client.bat mygame -config:Components:list:intensity.components.server_runner

Note that you can create a shell script or a batch file to run the commands just mentioned, for your convenience, so you can just doubleclick on that to run things.

The first time you run those commands, it will create the directory, and some configuration files. Leave them be for now.


Creating Content
----------------

Inside mygame/, which has been created, you can now begin to set up your game content. Create a 'packages' folder if there is not already one there, and inside packages/ create a 'base' directory, which is where maps are stored. Inside base/, create a folder called 'mymap' for your first map.

Into mygame/packages/base/mymap/, place the contents of master_django/intensity/tracker/fixtures/emptymap.tar.gz. That archive contains files for an empty map. So, your mymap/ folder should now contain the following:

* entities.json
* map.js
* map.ogz

Let's run the map. Run the client again if you closed it, with the same command as before, and tell the server_runner plugin to run **mymap**. You should now be inside the map, which doesn't contain anything interesting yet. You can enter edit mode and start building the map.


Saving Your Work
----------------

After doing some changes, you can save them using the server_runner plugin. In the GUI, go to 'plugins...' and click on 'save map'. This saves your work (where you would expect, in mygame/packages/base/mymap/).


Workflow
--------

When you change the map scripts, you normally need to restart the map for that to take effect. To do so, the simplest way is to use the server_runner plugin GUI, which has a menu item for restarting the server.


Getting Started
---------------

See mapping.markdown for mapping, and scripting.markdown for scripting.


Advanced Topics
---------------

* You can use a server plugin that calls set_map to switch to another map, when the player completes the current one. For something similar see the map_control plugin in src/python/intensity/components, and see packages/library/1_3/mapscripts/swarm.js for a map that uses it (search for signalComponent).

TODO: Expand on these

