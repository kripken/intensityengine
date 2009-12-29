#!BPY
"""
Name: 'MD5 Export (2007)'
Blender: 241
Group: 'Export'
Tip: 'Export md5mesh and md5anim files for the Doom3 engine'
"""

# blender2md5.py -- (olded: version 0.94, 2006-02-09)
# Exports animated characters and cameras
# from Blender to md5 (Doom3 engine format)
# Copyright (C) 2003,2004,2005,2006 Thomas "der_ton" Hutter, tom-(at)gmx.de,
#                    "der_ton" on www.doom3world.org
# Andreas Kirsch, 2007 (minor modifications)
# (I also wrote a viewer for md5 files, that is useful
#  for workflow in conjunction with this script,
#  available (as a Win32 binary only) on http://www.doom3world.org)

# Portions of this script are based on:
# blender2cal3D.py version 0.5 by Jean-Baptiste LAMY -- jiba@tuxfamily.org
# (See http://oomadness.tuxfamily.org/en/blender2cal3d)

# HOW TO USE:
# 1 - load the script in Blender's text editor
# 2 - type meta/alt + P
# 3 - specify frame range and filenames in the GUI
#     if you don't fill in one of the filenames when exporting to md5mesh/anim,
#     the corresponding file will not be generated but the other one will
#     (useful when exporting multiple anims for the same md5mesh)


# Questions and comments are welcome!
# If you use this script, a mention that you used this script in your
# production would be nice. :)
# Also, I would love to see what you achieved, please feel free to tell
# me about your results.


# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA


# This script is a Blender => MD5Mesh/MD5Anim/MD5Camera exporter

# Grab the latest version of the script here:
# http://home.tiscali.de/der_ton/blender2md5.zip
# http://www.doom3world.org


# Some Doom3world.org subforums where you will find additional infos,
# and where you can ask for support:
# "Blender":
# http://www.doom3world.org/phpbb2/viewforum.php?f=50
# "Modelling and Animations"
# http://www.doom3world.org/phpbb2/viewforum.php?f=3

# Thanks and acknowledgements:
#  - Jiba, for his excellent blender2cal3d exporter, clean and reusable
#  - kat, for his constant testing and feedback
#  - the folks at www.elysiun.org, a forum dedicated to Blender
#  - the many people behind Blender
#  - the many people behind Python


# ADVICES :
# - do not use "Size" keys in Blender's action IPOs

# TODO :
# - Optimization: almost all the time is spent on scene.makeCurrent(), called for
#   updating the IPO curve's values. Updating a single IPO and not the whole scene
#   would speed up a lot.

# - which objects should be output? should be centralized (for mesh-output and bbox output) (maybe let the user select the armature only)
# - camera output: how to handle the cut-list?
# - ignore meshes that are not attached to the one armature
# - animation stuff is kinda hacky
# Done (BlackHC) - "clean up" the framedata so that repeated identical values are not exported as animated components
# - use material name as shader name in export, instead of bitmap filename ?
# - use Mathlib instead of custom math routines
# - better utilize Blender's new API (bone rest pose extraction, ...)

# version history:
#       2007        rewrite GUI to support multiple animations and other stuff but disable camera export for now
# ----  2007        add batch job support
# ----  2007        purge unchanged bone frames from animations
# 0.94  2006-02-09: modified for Blender2.41,
#                   uses Pose-module (enables export of ik without baking to IPO)
# 0.93  2006-01-20: modified for Blender2.40
# 0.92a 2005-01-26: fixed stupid bug in camera export button handler
# 0.92  2004-11-04: changed the animation export back to specifyable action
#                   changed some filehandling (mesh or anim export are now optional)
# 0.91  2004-10-06: fixed borked baseframe for bones that have no IPO that animates them
#       2004-09-10: fixed script throwing an error for improperly skinned vertices with no weights
# 0.9   2004-09-03: initial release

# Parameters :

# Helps for having the model rotated right.
# Put in BASE_MATRIX your own rotation if you need some.

BASE_MATRIX = None # matrix_rotate_x(math.pi / 2.0)


#########################################################################################
# Code starts here.

import sys, os, os.path, struct, math, string
# if you get an error here, it might be
# because you don't have Python installed.
# go to www.python.org and get python 2.4 (not necessarily the latest!)

import Blender

# HACK -- it seems that some Blender versions don't define sys.argv,
# which may crash Python if a warning occurs.
if not hasattr(sys, "argv"): sys.argv = ["???"]


# Math stuff
# some of this is not used in this version, but not thrown out yet.
# TODO: remove unused code because it's likely that it is broken!

# Andreas Kirsch 07: change x, y, z, w to w, x, y, z order
def matrix2quaternion(m):
  s = math.sqrt(abs(m[0][0] + m[1][1] + m[2][2] + m[3][3]))
  if s == 0.0:
    x = abs(m[2][1] - m[1][2])
    y = abs(m[0][2] - m[2][0])
    z = abs(m[1][0] - m[0][1])
    if   (x >= y) and (x >= z): return 0.0, 1.0, 0.0, 0.0
    elif (y >= x) and (y >= z): return 0.0, 0.0, 1.0, 0.0
    else:                       return 0.0, 0.0, 0.0, 1.0
  return quaternion_normalize([
    0.5 * s,
    -(m[2][1] - m[1][2]) / (2.0 * s),
    -(m[0][2] - m[2][0]) / (2.0 * s),
    -(m[1][0] - m[0][1]) / (2.0 * s),
    ])

def quaternion_normalize(q):
  l = math.sqrt(q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3])
  return q[0] / l, q[1] / l, q[2] / l, q[3] / l

def rot_to_quaternion(rot):
  return (rot.w, rot.x, rot.y, rot.z)

def quaternion_multiply(q1, q2):
  r = [
    q2[0] * q1[0] - q2[1] * q1[1] - q2[2] * q1[2] - q2[3] * q1[3],
    q2[0] * q1[1] + q2[1] * q1[0] + q2[2] * q1[3] - q2[3] * q1[2],
    q2[0] * q1[2] + q2[2] * q1[0] + q2[3] * q1[1] - q2[1] * q1[3],
    q2[0] * q1[3] + q2[3] * q1[0] + q2[1] * q1[2] - q2[2] * q1[1],
    ]
  d = math.sqrt(r[0] * r[0] + r[1] * r[1] + r[2] * r[2] + r[3] * r[3])
  r[0] /= d
  r[1] /= d
  r[2] /= d
  r[3] /= d
  return r

def quaternion_to_md5(q):
  if q[0] > 0:
    return -q[1], -q[2], -q[3]
  else:
    return q[1], q[2], q[3]

def matrix_invert(m):
  det = (m[0][0] * (m[1][1] * m[2][2] - m[2][1] * m[1][2])
       - m[1][0] * (m[0][1] * m[2][2] - m[2][1] * m[0][2])
       + m[2][0] * (m[0][1] * m[1][2] - m[1][1] * m[0][2]))
  if det == 0.0: return None
  det = 1.0 / det
  r = [ [
      det * (m[1][1] * m[2][2] - m[2][1] * m[1][2]),
    - det * (m[0][1] * m[2][2] - m[2][1] * m[0][2]),
      det * (m[0][1] * m[1][2] - m[1][1] * m[0][2]),
      0.0,
    ], [
    - det * (m[1][0] * m[2][2] - m[2][0] * m[1][2]),
      det * (m[0][0] * m[2][2] - m[2][0] * m[0][2]),
    - det * (m[0][0] * m[1][2] - m[1][0] * m[0][2]),
      0.0
    ], [
      det * (m[1][0] * m[2][1] - m[2][0] * m[1][1]),
    - det * (m[0][0] * m[2][1] - m[2][0] * m[0][1]),
      det * (m[0][0] * m[1][1] - m[1][0] * m[0][1]),
      0.0,
    ] ]
  r.append([
    -(m[3][0] * r[0][0] + m[3][1] * r[1][0] + m[3][2] * r[2][0]),
    -(m[3][0] * r[0][1] + m[3][1] * r[1][1] + m[3][2] * r[2][1]),
    -(m[3][0] * r[0][2] + m[3][1] * r[1][2] + m[3][2] * r[2][2]),
    1.0,
    ])
  return r

def matrix_rotate_x(angle):
  cos = math.cos(angle)
  sin = math.sin(angle)
  return [
    [1.0,  0.0, 0.0, 0.0],
    [0.0,  cos, sin, 0.0],
    [0.0, -sin, cos, 0.0],
    [0.0,  0.0, 0.0, 1.0],
    ]

def matrix_rotate_y(angle):
  cos = math.cos(angle)
  sin = math.sin(angle)
  return [
    [cos, 0.0, -sin, 0.0],
    [0.0, 1.0,  0.0, 0.0],
    [sin, 0.0,  cos, 0.0],
    [0.0, 0.0,  0.0, 1.0],
    ]

def matrix_rotate_z(angle):
  cos = math.cos(angle)
  sin = math.sin(angle)
  return [
    [ cos, sin, 0.0, 0.0],
    [-sin, cos, 0.0, 0.0],
    [ 0.0, 0.0, 1.0, 0.0],
    [ 0.0, 0.0, 0.0, 1.0],
    ]

def matrix_rotate(axis, angle):
  vx  = axis[0]
  vy  = axis[1]
  vz  = axis[2]
  vx2 = vx * vx
  vy2 = vy * vy
  vz2 = vz * vz
  cos = math.cos(angle)
  sin = math.sin(angle)
  co1 = 1.0 - cos
  return [
    [vx2 * co1 + cos,          vx * vy * co1 + vz * sin, vz * vx * co1 - vy * sin, 0.0],
    [vx * vy * co1 - vz * sin, vy2 * co1 + cos,          vy * vz * co1 + vx * sin, 0.0],
    [vz * vx * co1 + vy * sin, vy * vz * co1 - vx * sin, vz2 * co1 + cos,          0.0],
    [0.0, 0.0, 0.0, 1.0],
    ]


def point_by_matrix(p, m):
  return [p[0] * m[0][0] + p[1] * m[1][0] + p[2] * m[2][0] + m[3][0],
          p[0] * m[0][1] + p[1] * m[1][1] + p[2] * m[2][1] + m[3][1],
          p[0] * m[0][2] + p[1] * m[1][2] + p[2] * m[2][2] + m[3][2]]

def vector_by_matrix(p, m):
  return [p[0] * m[0][0] + p[1] * m[1][0] + p[2] * m[2][0],
          p[0] * m[0][1] + p[1] * m[1][1] + p[2] * m[2][1],
          p[0] * m[0][2] + p[1] * m[1][2] + p[2] * m[2][2]]

def vector_length(v):
  return math.sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2])

def vector_normalize(v):
  l = math.sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2])
  try:
    return v[0] / l, v[1] / l, v[2] / l
  except:
    return 1, 0, 0

def vector_dotproduct(v1, v2):
  return v1[0] * v2[0] + v1[1] * v2[1] + v1[2] * v2[2]

def vector_crossproduct(v1, v2):
  return [
    v1[1] * v2[2] - v1[2] * v2[1],
    v1[2] * v2[0] - v1[0] * v2[2],
    v1[0] * v2[1] - v1[1] * v2[0],
    ]

def vector_angle(v1, v2):
  s = vector_length(v1) * vector_length(v2)
  f = vector_dotproduct(v1, v2) / s
  if f >=  1.0: return 0.0
  if f <= -1.0: return math.pi / 2.0
  return math.atan(-f / math.sqrt(1.0 - f * f)) + math.pi / 2.0

# end of math stuff.


NEXT_MATERIAL_ID = 0
class Material:
  def __init__(self, map_filename = None):
    self.ambient_r  = 255
    self.ambient_g  = 255
    self.ambient_b  = 255
    self.ambient_a  = 255
    self.diffuse_r  = 255
    self.diffuse_g  = 255
    self.diffuse_b  = 255
    self.diffuse_a  = 255
    self.specular_r = 255
    self.specular_g = 255
    self.specular_b = 255
    self.specular_a = 255
    self.shininess = 1.0
    if map_filename: self.maps_filenames = [map_filename]
    else:            self.maps_filenames = []

    MATERIALS[map_filename] = self

    global NEXT_MATERIAL_ID
    self.id = NEXT_MATERIAL_ID
    NEXT_MATERIAL_ID += 1


  def to_md5mesh(self):
    if self.maps_filenames:
      return self.maps_filenames[0]
    else:
      return ""


MATERIALS = {}

class Mesh:
  def __init__(self, name):
    self.name      = name
    self.submeshes = []

    self.next_submesh_id = 0


  def to_md5mesh(self):
    meshnumber=0
    buf = ""
    for submesh in self.submeshes:
      buf=buf + "mesh {\n"
      meshnumber += 1
      buf=buf + submesh.to_md5mesh()
      buf=buf + "}\n\n"

    return buf

class SubMesh:
  def __init__(self, mesh, material):
    self.material   = material
    self.vertices   = []
    self.faces      = []
    self.nb_lodsteps = 0
    self.springs    = []
    self.weights    = []

    self.next_vertex_id = 0
    self.next_weight_id = 0

    self.mesh = mesh
    self.name = mesh.name
    self.id = mesh.next_submesh_id
    mesh.next_submesh_id += 1
    mesh.submeshes.append(self)

  def bindtomesh (self, mesh):
    # HACK: this is needed for md5 output, for the time being...
    # appending this submesh to the specified mesh, disconnecting it from the original one
    self.mesh.submeshes.remove(self)
    self.mesh = mesh
    self.id = mesh.next_submesh_id
    mesh.next_submesh_id += 1
    mesh.submeshes.append(self)

  def generateweights(self):
    self.weights = []
    self.next_weight_id = 0
    for vert in self.vertices:
      vert.generateweights()

  def reportdoublefaces(self):
    for face in self.faces:
      for face2 in self.faces:
        if not face == face2:
          if (not face.vertex1==face2.vertex1) and (not face.vertex1==face2.vertex2) and (not face.vertex1==face2.vertex3):
            return
          if (not face.vertex2==face2.vertex1) and (not face.vertex2==face2.vertex2) and (not face.vertex2==face2.vertex3):
            return
          if (not face.vertex3==face2.vertex1) and (not face.vertex3==face2.vertex2) and (not face.vertex3==face2.vertex3):
            return
          print "doubleface! %s %s" % (face, face2)

  def to_md5mesh(self):
    self.generateweights()

    self.reportdoublefaces()

    buf="\tshader \"%s\"\n\n" % (self.material.to_md5mesh())
    if len(self.weights) == 0:
      buf=buf + "\tnumverts 0\n"
      buf=buf + "\n\tnumtris 0\n"
      buf=buf + "\n\tnumweights 0\n"
      return buf

    # output vertices
    buf=buf + "\tnumverts %i\n" % (len(self.vertices))
    vnumber=0
    for vert in self.vertices:
      buf=buf + "\tvert %i %s\n" % (vnumber, vert.to_md5mesh())
      vnumber += 1

    # output faces
    buf=buf + "\n\tnumtris %i\n" % (len(self.faces))
    facenumber=0
    for face in self.faces:
      buf=buf + "\ttri %i %s\n" % (facenumber, face.to_md5mesh())
      facenumber += 1

    # output weights
    buf=buf + "\n\tnumweights %i\n" % (len(self.weights))
    weightnumber=0
    for weight in self.weights:
      buf=buf + "\tweight %i %s\n" % (weightnumber, weight.to_md5mesh())
      weightnumber += 1

    return buf



class Vertex:
  def __init__(self, submesh, loc, normal):
    self.loc    = loc
    self.normal = normal
    self.collapse_to         = None
    self.face_collapse_count = 0
    self.maps       = []
    self.influences = []
    self.weights = []
    self.weight = None
    self.firstweightindx = 0
    self.cloned_from = None
    self.clones      = []

    self.submesh = submesh
    self.id = submesh.next_vertex_id
    submesh.next_vertex_id += 1
    submesh.vertices.append(self)

  def generateweights(self):
    self.firstweightindx = self.submesh.next_weight_id
    for influence in self.influences:
      weightindx = self.submesh.next_weight_id
      self.submesh.next_weight_id += 1
      newweight = Weight(influence.bone, influence.weight, self, weightindx, self.loc[0], self.loc[1], self.loc[2])
      self.submesh.weights.append(newweight)
      self.weights.append(newweight)

  def to_md5mesh(self):
    if self.maps:
      buf = self.maps[0].to_md5mesh()
    else:
      buf = "( %f %f )" % (self.loc[0], self.loc[1])
    buf = buf + " %i %i" % (self.firstweightindx, len(self.influences))
    return buf

class Map:
  def __init__(self, u, v):
    self.u = u
    self.v = v


  def to_md5mesh(self):
    buf = "( %f %f )" % (self.u, self.v)
    return buf


class Weight:
  def __init__(self, bone, weight, vertex, weightindx, x, y, z):
    self.bone = bone
    self.weight = weight
    self.vertex = vertex
    self.indx = weightindx
    invbonematrix = matrix_invert(self.bone.matrix)
    self.x, self.y, self.z = point_by_matrix ((x, y, z), invbonematrix)

  def to_md5mesh(self):
    buf = "%i %f ( %f %f %f )" % (self.bone.id, self.weight, self.x*scale, self.y*scale, self.z*scale)
    return buf


class Influence:
  def __init__(self, bone, weight):
    self.bone   = bone
    self.weight = weight


class Face:
  def __init__(self, submesh, vertex1, vertex2, vertex3):
    self.vertex1 = vertex1
    self.vertex2 = vertex2
    self.vertex3 = vertex3

    self.can_collapse = 0

    self.submesh = submesh
    submesh.faces.append(self)


  def to_md5mesh(self):
    buf = "%i %i %i" % (self.vertex1.id, self.vertex3.id, self.vertex2.id)
    return buf


class Skeleton:
  def __init__(self, MD5Version = 10, commandline = ""):
    self.bones = []
    self.MD5Version = MD5Version
    self.commandline = commandline
    self.next_bone_id = 0


  def to_md5mesh(self, numsubmeshes):
    buf = "MD5Version %i\n" % (self.MD5Version)
    buf = buf + "commandline \"%s\"\n\n" % (self.commandline)
    buf = buf + "numJoints %i\n" % (self.next_bone_id)
    buf = buf + "numMeshes %i\n\n" % (numsubmeshes)
    buf = buf + "joints {\n"
    for bone in self.bones:
      buf = buf + bone.to_md5mesh()
    buf = buf + "}\n\n"
    return buf


BONES = {}

class Bone:
  def __init__(self, skeleton, parent, name, mat,pmat, theboneobj):
    self.parent = parent #Bone
    self.name   = name   #string
    self.children = []   #list of Bone objects
    self.theboneobj = theboneobj #Blender.Armature.Bone
    # HACK: this flags if the bone is animated in the one animation that we export
    self.is_animated = 0  # = 1, if there is an ipo that animates this bone

    self.matrix = mat
    self.pmatrix = pmat # parentspace matrix
    if parent:
      parent.children.append(self)

    self.skeleton = skeleton
    self.id = skeleton.next_bone_id
    skeleton.next_bone_id += 1
    skeleton.bones.append(self)

    BONES[name] = self


  def to_md5mesh(self):
    buf= "\t\"%s\"\t" % (self.name)
    parentindex = -1
    if self.parent:
        parentindex=self.parent.id
    buf=buf+"%i " % (parentindex)

    pos1, pos2, pos3= self.matrix[3][0], self.matrix[3][1], self.matrix[3][2]
    buf=buf+"( %f %f %f ) " % (pos1*scale, pos2*scale, pos3*scale)
    #qx, qy, qz, qw = matrix2quaternion(self.matrix)
    #if qw<0:
    #    qx = -qx
    #    qy = -qy
    #    qz = -qz
    m = self.matrix
    bquat = self.matrix.toQuat().normalize()
    buf=buf+"( %f %f %f )\t\t// " % quaternion_to_md5(bquat)
    if self.parent:
        buf=buf+"%s" % (self.parent.name)

    buf=buf+"\n"
    return buf

class MD5Animation:
  def __init__(self, md5skel, MD5Version = 10, commandline = ""):
    self.framedata    = [] # framedata[boneid] holds the data for each frame
    self.bounds       = []
    self.baseframe    = []
    self.skeleton     = md5skel
    self.boneflags    = []  # stores the md5 flags for each bone in the skeleton
    self.boneframedataindex = [] # stores the md5 framedataindex for each bone in the skeleton
    self.MD5Version   = MD5Version
    self.commandline  = commandline
    self.numanimatedcomponents = 0
    self.framerate    = 24
    self.numframes    = 0
    for b in self.skeleton.bones:
      self.framedata.append([])
      self.baseframe.append([])
      self.boneflags.append(0)
      self.boneframedataindex.append(0)

  def to_md5anim(self):
    currentframedataindex = 0
    animatedbones = []
    for bone in self.skeleton.bones:
      # Andreas Kirsch '07 - purge frames of not animated bones
      self.purgeframes(bone.id)

      if (len(self.framedata[bone.id])>1):
        if (len(self.framedata[bone.id])>self.numframes):
          self.numframes=len(self.framedata[bone.id])

        self.boneframedataindex[bone.id]=currentframedataindex
        self.boneflags[bone.id] = 63
        currentframedataindex += 6
        self.numanimatedcomponents = currentframedataindex

        animatedbones.append( bone )

        # old code used to create base frames from the bone matrix (BlackHC)
        #qx, qy, qz =quaternion_to_md5( rot_to_quaternion( bone.pmatrix.toQuat().normalize() ) )
        #self.baseframe[bone.id]= (bone.pmatrix[3][0]*scale, bone.pmatrix[3][1]*scale, bone.pmatrix[3][2]*scale, qx, qy, qz)

      (x,y,z),(qx,qy,qz) = self.framedata[bone.id][0]
      self.baseframe[bone.id]= (x*scale,y*scale,z*scale,qx,qy,qz)

    buf = "MD5Version %i\n" % (self.MD5Version)
    buf = buf + "commandline \"%s\"\n\n" % (self.commandline)
    buf = buf + "numFrames %i\n" % (self.numframes)
    buf = buf + "numJoints %i\n" % (len(self.skeleton.bones))
    buf = buf + "frameRate %i\n" % (self.framerate)
    buf = buf + "numAnimatedComponents %i\n\n" % (self.numanimatedcomponents)
    buf = buf + "hierarchy {\n"

    for bone in self.skeleton.bones:
      parentindex = -1
      flags = self.boneflags[bone.id]
      framedataindex = self.boneframedataindex[bone.id]
      if bone.parent:
        parentindex=bone.parent.id
      buf = buf + "\t\"%s\"\t%i %i %i\t//" % (bone.name, parentindex, flags, framedataindex)
      if bone.parent:
        buf = buf + " " + bone.parent.name
      buf = buf + "\n"
    buf = buf + "}\n\n"

    buf = buf + "bounds {\n"
    for b in self.bounds:
      buf = buf + "\t( %f %f %f ) ( %f %f %f )\n" % (b)
    buf = buf + "}\n\n"

    buf = buf + "baseframe {\n"
    for b in self.baseframe:
      buf = buf + "\t( %f %f %f ) ( %f %f %f )\n" % (b)
    buf = buf + "}\n\n"

    for f in range(0, self.numframes):
      buf = buf + "frame %i {\n" % (f)
      for b in animatedbones:
        (x,y,z),(qx,qy,qz) = self.framedata[b.id][f]
        buf = buf + "\t%f %f %f %f %f %f\n" % (x*scale, y*scale, z*scale, qx,qy,qz)
      buf = buf + "}\n\n"

    return buf

  def addkeyforbone(self, boneid, time, loc, rot):
    # time is ignored. the keys are expected to come in sequentially
    # it might be useful for future changes or modifications for other export formats
    self.framedata[boneid].append((loc, rot))
    return

  # Andreas Kirsch '07
  def purgeframes(self,boneid):
    # loop through all frames and check whether the bone is actually being
    # animated
    framedata = self.framedata[boneid]
    framenum = len(framedata)
    if framenum <= 1:
      return

    #print "Bone %i" % boneid

    isanimated = False
    #print framedata[0]
    (bx, by, bz), (bqx, bqy, bqz) = baseframe = framedata[0]
    frameindex = 0
    for frame in framedata:
      (x, y, z), (qx, qy, qz) = frame
      frameindex += 1
      epsilon = 0.00001
      if math.fabs( bx - x ) > epsilon or math.fabs( by - y ) > epsilon or math.fabs( bz - z ) > epsilon or \
         math.fabs( bqx - qx ) > epsilon or math.fabs( bqy - qy ) > epsilon or math.fabs( bqz - qz ) > epsilon:
        isanimated = True
        #print math.fabs( bx - x )
        #print math.fabs( by - y )
        #print math.fabs( bz - z )
        #print math.fabs( bqx - qx )
        #print math.fabs( bqy - qy )
        #print math.fabs( bqz - qz )
        #print frameindex
        #print frame
        #print baseframe
        break

    # purge all frames if the bone isn't animated
    if isanimated == False:
      # only keep baseframe - frame 0
      # NOTE: I hope this doesn't break anything but it fixes a bug with motorsep's models (BlackHC)
      framedata[1:] = []
  # --

def getIpoValue(ipo, value, time):
  ipocurves = ipo.getCurves()
  #return ipo.getCurve(value).evaluate(time)

  for i in range(len(ipocurves)):
    if value == ipocurves[i].getName():
      return ipo.getCurveCurval(i)
  print "Error: ipo %s has no ipo curve for %s!" %(ipo, value)

def getminmax(listofpoints):
  if len(listofpoints) == 0: return ([0,0,0],[0,0,0])
  min = [listofpoints[0][0], listofpoints[0][1], listofpoints[0][2]]
  max = [listofpoints[0][0], listofpoints[0][1], listofpoints[0][2]]
  if len(listofpoints)>1:
    for i in range(1, len(listofpoints)):
      if listofpoints[i][0]>max[0]: max[0]=listofpoints[i][0]
      if listofpoints[i][1]>max[1]: max[1]=listofpoints[i][1]
      if listofpoints[i][2]>max[2]: max[2]=listofpoints[i][2]
      if listofpoints[i][0]<min[0]: min[0]=listofpoints[i][0]
      if listofpoints[i][1]<min[1]: min[1]=listofpoints[i][1]
      if listofpoints[i][2]<min[2]: min[2]=listofpoints[i][2]
  return (min, max)

def generateboundingbox(objects, md5animation, framerange):
  scene = Blender.Scene.getCurrent()
  context = scene.getRenderingContext()
  for i in range(framerange[0], framerange[1]+1):
    corners = []
    context.currentFrame(i)
    scene.makeCurrent()
    for obj in objects:
      data = obj.getData()
      if (type(data) is Blender.Types.NMeshType) and data.faces:
        obj.makeDisplayList()
        (lx, ly, lz) = obj.getLocation()
        bbox = obj.getBoundBox()
        #matrix = obj.getMatrix()
        matrix = [
          [1.0,  0.0, 0.0, 0.0],
          [0.0,  1.0, 0.0, 0.0],
          [0.0,  0.0, 1.0, 0.0],
          [0.0,  0.0, 0.0, 1.0],
          ]
        if BASE_MATRIX: matrix = BASE_MATRIX * matrix
        for v in bbox:
          corners.append(point_by_matrix (v, matrix))

    (min, max) = getminmax(corners)

    md5animation.bounds.append((min[0]*scale, min[1]*scale, min[2]*scale, max[0]*scale, max[1]*scale, max[2]*scale))

def export(meshfilename, animfilename, rangestart, rangeend, action_name, scaleval):
  global scale
  scale = scaleval
  # Get the scene
  scene = Blender.Scene.getCurrent()
  context = scene.getRenderingContext()

  action_dict = Blender.Armature.NLA.GetActions()
  if not action_dict.has_key(action_name):
    print "Specified action does not exist: %s " % (action_name)
    print "No md5anim will be generated"
    arm_action = None
  else:
    arm_action = action_dict[action_name]
    ipomap = arm_action.getAllChannelIpos()

  # Export skeleton (=armature)
  skeleton = Skeleton()
  for obj in Blender.Object.Get():
    data = obj.getData()
    if type(data) is Blender.Types.ArmatureType:
      thearmature = obj
      matrix = obj.getMatrix('worldspace')
      if BASE_MATRIX: matrix = BASE_MATRIX * matrix

      def treat_bone(b, parent = None):
        if (parent and not b.parent.name==parent.name):
          return #only catch direct children

        mat = Blender.Mathutils.Matrix(b.matrix['ARMATURESPACE'])*matrix
        #pmat is bone's matrix in parent space
        #pmat = Blender.Mathutils.Matrix(b.matrix['BONESPACE'])*matrix.toQuat().toMatrix() #god this api is ugly, bonespace, armaturespace matrices have different dimensions
        if (parent):
          pmat = Blender.Mathutils.Matrix(b.matrix['ARMATURESPACE'])*Blender.Mathutils.Matrix(parent.theboneobj.matrix['ARMATURESPACE']).invert()
        else:
          pmat = Blender.Mathutils.Matrix(b.matrix['ARMATURESPACE'])*matrix
        bone = Bone(skeleton, parent, b.name, mat, pmat, b)

        if (b.children):
          for child in b.children: treat_bone(child, bone)

      for b in data.bones.values():
        if (not b.parent):  # only treat root bones, treat_bone takes care of children
          treat_bone(b)

      # Only one armature (we break here so this first armature in the scene is the only one we export)
      break


  # Export Mesh data

  meshes = []
  for obj in Blender.Object.Get():
    data = obj.getData()
    if (type(data) is Blender.Types.NMeshType) and data.faces:
      mesh = Mesh(obj.name)
      print "Processing mesh: ", obj.name
      meshes.append(mesh)

      matrix = obj.getMatrix('worldspace')
      if BASE_MATRIX: matrix = BASE_MATRIX * matrix

      faces = data.faces
      while faces:
        image          = faces[0].image
        image_filename = image and image.filename
        material       = MATERIALS.get(image_filename) or Material(image_filename)

        # TODO add material color support here

        submesh  = SubMesh(mesh, material)
        vertices = {}
        for face in faces[:]:
          # der_ton: i added this check to make sure a face has at least 3 vertices.
          # i've seen several models that threw an error in the p3 = face.v[2].co line
          if len(face.v)<3: # throw away faces that have less than 3 vertices
            faces.remove(face)
          elif face.v[0].co == face.v[1].co:
            faces.remove(face)
          elif face.v[0].co == face.v[2].co:
            faces.remove(face)
          elif face.v[1].co == face.v[2].co:
            faces.remove(face)
          elif (face.image and face.image.filename) == image_filename:
            faces.remove(face)

            if not face.smooth:
              p1 = face.v[0].co
              p2 = face.v[1].co
              p3 = face.v[2].co
              normal = vector_normalize(vector_by_matrix(vector_crossproduct(
                [p3[0] - p2[0], p3[1] - p2[1], p3[2] - p2[2]],
                [p1[0] - p2[0], p1[1] - p2[1], p1[2] - p2[2]],
                ), matrix))

            face_vertices = []
            for i in range(len(face.v)):
              vertex = vertices.get(face.v[i].index)
              if not vertex:
                coord  = point_by_matrix (face.v[i].co, matrix)
                if face.smooth: normal = vector_normalize(vector_by_matrix(face.v[i].no, matrix))
                vertex  = vertices[face.v[i].index] = Vertex(submesh, coord, normal)

                influences = data.getVertexInfluences(face.v[i].index)
                if not influences:
                  print "There is a vertex without attachment to a bone in mesh: " + mesh.name
                # sum of influences is not always 1.0 in Blender ?!?!
                sum = 0.0
                for bone_name, weight in influences: sum += weight

                for bone_name, weight in influences:
                  if sum!=0:
                    try:
                        vertex.influences.append(Influence(BONES[bone_name], weight / sum))
                    except:
                        continue
                  else: # we have a vertex that is probably not skinned. export anyway
                    try:
                        vertex.influences.append(Influence(BONES[bone_name], weight))
                    except:
                        continue

              elif not face.smooth:
                # We cannot share vertex for non-smooth faces, since Cal3D does not
                # support vertex sharing for 2 vertices with different normals.
                # => we must clone the vertex.

                old_vertex = vertex
                vertex = Vertex(submesh, vertex.loc, normal)
                vertex.cloned_from = old_vertex
                vertex.influences = old_vertex.influences
                old_vertex.clones.append(vertex)

              if data.hasFaceUV():
                uv = [face.uv[i][0], 1.0 - face.uv[i][1]]
                if not vertex.maps: vertex.maps.append(Map(*uv))
                elif (vertex.maps[0].u != uv[0]) or (vertex.maps[0].v != uv[1]):
                  # This vertex can be shared for Blender, but not for MD5
                  # MD5 does not support vertex sharing for 2 vertices with
                  # different UV texture coodinates.
                  # => we must clone the vertex.

                  for clone in vertex.clones:
                    if (clone.maps[0].u == uv[0]) and (clone.maps[0].v == uv[1]):
                      vertex = clone
                      break
                  else: # Not yet cloned...
                    old_vertex = vertex
                    vertex = Vertex(submesh, vertex.loc, vertex.normal)
                    vertex.cloned_from = old_vertex
                    vertex.influences = old_vertex.influences
                    vertex.maps.append(Map(*uv))
                    old_vertex.clones.append(vertex)

              face_vertices.append(vertex)

            # Split faces with more than 3 vertices
            for i in range(1, len(face.v) - 1):
              Face(submesh, face_vertices[0], face_vertices[i], face_vertices[i + 1])



  # Export animations

  ANIMATIONS = {}
  if arm_action:
    animation = ANIMATIONS[arm_action.getName()] = MD5Animation(skeleton)
    arm_action.setActive(thearmature)
    armData = thearmature.getData()

    currenttime = rangestart # currenttime is blender's framenumber that we iterate over, getting also the frames between keyframes
                    # time is the "realtime", stored in keyframe objects and in animation.duration
    while currenttime <=rangeend:
      context.currentFrame(int(currenttime))
      # Needed to update IPO's value, but probably not the best way for that...
      scene.makeCurrent()
      Blender.Set("curframe", int(currenttime))
      Blender.Window.Redraw()
      # Convert time units from Blender's frame (starting at 1) to seconds
      time = (currenttime - 1.0) / 24.0 #(assuming default 24fps for md5 anim)
      pose = thearmature.getPose()

      for bonename in armData.bones.keys():
        posebonemat = Blender.Mathutils.Matrix(pose.bones[bonename].poseMatrix)

        try:
          bone  = BONES[bonename] #look up md5bone
        except:
          print "found a posebone animating a bone that is not part of the exported armature: ", ipo.getName()
          continue
        if bone.parent: # need parentspace-matrix
          parentposemat = Blender.Mathutils.Matrix(pose.bones[bone.parent.name].poseMatrix)
          posebonemat = posebonemat*parentposemat.invert()
        else:
          posebonemat = posebonemat*thearmature.getMatrix('worldspace')
        loc = [posebonemat[3][0],
            posebonemat[3][1],
            posebonemat[3][2],
            ]
        rot = quaternion_to_md5( rot_to_quaternion( posebonemat.toQuat().normalize() ) )

        animation.addkeyforbone(bone.id, time, loc, rot)
      currenttime += 1


  #if not os.path.exists(SAVE_TO_DIR): os.makedirs(SAVE_TO_DIR)

  # here begins md5mesh and anim output
  # this is how it works
  # first the skeleton is output, using the data that was collected by the above code in this export function
  # then the mesh data is output (into the same md5mesh file)


  # HACK: save all submeshes in the first mesh
  if len(meshes)>1:
    for mesh in range (1, len(meshes)):
      for submesh in meshes[mesh].submeshes:
        submesh.bindtomesh(meshes[0])
  if (meshfilename != ""):
    try:
      file = open(meshfilename, 'w')
    except IOError, (errno, strerror):
      errmsg = "IOError #%s: %s" % (errno, strerror)
    buffer = skeleton.to_md5mesh(len(meshes[0].submeshes))
    #for mesh in meshes:
    buffer = buffer + meshes[0].to_md5mesh()
    file.write(buffer)
    file.close()
    print "saved mesh to " + meshfilename
  else:
    print "No md5mesh file was generated."

##  # this piece of code might be useful for someone, so it is left here in comments
##  # this generates a md5anim file that simply contains the idle pose
##  try:
##    file = open("idlepose.md5anim"), 'w')
##  except IOError, (errno, strerror):
##    errmsg = "IOError #%s: %s" % (errno, strerror)
##  animation = skeleton.to_idlemd5anim() # animation is a MD5Animation object, not an Animation
##  buffer = animation.to_md5anim()
##  file.write(buffer)
##  file.close()

  if len(ANIMATIONS)>0 and animfilename != "":
    anim = ANIMATIONS.values()[0]
    #animfilename = anim.name + ".md5anim"
    try:
      file = open(animfilename, 'w')
    except IOError, (errno, strerror):
      errmsg = "IOError #%s: %s" % (errno, strerror)
##    if len(themd5anim.channels)>0:
##      ##rangestart = themd5anim.channels[0].starttime*themd5anim.channels[1].framerate
##      ##rangeend = themd5anim.channels[0].endtime*themd5anim.channels[1].framerate
##      rangeend = len(themd5anim.channels[0].keys)
##      for ch in themd5anim.channels:
##        ##chstart = ch.starttime*ch.framerate
##        chend = len(ch.keys)
##        ##if rangestart>chstart: rangestart = chstart
##        if rangeend<chend: rangeend = chend

    objects = []
    for submesh in meshes[0].submeshes:
      if len(submesh.weights) > 0:
        obj = Blender.Object.Get(submesh.name)
        objects.append (obj)
    generateboundingbox(objects, anim, [rangestart, rangeend])
    buffer = anim.to_md5anim()
    file.write(buffer)
    file.close()
    print "saved animation to " + animfilename
  else:
    print "No md5anim was generated."

class MD5CameraV10:
  def __init__(self, framerate):
    self.commandline = ""
    self.framerate = framerate
    self.cuts = []
    self.frames = []
  def to_md5camera(self):
    buf = "MD5Version 10\n"
    buf = buf + "commandline \"%s\"\n\n" % (self.commandline)
    buf = buf + "numFrames %i\n" % (len(self.frames))
    buf = buf + "frameRate %i\n" % (self.framerate)
    buf = buf + "numCuts %i\n\n" % (len(self.cuts))
    buf = buf + "cuts {\n"
    for c in self.cuts:
      buf = buf + "\t%i\n" % (c)
    buf = buf + "}\n\n"

    buf = buf + "camera {\n"
    for f in self.frames:
      buf = buf + "\t( %f %f %f ) ( %f %f %f ) %f\n" % (f)
    buf = buf + "}\n\n"
    return buf

def export_camera(camanimfilename, rangestart, rangeend, scaleval):
  global scale
  scale = scaleval
  cams = Blender.Camera.Get()
  scene = Blender.Scene.getCurrent()
  context = scene.getRenderingContext()
  if len(cams)==0: return None
  camobj = Blender.Object.Get(cams[0].name)

  #generate the animation
  themd5cam = MD5CameraV10(context.framesPerSec())
  for i in range(rangestart, rangeend+1):
    context.currentFrame(i)
    scene.makeCurrent()
    Blender.Redraw() # apparently this has to be done to update the object's matrix. Thanks theeth for pointing that out
    loc = camobj.getLocation()
    m1 = camobj.getMatrix('worldspace')

    # this is because blender cams look down their negative z-axis and "up" is y
    # doom3 cameras look down their x axis, "up" is z
    m2 = [[-m1[2][0], -m1[2][1], -m1[2][2], 0.0], [-m1[0][0], -m1[0][1], -m1[0][2], 0.0], [m1[1][0], m1[1][1], m1[1][2], 0.0], [0,0,0,1]]
    qx, qy, qz = quaternion_to_md5( matrix2quaternion(m2) )

    fov = 2 * math.atan(16/cams[0].getLens())*180/math.pi
    themd5cam.frames.append((loc[0]*scale, loc[1]*scale, loc[2]*scale, qx, qy, qz, fov))

  try:
    file = open(camanimfilename, 'w')
  except IOError, (errno, strerror):
    errmsg = "IOError #%s: %s" % (errno, strerror)
  buffer = themd5cam.to_md5camera()
  file.write(buffer)
  file.close()
  print "saved md5animation to " + camanimfilename


print "\nAvailable actions:"
print Blender.Armature.NLA.GetActions().keys()

draw_busy_screen = 0
EVENT_NOEVENT = 1
EVENT_EXPORT = 2
EVENT_QUIT = 3
EVENT_MESHFILENAME = 4
#EVENT_CAMEXPORT = 8
#EVENT_CAM_ANIMFILENAME = 10
# AK 2007 add support for batch runs
EVENT_LOADSETTINGS = 11
EVENT_SAVESETTINGS = 12

EVENT_INSERT = 14
EVENT_INSERT_ALL = 15
EVENT_REMOVE_ALL = 16
EVENT_ANIMFILENAME_FLAG = 1 << 10
EVENT_ACTION_FLAG = 1 << 11
EVENT_DELETE_FLAG = 1 << 12

#md5camanim_filename = Blender.Draw.Create("")

actions = Blender.Armature.NLA.GetActions()

# this is a scale factor for md5 exporting. scaling with BASE_MATRIX won't work correctly
# setting this to 1 (no scaling) might help if your animation looks distorted
scale_slider = Blender.Draw.Create(1.0)
scale = 1.0

scene = Blender.Scene.getCurrent()
settings_default_filename = scene.getName() + '.b2md5'
md5mesh_default_filename = scene.getName() + '.md5mesh'
md5mesh_filename = Blender.Draw.Create( md5mesh_default_filename )

log_buffer = []

def log_print( line ):
  print line
  log_buffer.extend( line.split( '\n' ) )

def log_clear():
  del log_buffer[:]

def log_empty():
  return len( log_buffer ) == 0

def log_display():
  if not log_empty():
    Blender.Draw.PupBlock( "Export Log", log_buffer )
    log_clear()

def actions_empty():
  return len( actions ) == 0

def get_action_names():
  return actions.keys()

def get_action_name( index ):
  if not actions_empty():
    return get_action_names()[ index ]
  else:
    return ""

def get_action_endframe( index ):
  if not actions_empty():
    action_frames = actions.values()[ index ].getFrameNumbers()
    if len( action_frames ) > 0:
      return action_frames[ -1 ]

  return 1

def get_actions_menutext():
  return '|'.join( get_action_names() )

def clamp( min, value, max ):
  if value < min:
    return min
  elif value > max:
    return max
  else:
    return value

class animation_info:
  settings_to_function_mapping = { 'md5mesh' : 'meshfilename', 'md5anim' : 'animfilename',
           'start' : 'rangestart', 'end' : 'rangeend',
           'action' : 'action_name', 'scale' : 'scaleval' }

  def __init__( self ):
    self.action_index = 0
    self.action_index_menu = Blender.Draw.Create(1)
    self.startframe_slider = Blender.Draw.Create(1)
    self.endframe_slider = Blender.Draw.Create( self.get_action_endframe() )
    self.target_filename = Blender.Draw.Create( self.get_default_filename() )

  def get_action_endframe( self ):
    return get_action_endframe( self.action_index )

  def get_action_name( self ):
    return get_action_name( self.action_index )

  def get_default_filename( self ):
    return self.get_action_name() + '.md5anim'

  def update_action_index( self ):
    if self.target_filename.val == self.get_default_filename():
      self.target_filename.val = ''

    startratio = 1.0 * self.startframe_slider.val / self.get_action_endframe()
    endratio = 1.0 * self.endframe_slider.val / self.get_action_endframe()

    self.action_index = self.action_index_menu.val - 1

    self.startframe_slider.val = startratio * self.get_action_endframe()
    self.endframe_slider.val = endratio * self.get_action_endframe()

    if self.target_filename.val == '':
      self.target_filename.val = self.get_default_filename()

  def export( self ):
    export( "", self.target_filename.val, self.startframe_slider.val, self.endframe_slider.val, self.get_action_name(), scale_slider.val)

  def get_settings_dict( self ):
    return { 'target': self.target_filename, 'action': self.get_action_name(),
            'start': self.startframe_slider.val, 'end': self.endframe_slider.val }

  @staticmethod
  def from_settings_dict( dict ):
    self = animation_info()
    self.target_filename.val = dict[ 'target' ]
    self.action_index = get_action_names().index( dict[ 'action' ] )
    self.action_index_menu.val = self.action_index + 1
    self.startframe_slider.val = clamp( 1, dict[ 'start' ], self.get_action_endframe() )
    self.endframe_slider.val = clamp( self.startframe_slider.val, dict[ 'end' ], self.get_action_endframe() )
    return self

  def get_description( self ):
    return "%s[%s:%s] -> '%s'" % (self.get_action_name(), self.startframe_slider.val, self.endframe_slider.val, self.target_filename.val)

animation_infos = []

current_animation_info = None

######################################################
# Callbacks for Window functions
######################################################
def savesettings_callback(filename):
  if filename == "": return
  # do we really want to safe?
  lastcheck = Blender.Draw.PupMenu( "Save settings to '" + filename + "'?%t|Yes%x1|No%x0" )
  if not lastcheck: return
  Blender.Draw.Draw()
  save_settings( filename )
  Blender.Draw.Redraw(1)

def loadsettings_callback(filename):
  if filename == "": return
  Blender.Draw.Draw()
  load_settings( filename )
  Blender.Draw.Redraw(1)

def md5meshname_callback(filename):
  global md5mesh_filename
  md5mesh_filename.val=filename

def md5animname_callback(filename):
  if current_animation_info:
    current_animation_info.target_filename.val = filename
  else:
    print "md5animname_callback called without valid current_animation_info!"

"""
def md5camanimname_callback(filename):
  global md5camanim_filename
  md5camanim_filename.val=filename
"""

######################################################
# save/load functions (Andreas Kirsch 2007)
######################################################
def get_settings():
  return { 'meshtarget': md5mesh_filename.val,
          'scale': scale_slider.val,
          #'cameratarget': md5camanim_filename.val,
          'animations': [info.get_settings_dict() for info in animation_infos]
          }

def apply_settings( settings ):
  global md5mesh_filename, scale_slider, md5camanim_filename, animation_infos
  md5mesh_filename.val = settings[ 'meshtarget' ]
  scale_slider.val = settings[ 'scale' ]
  #md5camanim_filename.val = settings[ 'cameratarget' ]
  animation_infos = [ animation_info.from_settings_dict( dict ) for dict in settings[ 'animations' ] ]

def load_settings( filename ):
  try:
    file = open(filename, 'r')
  except:
    pass
  else:
    settings = eval( file.read() )
    file.close()

    apply_settings( settings )
    print "settings loaded from '%s'" % (filename)

def save_settings( filename ):
  settings = get_settings()

  try:
    file = open(filename, 'w')
  except:
    pass
  else:
    file.write( `settings`.replace("{", "{\n").replace( "}, ", "},\n" ).replace( "[", "[\n" ).replace( "]", "\n]" ) )
    file.close()
    print "settings written to '%s'" % (filename)

def load_settings_from_scene():
  if not 'blender2md5_settings' in scene.properties:
    return

  # dont be an ass if there's an error
  try:
    # we save the settings in string representation because blender only allows int and float sequences..
    apply_settings( eval( scene.properties[ 'blender2md5_settings' ] ) )
  except:
    pass
  else:
    pass

def save_settings_to_scene():
  settings = get_settings()

  #print `settings`
  scene.properties[ 'blender2md5_settings' ] = `settings`

######################################################
# GUI Functions
######################################################
def handle_exit_event():
  save_settings_to_scene()
  Blender.Draw.Exit()

def handle_event(evt, val):
  if evt == Blender.Draw.ESCKEY:
    handle_exit_event()

def handle_button_event(evt):
  global EVENT_NOEVENT, EVENT_EXPORT, EVENT_QUIT, EVENT_MESHFILENAME, EVENT_ANIMFILENAME, EVENT_MESHFILENAME_STRINGBUTTON, EVENT_ANIMFILENAME_STRINGBUTTON
  #global EVENT_CAM_MESHFILENAME, EVENT_CAM_ANIMFILENAME, EVENT_CAMEXPORT
  global EVENT_SETTINGSFILENAME, EVENT_LOADSETTINGS, EVENT_SAVESETTINGS, settings_filename
  global draw_busy_screen, md5mesh_filename, current_animation_info, scale_slider
  global animation_infos
  #global md5camanim_filename
  if evt == EVENT_EXPORT:
    if md5mesh_filename.val == "" and len( animation_infos ) == 0: return

    draw_busy_screen = 1
    Blender.Draw.Draw()

    try:
      export( md5mesh_filename.val, "", 0, 0, 0, scale_slider.val )
    except:
      log_print( "Mesh export failed:\nmesh * %s -> '%s'" % (scale_slider.val, md5mesh_filename.val) )

    for info in animation_infos:
      # maybe the try-except-block should be moved to .export()?
      try:
        info.export()
      except:
        log_print( "Animation export failed:\n%s" % (info.get_description()) )

    log_display()

    draw_busy_screen = 0
    Blender.Draw.Redraw(1)
    return
  if evt == EVENT_QUIT:
    handle_exit_event()
  if evt == EVENT_MESHFILENAME:
    Blender.Window.FileSelector(md5meshname_callback, "Select md5mesh file...", md5mesh_default_filename )
    Blender.Draw.Redraw(1)
  if evt == EVENT_INSERT:
    animation_infos.append( animation_info() )
    Blender.Draw.Redraw(1)
  if evt == EVENT_INSERT_ALL:
    # not very clean but should work for now
    for i in range( len( actions ) ):
      info = animation_info()
      # TODO: add a constructor parameter for this maybe?
      info.action_index_menu.val = i + 1
      info.update_action_index()
      animation_infos.append( info )
    Blender.Draw.Redraw(1)
  if evt == EVENT_REMOVE_ALL:
    animation_infos = []
    Blender.Draw.Redraw(1)
  if evt & EVENT_ANIMFILENAME_FLAG:
    index = evt & ~EVENT_ANIMFILENAME_FLAG
    current_animation_info = animation_infos[ index ]
    Blender.Window.FileSelector( md5animname_callback, "Select md5anim file", current_animation_info.get_default_filename() )
    Blender.Draw.Redraw(1)
  if evt & EVENT_ACTION_FLAG:
    index = evt & ~EVENT_ACTION_FLAG
    animation_infos[ index ].update_action_index()
    Blender.Draw.Redraw(1)
  if evt & EVENT_DELETE_FLAG:
    index = evt & ~EVENT_DELETE_FLAG
    del animation_infos[ index ]
    Blender.Draw.Redraw(1)
  # deactivating camera export for now
  """if evt == EVENT_CAM_ANIMFILENAME:
    Blender.Window.FileSelector(md5camanimname_callback, "Select md5anim file...")
    Blender.Draw.Redraw(1)
  if evt == EVENT_CAMEXPORT:
    if md5camanim_filename.val == "": return
    draw_busy_screen = 1
    Blender.Draw.Draw()
    try:
      export_camera(md5camanim_filename.val, startframe_slider.val, endframe_slider.val, scale_slider.val)
    except:
      log_print( "Camera export failed:\n[%s:%s] * %s -> '%s'" % (startframe_slider.val, endframe_slider.val, scale_slider.val, md5camanim_filename.val) )
    log_display()
    draw_busy_screen = 0
    Blender.Draw.Redraw(1)"""
  if evt == EVENT_SAVESETTINGS:
    Blender.Window.FileSelector(savesettings_callback, "Specify a settings file...", settings_default_filename )
  if evt == EVENT_LOADSETTINGS:
    Blender.Window.FileSelector(loadsettings_callback, "Specify a settings file...", settings_default_filename )

draw_x = 0
draw_y = 0
draw_basex = 0
draw_basey = 0
#specify width and height
draw_defheight = 25
draw_defwidth = 60
draw_horzspace = 20
draw_vertspace = draw_defheight / 4

def draw_reset():
  global draw_x, draw_y, draw_basex, draw_basey
  draw_basex = draw_x = 0
  draw_basey = draw_y = 0

def draw_setbase():
  global draw_basex, draw_basey
  draw_basex = draw_x
  draw_basey = draw_y

def draw_addhorzspace( c = 1 ):
  global draw_x
  draw_x += draw_horzspace * c

def draw_addvertspace( c = 1 ):
  global draw_y
  draw_y += draw_vertspace * c

def draw_newline():
  global draw_x, draw_y
  draw_y += draw_defheight
  draw_x = draw_basex

def draw_button( text, event, tooltip, width = 1, height = 1 ):
  global draw_x, draw_y
  _width = int( width * draw_defwidth )
  _height = int( height * draw_defheight )
  result = Blender.Draw.Button( text, event, draw_x, draw_y, _width, _height, tooltip )
  draw_x += _width
  return result

def draw_string( text, event, default, length, tooltip, width = 1, height = 1 ):
  global draw_x, draw_y
  _width = int( width * draw_defwidth )
  _height = int( height * draw_defheight )
  result = Blender.Draw.String( text, event, draw_x, draw_y, _width, _height, default, length, tooltip )
  draw_x += _width
  return result

def draw_slider( text, event, default, min, max, realtime, tooltip, width = 1, height = 1 ):
  global draw_x, draw_y
  _width = int( width * draw_defwidth )
  _height = int( height * draw_defheight )
  result = Blender.Draw.String( text, event, draw_x, draw_y, _width, _height, default, min, max, realtime, tooltip )
  draw_x += _width
  return result

def draw_slider( text, event, default, min, max, realtime, tooltip, width = 1, height = 1 ):
  global draw_x, draw_y
  _width = int( width * draw_defwidth )
  _height = int( height * draw_defheight )
  result = Blender.Draw.Slider( text, event, draw_x, draw_y, _width, _height, default, min, max, realtime, tooltip )
  draw_x += _width
  return result

def draw_menu( text, event, default, tooltip, width = 1, height = 1 ):
  global draw_x, draw_y
  _width = int( width * draw_defwidth )
  _height = int( height * draw_defheight )
  result = Blender.Draw.Menu( text, event, draw_x, draw_y, _width, _height, default, tooltip )
  draw_x += _width
  return result

def draw_number( text, event, default, min, max, tooltip, width = 1, height = 1 ):
  global draw_x, draw_y
  _width = int( width * draw_defwidth )
  _height = int( height * draw_defheight )
  result = Blender.Draw.Number( text, event, draw_x, draw_y, _width, _height, default, min, max, tooltip )
  draw_x += _width
  return result

def show_gui():
  global EVENT_NOEVENT, EVENT_EXPORT, EVENT_QUIT, EVENT_MESHFILENAME, EVENT_ANIMFILENAME, EVENT_MESHFILENAME_STRINGBUTTON, EVENT_ANIMFILENAME_STRINGBUTTON
  global draw_busy_screen, md5mesh_filename, scale_slider
  global EVENT_SETTINGSFILENAME, EVENT_LOADSETTINGS, EVENT_SAVESETTINGS, settings_filename
  #global EVENT_CAM_MESHFILENAME, EVENT_CAM_ANIMFILENAME, EVENT_CAMEXPORT
  #global md5camanim_filename
  if draw_busy_screen == 1:
    Blender.BGL.glClearColor(0.3,0.3,0.3,1.0)
    Blender.BGL.glClear(Blender.BGL.GL_COLOR_BUFFER_BIT)
    Blender.BGL.glColor3f(1,1,1)
    Blender.BGL.glRasterPos2i(20,25)
    Blender.Draw.Text("Please wait while exporting...")
    return
  Blender.BGL.glClearColor(0.6,0.6,0.6,1.0)
  Blender.BGL.glClear(Blender.BGL.GL_COLOR_BUFFER_BIT)

  draw_reset()
  draw_newline()
  draw_addhorzspace()
  draw_setbase()

  draw_button("Cancel", EVENT_QUIT, "Quit this script")

  draw_button("Export", EVENT_EXPORT, "Start the MD5-export")

  draw_addhorzspace()

  # Andreas Kirsch 2007 - add batch gui stuff
  draw_button("Save Settings", EVENT_SAVESETTINGS, "Save the current settings", 2)
  draw_button("Load Settings", EVENT_LOADSETTINGS, "Load the settings from a file", 2)

  draw_newline()
  draw_addvertspace()

  md5mesh_filename = draw_string("MD5Mesh file:", EVENT_NOEVENT, md5mesh_filename.val, 255, "md5mesh file to generate", 3)
  draw_button("...", EVENT_MESHFILENAME, "Browse for an md5mesh file", 0.3 )

  draw_addhorzspace( 2 )

  scale_slider = draw_slider("Scale:", EVENT_NOEVENT, scale_slider.val, 0.01, 10.0, 0, "Adjust the size factor of the exported object and camera anim", 4)

  draw_newline()
  draw_addvertspace()

  draw_button( "Insert Animation", EVENT_INSERT, "Insert an action/animation to export", 2 )
  draw_addhorzspace()
  draw_button( "Insert All Animations", EVENT_INSERT_ALL, "Insert all actions/animations", 2.5 )
  draw_button( "Remove All Animations", EVENT_REMOVE_ALL, "Remove all actions/animations", 2.5 )

  """Blender.Draw.Button("Export Camera!", EVENT_CAMEXPORT, 2 * space + 2 * button_width, 3 * button_height, 2 * button_width, button_height, "Start the Camera-export")
  md5camanim_filename = Blender.Draw.String("MD5Camera file:", EVENT_NOEVENT, 2 * space +  4 * button_width,3 * button_height, 3 * button_width, button_height, md5camanim_filename.val, 255, "MD5Camera-File to generate")
  Blender.Draw.Button("Browse...", EVENT_CAM_ANIMFILENAME, 2 * space + 7 * button_width, 3 * button_height, button_width, button_height, "Specify md5camera file")
  """

  draw_newline()

  index = 0
  for info in animation_infos:
    draw_newline()

    draw_button("X", index | EVENT_DELETE_FLAG, "Delete an animtion", 0.3 )
    info.action_index_menu = draw_menu( "Actions:%t|" + get_actions_menutext(), index | EVENT_ACTION_FLAG, info.action_index_menu.val, "Name of the Action that should be exported (IPO name prefix)", 2 )
    info.startframe_slider = draw_number( "Start:", EVENT_NOEVENT, info.startframe_slider.val, 1, info.get_action_endframe(), "Start frame to export from", 1.2 )
    info.endframe_slider = draw_number( "End:", EVENT_NOEVENT, info.endframe_slider.val, info.startframe_slider.val, info.get_action_endframe(), "End frame to export from", 1.2 )
    info.target_filename = draw_string("", EVENT_NOEVENT, info.target_filename.val, 255, "md5anim file to export the action to", 3 )
    draw_button("...", index | EVENT_ANIMFILENAME_FLAG, "Browse for an md5anim file", 0.3 )

    index += 1

# try to load settings from the scene
load_settings_from_scene()

Blender.Draw.Register (show_gui, handle_event, handle_button_event)
