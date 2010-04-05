Adding web browsers to your map
===============================

* For clients to actually see the browsers, they need to use the plugin. Currently there is no graphical interface for doing so. To do it, edit settings.cfg in (intensityengine_client, under the user's home directory), and make sure it has a section like the following

  [Components]
  list = intensity.components.qtwebkit

  (list is comma separated and can include additional plugins, of course). The new settings will go into effect the next time the user starts up the client.

  The component needs the user to have Qt and PyQt installed. On Ubuntu, this can be done with apt-get install python-qt4. For other operating systems, there should be convenient downloads from the Qt and PyQt people.

  The plugin is included (but not set to run by default) in version 1.1.1 (bzr revision 1305) and above of the Syntensity client. Users that have older versions installed need to upgrade.

* To add web browsers to the map, make sure its map.js uses the 1.3 library, and includes the following files, like so:

  Library.include('library/1_3/');
  Library.include('library/' + Global.LIBRARY_VERSION + '/Plugins');
  Library.include('library/' + Global.LIBRARY_VERSION + '/__CorePatches');
  Library.include('library/' + Global.LIBRARY_VERSION + '/mapelements/specific/WebBrowser');

  The last line sets up the WebBrowser entity class, the ones before it load necessary components (which you are probably loading anyhow for other things).

* After editing the map script as described above, run the map and add web browsers. Like any entity, you add them by pressing F8 in edit mode. In this case, select 'WebBrowser' from the popup list of classes.

  After creating such an entity, it will load a default URL. You can change the URL by rightclicking (in edit mode) on the entity and changing webBrowserURL. This is the URL that will be loaded when people enter the map. You can also do standard things to the entity like drag it to move it around, rotate it with R plus the mouse wheel, etc.


Limitations (at the moment)
---------------------------

* Key presses are ignored for some reason.
* Mouse clicks seem to be slightly 'off' in their position.
* Users can see different pages, as the browsers are client-side, and they can click on the pages to scroll them, to use hyperlinks to go to other pages, etc.
* You can run up to 10 browsers at a time in a map. Note that anyhow 10 browsers would probably put a lot of stress on the CPU, to push all that pixel data to the video card (each browser window is 1024x768 pixels).

