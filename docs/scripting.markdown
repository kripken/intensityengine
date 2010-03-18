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

* __0_hello_world__ - A very short 'hello world' example, just enough to get running. It does the following:
  * Set up default materials and so forth.
  * Set up some textures for world geometry.
  * Define a player class, a subclass of 'Player'. No new functionality is added, just a new class name ('GamePlayer').
  * Define the application - the object that defines various 'entry points' into the map code. Here we just fill in the getPcClass, which is what is called to get the name of the player entity class.
  * Load permanent map entities (could be lights, mapmodels, particle effects, etc.). We simply read a JSON file with that data, and call loadEntities.

* __1_recommended_template__ - Contains some additional things that are optional (our approach is 'everything is a plugin'), but highly-recommended for basically every map.
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

From here on, the examples all build upon __1_recommended_template__.

* __2_change_speed__ - Changes the speed of the players.
  * We add an 'init' function to our player class. 'init' is called once, when the entity is created (on the server). Here, we set the movement speed to 100 (which is btw the speed in sauerbraten).
  * There are 5 other important functions we can add in our plugins, that get called at special times:
    * activate: When the entity is 'brought into action'. init is called once on creation, activate is called whenever we call up the entity, for example after saving and loading it (init is not called in such cases).
    * clientActivate: Called on the **client** when activated. Note how you write code for both the client and server, in the same map script. In many cases the same code runs on both (all the other code does that), but sometimes like with clientActivate we need to specify things otherwise.
    * act: Called every frame.
    * clientAct: Called every frame on the client.
    * renderDynamic: Called to render the object (usually called several times per frame, for shadow mapping passes, etc.).

* __3_plugin_example__ - A simple 'run out of breath' plugin: If you run for too long without stopping, you lose health.
  * RunOutOfBreathPlugin is simply a JavaScript object that contains some attributes and functions. When the plugin is 'baked in', those are added to the class we are creating.
    * maxBreath is a constant attribute, the total time before we run out of breath.
    * In clientActivate, which is called once on startup, we set the current 'breath' to the maximum value.
    * in clientAct, which is called each frame, we check if we are moving or strafing (this.move || this.strafe), and if so we deduct the time spent in this frame - seconds - from our breath. If we run out of breath, we deduct health.
  * We add RunOutOfBreathPlugin to the plugins used in our player class, alongside Health.plugin.

* __4_guns__ - Adds two guns to the __1_recommended_template__ map script.
  * We include the Firing module and two gun modules, Insta and Shotgun.
  * We create the guns, playerInstaGun and playerShotgun. In doing so we also give a name and a HUD icon for them.
  * We bake two plugins for guns: Firing.plugins.protocol (used by all firing entities) and Firing.plugins.player, used by players.
  * In the player's init, we add code to define the possible guns (the two we created before) and set the Insta gun as default.
  * In the application, we set clientClick to Firing.clientClick, which lets the Firing module handle clicks, so clicking can fire the guns.
  * Note: Some guns require additional plugins. For example, rockets require the Projectiles module and plugins. See library/1_3/mapscripts for some map scripts using such weapons.

* __5_eventlist__ - A simple example of a repeating event. You can use act() to run scripts each frame, but sometimes you want events to run at a lower frequency. The GameManager eventList plugin provides an easy and efficient way to do that. In this example script, we add 10 health each second.
  * We add the event in clientActivate - it will run on the client. (Note that, as written here, it will run on **all** the connected clients - each adding health to every player. Normally you would want each client to add health to just that player's entity.)
  * eventManager.add() receives a JavaScript object. This makes it easy to define the parameters using JSON syntax.
    * secondsBefore is how long before running the event. In this case, 0 - start immediately.
    * secondsBetween, if defined, is how long to wait between calling the event. If not specified, the event is one-time. If specified, it will run repeatedly until the event handler returns false.
    * func: The function, or 'handler', that is called. Note how we use bind() to keep 'this' pointing to the current object (this is required by JavaScript, which otherwise changes 'this' in function definitions, unlike normally with closures).
      * We simply check if health is low, and if so, add 10.
    * entity: The entity to which this event 'belongs'. If that entity vanishes from the game, the event is stopped automatically. If you forget this, you may get crashes when players leave the game and their entities are destroyed, but the events keep trying to access them.
  * The eventList plugin allows other things, like changing the delay between repeating events, etc.; see the source code or other code examples for more info.

* TODO: client-server synching example with SV

