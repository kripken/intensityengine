/*
 *=============================================================================
 * Copyright (C) 2008 Alon Zakai ('Kripken') kripkensteiner@gmail.com
 *
 * This file is part of the Intensity Engine project,
 *    http://www.intensityengine.com
 *
 * The Intensity Engine is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * The Intensity Engine is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with the Intensity Engine.  If not, see
 *     http://www.gnu.org/licenses/
 *     http://www.gnu.org/licenses/agpl-3.0.html
 *=============================================================================
 */

// This file reflects the work in utility.py and steering.py, implementing
// the same functionality in C++. This is necessary for speed reasons, as
// steering operations are calculated for each NPC and for each frame. Sadly
// this leads to some code duplication. Note that we still need the Python
// versions as steering-related operations *are* still possible in Python,
// e.g., planning where to go, so long as they are not intensive and done
// again and again on each frame.

#include "cube.h"
#include "engine.h"


//======================================================================================
// Python/C disparities management
//
// Python has all entity positions at their 'feet', while Sauer has dynents at their
// head. Here are some utilities to help convert.
//======================================================================================

vec foot_position(dynent* d)
{
    vec ret(d->o);
    ret.z -= (d->eyeheight + d->aboveeye);
    return ret;
}

//=====================
// utility.py functions
//=====================

float sign(float x)
{
    if (x > 0)
        return 1;
    else if (x < 0)
        return -1;
    else
        return 0;
}

//! See documentation in utility.py
float distance_between(vec& a, vec& b)
{
    return sqrt( (a.x-b.x)*(a.x-b.x) + (a.y-b.y)*(a.y-b.y) + (a.z-b.z)*(a.z-b.z) );
}

//! See documentation in utility.py
float normalize_angle(float angle, float relative_to)
{
    while (angle < relative_to - 180.0)
        angle += 360.0;
    while (angle > relative_to + 180.0)
        angle -= 360.0;
    return angle;
}

//! See documentation in utility.py
float yaw_to(vec& origin, vec& target, bool reverse=false)
{
    if (!reverse)
        return (-atan2(target.x - origin.x, target.y - origin.y)/(PI/180.0)) + 180.0;
    else
        return yaw_to(target, origin); // Slow, TODO: write this explicitly
}

//! See documentation in utility.py
float pitch_to(vec& origin, vec& target, bool reverse=false)
{
    if (!reverse)
    {
        float radians = asin( (target.z - origin.z) / distance_between(origin, target) );
        return 360.*radians/(2.*PI);
    }
    else
        return pitch_to(target, origin); // Slow, TODO: write this explicitly
}

//! See documentation in utility.py
bool has_line_of_sight(vec& a, vec& b)
{
    vec target;
    bool ret = raycubelos(a, b, target);
    return ret;
}

//! See documentation in utility.py
float ray_collision_distance(vec& origin, vec& ray)
{
    vec hitpos;
    return raycubepos(origin, ray, hitpos, ray.magnitude(), RAY_CLIPMAT|RAY_POLY);
}

//! See documentation in utility.py
float floor_distance(vec& origin, float distance)
{
    vec floor;
    return rayfloor(origin, floor, 0, distance);
}

//! See documentation in utility.py
float highest_floor_distance(vec& origin, float distance, float radius)
{
    float ret = floor_distance(origin, distance);

    for (int x = -1; x <= +1; x++) //in [ -radius/2, 0, +radius/2 ]:
        for (int y = -1; y <= 1; y++)
        {
            vec curr(origin);
            curr.add(vec(float(x) * radius/2, float(y) * radius/2, 0));
            ret = min(ret, floor_distance(curr, distance));
        }

    return ret;
}

//! See documentation in utility.py
float lowest_floor_distance(vec& origin, float distance, float radius)
{
    float ret = floor_distance(origin, distance);

    for (int x = -1; x <= +1; x++) //in [ -radius/2, 0, +radius/2 ]:
        for (int y = -1; y <= 1; y++)
        {
            vec curr(origin);
            curr.add(vec(float(x) * radius/2, float(y) * radius/2, 0));
            ret = max(ret, floor_distance(curr, distance));
        }

    return ret;
}


//======================
// steering.py functions
//======================

//! See documentation in steering.py
float move_angle_towards(float angle, float desired_angle, float speed)
{
    angle = normalize_angle(angle, desired_angle);

    float original_sign = sign(desired_angle - angle);
    angle += speed * original_sign;
    if (sign(desired_angle - angle) == -original_sign && original_sign != 0) // We have overshot
        angle = desired_angle;

    return angle;
}

//! See documentation in steering.py
bool face_towards(dynent* actor, vec& target, bool reverse, float speed)
{
    vec foot_o = foot_position(actor);

    float target_yaw = yaw_to(foot_o, target, reverse);
    actor->yaw = move_angle_towards(actor->yaw, target_yaw, speed);

    float target_pitch = pitch_to(foot_o, target, reverse);
    actor->pitch = move_angle_towards(actor->pitch, target_pitch, speed);

    return fabs(actor->yaw - target_yaw) < 1.0;
}

bool face_towards_helper(int actor_addr, float targetx, float targety, float targetz, bool reverse, float speed)
{
    vec target(targetx, targety, targetz);
    return face_towards((dynent*)actor_addr, target, reverse, speed);
}

//! See documentation in steering.py
bool move_towards(dynent* actor, vec& target, bool reverse, float facing_speed, float desired_distance)
{
    vec foot_o = foot_position(actor);

    bool facing_complete = face_towards(actor, target, reverse, facing_speed);

    bool distance_complete = distance_between(foot_o, target) <= desired_distance;

    actor->move = 0; // By default, do not move forward

    if (!distance_complete)
    {
        float target_yaw = yaw_to(foot_o, target, reverse);

        // Move forward, if facing closely enough
        if (fabs(target_yaw - actor->yaw) <= 30.0)
        {
            // Check if our current facing is reasonable in that moving forward will not hit something or fall us off a cliff

// #            ahead_point = actor.position + actor.movement_speed*0.2*actor.velocity # Where we will soon be in 0.2 seconds
//
// #            if distance_between(ahead_point, actor) < actor.radius/4:# or
// #               can_walk_distance(actor, (ahead_point, actor.radius)): # Too slow, to do on every movement!
            actor->move = +1; // We are facing closely enough to the target, and also can move forward without something bad happening,
                              // so go for it
        }
    }

    return (facing_complete && distance_complete);
}

bool move_towards_helper(int actor_addr, float targetx, float targety, float targetz, bool reverse, float facing_speed, float desired_distance)
{
    vec target(targetx, targety, targetz);
    return move_towards((dynent*)actor_addr, target, reverse, facing_speed, desired_distance);
}

#if 0 // TODO
//! See documentation in steering.py
def is_path_blocked(actor, target, cache_calculations=True):
    actor = worldobjectify(actor)
    target = worldobjectify(target)

    if cache_calculations:
        ret = use_cached_result(actor, target, "is_path_blocked")
        if ret is not None:
            return ret

    ray = target.position - actor.position
    mag = ray.magnitude()

    # Check for obstacles in our path
    clear_distance_head   = ray_collision_distance(actor.position, ray)
    clear_distance_middle = ray_collision_distance(actor.position - Vector3(0, 0, actor.eye_height/2), ray)

    clear_distance = max(clear_distance_head, clear_distance_middle) # Need at least one to be ok, to 'see' the target

    ret = clear_distance < (mag-actor.radius/4) # Whether it appears we can walk to the target without hitting something

    if cache_calculations:
        set_cached_result(actor, target, "is_path_blocked", ret)

    return ret


//! See documentation in steering.py
def can_walk_distance(actor, target, radii_to_check=2, step_fraction=3, cache_calculations=True):
    actor = worldobjectify(actor)
    target = worldobjectify(target)

    if cache_calculations:
        ret = use_cached_result(actor, target, "can_walk_distance")
        if ret is not None:
            return ret

    # Check at increments of radius/3, since radius is about what we need to not fall
    step_size = actor.radius/step_fraction
    steps = radii_to_check * step_fraction

    curr_pos = actor.position

    move_vec = target.position - actor.position
    move_vec.normalize()
    move_vec *= step_size

    full_height = actor.eye_height + actor.above_eye

    # How much of a floor difference we can accept per step. This can be going up or going down.
    allowable_difference_per_step = 2 * full_height * (step_size/actor.radius) # Normalized relative to radius

    # Calculate the current floor distance carefully. Just a single point is not enough, since we might be overhanging a bit.
    # So try some points around as well. We take the highest floor, that is what we are perched upon
    last_floor_dist = highest_floor_distance(curr_pos, 2.*full_height, actor.radius)

    ret = True # Unless there is a problem, all is ok

    # Take steps, checking in each
    for i in range(steps):
        curr_pos = curr_pos + move_vec

        # Check for floor. We take the highest, i.e., deepest floor, the maximum, as we want to avoid overhanging
        floor_dist = highest_floor_distance(curr_pos, last_floor_dist + full_height/2, actor.radius)

        if abs(floor_dist - last_floor_dist) > allowable_difference_per_step:
            ret = False
            break

        last_floor_dist = floor_dist

    if cache_calculations:
        set_cached_result(actor, target, "can_walk_distance", ret)

    return ret
#endif

