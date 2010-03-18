Scripting Docs
==============

NOTE: scripting_docs.html is an older documentation, but it might still be relevant and useful.


Overview
--------

This document will briefly explain Intensity Engine scripting, then proceed to go through a series of concrete examples, in order to explain the concepts in a hands-on manner.


Basics
------

* When a map is loaded, its map script - map.js - is loaded. The map script is run once. The map script sets up various things for the map, like textures, entities, gameplay logic, etc.
* Scripts are written in JavaScript.
* The map in particular defines Logic Entity classes. Logic Entities are the basic entities in the world, that are synched between clients and servers. For example, there is a Logic Entity for each player, and for each mapmodel, etc. Each Logic Entity has a class (in the sense of object-oriented programming), and inheritance is possible, but also a component/plugin approach, which is better for some things.
* Logic Entities have State Variables, which are variables that are automatically synched between clients and the server. If you change the value on the server, it will transparently propagate to the clients, and if you change them on a client, it will transparently send a change request to the server, which will then process it and update the clients. From the programmer's perspective, for the most part State Variables look like normal JavaScript variables. The dynamic aspects of JavaScript are used to do the transparent synching behind the scenes.
* Each frame, the engine will run scripts, through manageActions. This will basically let each Logic Entity run a script for itself. As such scripts are run in each frame, they must be brief (long-running code must be split up into small tasks).


Examples
--------

The docs/script_examples/ directory contains some examples of working scripts. You can look at each of them and then read the comments below, which explain what the code does.

* *0_hello_world* - A very short 'hello world' example, just enough to get running. It does the following:
  * Set up default materials and so forth.
  * Set up some textures for world geometry.
  * Define a player class, a subclass of 'Player'. No new functionality is added, just a new class name ('GamePlayer').
  * Define the application - the object that defines various 'entry points' into the map code. Here we just fill in the getPcClass, which is what is called to get the name of the player entity class.
  * Load permanent map entities (could be lights, mapmodels, particle effects, etc.). We simply read a JSON file with that data, and call loadEntities.

* *1_recommended_template* - Contains some additional things that are optional (our approach is 'everything is a plugin'), but highly-recommended for basically every map.
  * Uses the standard library, version 1.3, and imports some modules from there:
    * Plugins: Makes it easy to extend entity classes
    * Health: Defines spawning, dying, etc.
    * GameManager: A class from which we create a singleton, that is 'in charge' of the game, controlling things like teams, score, and also other 'global' things. 
  * We manually define some map settings - fog color, sky, shadows, etc.
  * We define the player class using the plugin system. Specifically we 'bake' into our new player class two plugins, for health and for the game manager. This enables the necessary things from those modules on our players.
  * We define what to do when we fall off the map: Health.dieIfOffMap will kill us, and respawn us a little later.
  * We define the 'scoreboard text', which is shown when pressing TAB.
  * We set up the game manager class. The game manager can also have plugins. In this case we enable two very useful ones:
    * messages: Lets you send messages over the network for display on the clients
    * eventList: Manages a list of events/handlers, for scripts that run at certain times or intervals. Many library modules require this, and it is recommended to use it. Later example scripts will show how.
    * On the server, the game manager registers the teams. In this case we have just one team, 'players', and everyone will belong to it. The default model for those players (i.e., everyone) is 'stromar'.

