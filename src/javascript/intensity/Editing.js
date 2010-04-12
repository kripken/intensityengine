
// Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
// This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

// Sauer face ('orientation') constants

FACE_ALL = -1;
FACE_SOUTH = 0;
FACE_NORTH = 1;
FACE_WEST = 2;
FACE_EAST = 3;
FACE_DOWN = 4;
FACE_UP = 5;


// Sauer material constants

MATERIAL_AIR   = 0;
MATERIAL_WATER = 1;
MATERIAL_LAVA  = 2;
MATERIAL_GLASS = 3;

//    MAT_NOCLIP = 1 << MATF_CLIP_SHIFT,  // collisions always treat cube as empty
//    MAT_CLIP   = 2 << MATF_CLIP_SHIFT,  // collisions always treat cube as solid
//    MAT_AICLIP = 3 << MATF_CLIP_SHIFT,  // clip monsters only

//    MAT_DEATH  = 1 << MATF_FLAG_SHIFT,  // force player suicide
//    MAT_EDIT   = 4 << MATF_FLAG_SHIFT   // edit-only surfaces


//
// Low-level functions
//

eraseGeometry = CAPI.eraseGeometry;

getWorldSize = CAPI.getWorldSize;

//! @param position The lower edge of the cube (e.g., [512,512,512] is the middle of a normal-sized map)
//! @param gridsize The size of the cube, e.g., 256. Must be a power of 2.
function createCube(position, gridsize) {
    return CAPI.create_cube(integer(position.x), integer(position.y), integer(position.z), gridsize);
}

//! @param position The lower edge of the cube (e.g., [512,512,512] is the middle of a normal-sized map)
//! @param gridsize The size of the cube, e.g., 256. Must be a power of 2.
function deleteCube(position, gridsize) {
    return CAPI.delete_cube(integer(position.x), integer(position.y), integer(position.z), gridsize);
}

//! @param position The lower edge of the cube (e.g., [512,512,512] is the middle of a normal-sized map)
//! @param gridsize The size of the cube, e.g., 256. Must be a power of 2.
function setCubeTexture(position, gridsize, face, texture) {
    return CAPI.set_cube_texture(integer(position.x), integer(position.y), integer(position.z), gridsize, face, texture)
}

//! @param position The lower edge of the cube (e.g., [512,512,512] is the middle of a normal-sized map)
//! @param gridsize The size of the cube, e.g., 256. Must be a power of 2.
function setCubeMaterial(position, gridsize, material) {
    return CAPI.set_cube_material(integer(position.x), integer(position.y), integer(position.z), gridsize, material)
}

//! @param position The lower edge of the cube (e.g., [512,512,512] is the middle of a normal-sized map)
//! @param gridsize The size of the cube, e.g., 256. Must be a power of 2.
function pushCubeCorner(position, gridsize, face, corner, direction) {
    return CAPI.push_cube_corner(integer(position.x), integer(position.y), integer(position.z), gridsize, face, corner, direction)
}

//
// Higher-level functions
//

MAX_SIZE = 1024 // TODO: Sync with getworldsize() with Sauer

/*
//! @param start The lower edge defining the rectangle you want to process
//! @param end The higher edge defining the rectangle you want to process
function processCubeset(start, end, operation, params): // XXX params is problematic
    if start.x >= end.x or start.y >= end.y or start.z >= end.z:
        return // We are done, this space has no volume to speak of

    // Find largest cube size that will fit, and be appropriate, for the space starting from the bottom corner
    size = MAX_SIZE
    while start.x + size > end.x or \
          start.y + size > end.y or \
          start.z + size > end.z or \
          start.x % size != 0 or start.y % size != 0 or start.z % size != 0:
        size /= 2

    // Generate a cube for that space
    operation(start, size, *params)

    // Recursively create cubes for the remaining spaces
    process_cubeset(start + Vector3(0, 0, size), end, operation, params)
    process_cubeset(start + Vector3(size, 0, 0), Vector3(end.x, end.y, start.z + size), operation, params)
    process_cubeset(start + Vector3(0, size, 0), Vector3(start.x + size, end.y, start.z + size), operation, params)

function create_cubeset(start, end):
    process_cubeset(start, end, create_cube)

function delete_cubeset(start, end):
    process_cubeset(start, end, delete_cube)

function set_cubeset_texture(start, end, face, texture):
    process_cubeset(start, end, set_cube_texture, [face, texture])

function set_cubeset_material(start, end, material):
    process_cubeset(start, end, set_cube_material, [material])


//! @param resolution The size of each dimension, i.e., there are resolution^3 data points. These points
//! are interpolated so as to cover the actual worldsize in Sauer. So, if the real worldsize is 1024,
//! and the resolution is 256, then each data point covers a 4x4x4 area in Sauer.
//! @param raw_data An array containing bytes
//! @param smoothing Whether to apply smoothing to the generated map
function createMapFromRaw(resolution, rawData, smoothing) {
    smoothing = defaultValue(smoothing, false);

    info = raw_data.buffer_info()
    assert(info[1] == resolution**3) // Ensure array size is the right size
    CAPI.create_map_from_raw(resolution, info[0], smoothing)

function create_heightmap_from_raw(resolution, raw_data):
    info = raw_data.buffer_info()
    CAPI.create_heightmap_from_raw(resolution, info[0])


//!
function create_staircase(start, stair_size, direction, num_stairs):
    curr_position = start.copy()
    for i in range(num_stairs):
        create_cubeset(curr_position, curr_position + stair_size)
        curr_position += direction


//! @param raw_data An array of size resolution*resolution, with values in 0-1 (0, minimum, 1 maximum)
//! TODO: Make this work in all orientations
function create_heightmap(resolution, raw_data):
    erase_geometry()

    world_size = get_world_size();

    gridsize = world_size / resolution

    function pushing_needed(max_height, curr):
        return integer( min(1.0, (max_height - curr)/gridsize) * 8 )

    for x in range(resolution-1):
        for y in range(resolution-1):
            // Calculate heights at all 4 corners of this square area
            low_x_low_y = raw_data[x*resolution + y] * world_size
            low_x_high_y = raw_data[x*resolution + y+1] * world_size
            high_x_low_y = raw_data[(x+1)*resolution + y] * world_size
            high_x_high_y = raw_data[(x+1)*resolution + y+1] * world_size

            max_height = Math.max(low_x_low_y, low_x_high_y, high_x_low_y, high_x_high_y)
            max_height = math.ceil(max_height / gridsize) * gridsize // We start the cube at the maximum height, then push down

            position = Vector3(x * gridsize, y * gridsize, max_height - gridsize)

            create_cube(position, gridsize)
            set_cube_texture(position, gridsize, FACE_ALL, 5)

            lower_position = position - Vector3(0, 0, gridsize)
            create_cube(lower_position, gridsize) // Create a cube below to hide artifacts
            set_cube_texture(lower_position, gridsize, FACE_ALL, 5)

            left = pushing_needed(max_height, low_x_low_y)
            for i in range(left):
                push_cube_corner(position, gridsize, FACE_UP, 0, 1)

            left = pushing_needed(max_height, high_x_low_y)
            for i in range(left):
                push_cube_corner(position, gridsize, FACE_UP, 1, 1)

            left = pushing_needed(max_height, low_x_high_y)
            for i in range(left):
                push_cube_corner(position, gridsize, FACE_UP, 2, 1)

            left = pushing_needed(max_height, high_x_high_y)
            for i in range(left):
                push_cube_corner(position, gridsize, FACE_UP, 3, 1)
*/

