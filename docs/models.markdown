Models
======

Importing an MD5 mesh file
--------------------------

* Place the files in a folder. You should have an .md5mesh file, some .md5anim files (if you have animations), and some texture files (png, jpg or dds).
* Name the texture files using the standard conventions also used in the texture config tool: \_cc.\* for primary (diffuse/colors), \_nm.\* for normal map, \_sc.\* for masks (see sauer docs (models.html); basically, spec/glow/chrome in RGB channels).
* Run

    python tools/package\_model.py SOURCE\_DIR OUTPUT\_DIR

  where SOURCE\_DIR is where you put those files, and OUTPUT\_DIR is where the packaged model will be placed. Note that OUTPUT\_DIR must be a directory inside a packages/ hierarchy, something like some\_dir/packages/models/modelpack1/mynewmodel. This is necessary because the config files need to know the absolute location (under packages/models) of the model itself.

  The mesh, animation and texture files will be copied over after appropriate processing and renaming, and a config file will be created for all of that.

You should now be able to load the mesh in the client. To test it, first put the model in your packages/ folder (if you didn't already create it in there with the package\_model.py tool). Then create a mapmodel in some map (F8 in edit mode, then Mapmodel), rightclick on the mapmodel, and change modelName to the name of your model. So if the model is in home\_dir/packages/models/modelpack1/mynewmodel, the name you need to write in modelName is modelpack1/mynewmodel.

You can now adjust model parameters, like scaling, etc., by editing the model's config file (md5.js). For example, changing the value sent to Model.scale will scale the model. To see the change, do /reloadmodel MODELNAME (where MODELNAME is the modelName), that will reload the model so you can see the changes take effect. (Note: sauer's /clearmodel will not work, see the reloadmodel comments in the code for more info.)


Older docs
----------

(Sorry about improper formatting here - just copied over from wiki. Need editing.)

Best thing is to look at the existing example models. But here is an overview of important stuff:

The Cube 2 models reference is useful (note that we use .js config files, not .cfg, and we use JavaScript syntax in those files. See the example models). The Cube Wiki also has several useful pages on the exporting process.

Some clarifications to those docs:

    * MD5 models:
          o Each mesh in the md5mesh file must be 'marked' as follows:

            mesh {
            // meshes: MESHNAME

            where MESHNAME is the name of the mesh, that you will use in the config file.
          o MD5 models are meant, by default, for characters (basically human-type figures). One implication of that is that such models will not pitch, by default. The typical usage is to tell the model (using md5Pitch) to pitch a particular bone. Then that bone will pitch, but everything else by default wont - which lets you pitch the upper half of the model, keeping the lower half fixed (how characters normally work). If you want, instead, for an entire MD5 model to pitch (like an OBJ model would), do

            Model.md5Pitch('', 1.0, 0, -90, 90);


    * OBJ models: After the last 'vt' line for a mesh, you should have:

      g MESHNAME

    * Unit sizes: The units for meshes - and everything else in the world - is in 'cube units'. 7 cube units are about equal to a meter (3.28 feet). So, typical character models are usually around 10-12 cube units or so.
          o If you are importing from Blender, then generally 1 Blender unit is equal to one meter. So, your model config file should have something like Model.scale(500); - which scales the model 500% (or 5 times), so 2 units in meters in the model file will be 10 units in the game (a reasonable value).


Differences with Cube 2/Sauerbraten

    * You can have up to 128 animations per model. 0-39 are defined by Cube 2, which leaves 40-127 free to be used as custom animations by you.
          o To define a custom animation in the model script, use e.g. "50" to define animation 50 (use that where you would have used a Cube 2 animation identifier like "jump", "forward", etc.).
          o To run a custom animation on a model, use the integer value, like 50, where you would have used a Cube 2 constant like ANIM\_JUMP or ANIM\_FORWARD (of course, you can define constants for your own use, so you don't type 50 everywhere).


Tips on Importing and Setting Up

    * /clearmodel will remove a loaded model, which will then be reloaded, so that is a way to refresh it as you are testing changes to your md5.js. It might be convenient to do something like /bind F5 [clearmodel MYMODEL] so that you can quickly iterate and test.

