blender2md5.py version 0.94
Exports animated characters from Blender to MD5 (Doom3 engine format)
Copyright (C) 2003, 2004,2005,2006 Thomas "der_ton" Hutter, tom-(at)gmx.de,
                   "der_ton" on www.doom3world.org

based on:
blender2cal3D.py version 0.5 by Jean-Baptiste LAMY -- jiba@tuxfamily.org
(See http://oomadness.tuxfamily.org/en/blender2cal3d)
Thanks to PaladinOfKaos for a workaround for a Bug in Blender2.40

The included fish.blend file: 
The fish model was taken from the tutorial files on www.blender.org.
It was "rigged" and animated by me, der_ton, to provide
a simple out-of-the-box model to get started with.



Legal Stuff:
************
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA



Where to get the latest version:
********************************
http://home.tiscali.de/der_ton/blender2md5.rar
http://www.doom3world.org/ (there is a Blender section in the forum)



How to use:
***********
This script will work with Blender 2.41, which is the current version at the 
time this is being written.

Caution:
Be careful when using the exporter, there is a bug in the python API that might affect this script. Save your .blend before using the exporter, then export, then reload the .blend file, just to make sure. I haven't tested this but just saw a bug report about a blender function (Action.setActive()) that this script is using, too:
http://www.blender.org/forum/viewtopic.php?t=8094 

A 3d view needs to be visible during export because of a limitation in the Blender.Pose module.

The necessary Python modules are included in the Windows version of Blender, 
so you do not need to install python.

For a quickstart, open the fish.blend file in Blender. The fish model was
taken from Blender´s tutorial files on www.blender.org. It was rigged and 
animated by me, der_ton, to provide a simple out-of-the-box
model to get started with.

Smoothgroups:
The exporter exports smoothgroups (it duplicates vertices along the borders of 
smoothgroups, so that the vertices have individual normals in respect of their
smoothgroup. But if you haven't set smoothgroups, this can lead to each vertex
being cloned for each triangle it belongs to, resulting in an unnecessary big
md5mesh file, and may lead to bad performance of the model in the game. To set
smoothgroups for the whole mesh, select the mesh, go to face-edit mode ('f'), 
select all faces ('a') and press "Set Smooth" in the edit buttons.

Read the instructions in the blender2md5.py file (open it in a
text editor, or in Blender´s text window).
This is just a short readme for the package.

To export only the md5mesh or only the md5anim, leave the field for md5anim or 
md5mesh filename (respectively) blank.



Recommended Reads for using Blender in character animation:
***********************************************************
For general info, take a look at
www.blender.org
www.blender3d.org
www.elysiun.com
The Blender section of the www.Doom3World.org Forum


These links may be outdated by now:

http://download.blender.org/release/Publisher2.25/PublisherUserDoc.pdf
http://www.blender.org/modules/documentation/publisherdoc/publisherdoc.html
(HTML version of the above PDF file)

http://10secondclub.org/users/juicy/
(there are tutorials about keyframing, among other stuff)

http://blenderchar.weirdhat.com/
(several tutorials)

Blender documentation with tutorials:
http://download.blender.org/documentation/html/book1.html

part of this might be interesting especially for newbies who want to get
a quick start in character creation and animation:
http://download.blender.org/documentation/html/c798.html
