Mapping Tutorial
================

(Copied from the wiki, not edited for formatting yet.)

See also the Cube 2/Sauerbraten editing docs (editing.html).

This is a brief overview of the main aspects of mapping.
Setting up
First, create a new map and then run that map. Once you have a running map which you can edit, you can start with the tutorial below.
First steps

    * Get a feel for moving around in the world, using the mouse and arrow keys/WASD/numpad.
    * Pressing M toggles mouselook mode. Some people prefer to edit in that mode, others don't. It's a good idea to get used to both modes to some degree, as they are useful for different things. When in this mode, it's useful to use the 4 and 6 numpad keys to turn left and right (as the mouse no longer does that), and pageup and pagedown for turning up and down.
    * Pressing E toggles edit mode. In edit mode you can perform editing. Note that M for toggling mouselook mode only works outside of edit mode (it does something else in edit mode); this sometimes can confuse people.

Entities

    * Editing entity data: In edit mode, hover the mouse over an 'entity', like your own avatar, a tree, or some other significant object. When you do so, you will see the entity highlighted. Try right-clicking on it. This will show a dialog with the entity's 'state data' - information about the entity, typically shared between the client and server.
          o You can edit the state data fields and the effects take hold immediately. For example, rightclick on your own avatar in edit mode and right click it, then find 'movementSpeed' (usually in the second tab). Change it to 100. Your avatar will now move much faster than before.
    * Adding entities: Add entities by aiming at a position in edit mode, then pressing F8. A dialog will appear asking what 'class' of entity to add. There are several default entity classes, and you can also add your own. For now, some interesting classes are: Light, ParticleEffect, and Mapmodel.
          o For example, select ParticleEffect. A small fire should appear. Still in edit mode, rightclick on it to edit it's state data. Try changing value1 and value2; these affect the width and height of the flame. Value3 affects the color, and type lets you pick different kinds of particle effects.
    * Moving entities: You can move the entity by clicking on it and dragging it around. Note that you always move it in two coordinates, depending on where you place the mouse on the entity.
    * Deleting entities: You can delete an entity by clicking on it to select it, then pressing the delete key.
    * Copy&pasting entities: You can copy and paste entities by clicking to select, pressing C, clicking somewhere else, and pressing V.


Cube Geometry

    * A good overview is in the Sauerbraten documentation. The following will repeat the main points from there.
    * Pushing/pulling cubes: In edit mode, click on the ground somewhere. Then use the mouse wheel. This pushes and pulls cubes, the most basic way to change the world.
    * Cube size: Hold down G and scroll the mouse wheel
    * Pushing cube corners: Hold down Q and scroll the mouse wheel. F will push all 4 corners at once.
    * Texturing cube faces: Hold down Y and scroll the mouse wheel.
    * Copy&pasting geometry: Select an area by clicking and dragging, or clicking at one edge and rightclicking on the other. C then copies. Click elsewhere and press V to paste.
    * Undo: Press Z to undo the last action. Note that this won't work in all
    * Heightmaps: These can be done in any orientation, like from the walls or ceiling. You must be in private edit mode for this, which you can enter by selecting 'request private edit mode' in the menu (it is only possible if you are alone on the server). Heightmaps can then be used by selecting an area, then pressing H, then using the mouse wheel to raise and lower areas.
    * Materials: Select an area, then in the menu select 'editing commands'->'materials' and, for example, water. The area will now be filled with water.

Lighting

    * Adding a light: There are two ways:
          o The same as any other entity: In edit mode, press F8 and select 'Light'
          o Using the GUI: In the menu, select 'editing commands'->'ents' tab->'light'. Adjust the parameters as desired, and press the line containing 'newent Light'.
    * Editing parameters: Just like any other entity, rightclick on it. You can then adjust the radius, red, green and blue variables. Note that radius '0' is considered infinite radius (this would be suitable for sunlight).
    * Calculating lighting: In the menu, select 'editing commands'->'light' tab, and select one of the first three options. More detailing lighting takes longer to calculate.

Next steps
