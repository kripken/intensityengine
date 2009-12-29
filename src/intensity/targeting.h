
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


//! Manages relating mouse positions, world positions, and logic entities

struct TargetingControl
{
#ifdef CLIENT
    static void setupOrientation();

    //! Contains the latest and current information about what world geometry (cube) the mouse cursor is hovering over
    static vec           worldPosition;       // Position in cube geometry

    //! Contains the position where the mouse cursor is aiming. Equal to worldposition in general, unless hovering
    //! on an entity
    static vec targetPosition;

    //! Contains the latest and current information about what logic entity the mouse cursor is hovering over
    static LogicEntityPtr targetLogicEntity;
#endif

    //! Utility that wraps around sauer's complex system for intersecting a ray (from->to) with a dynamic entity
    static void intersectClosestDynamicEntity(vec &from, vec &to, physent *targeter, float &dist, dynent*& target);

    //! Utility that wraps around sauer's complex system for intersecting a ray (from->to) with a mapmodel
    static void intersectClosestMapmodel(vec &from, vec &to, float& dist, extentity*& target);

    //! Find the logic entity that the ray from->to intersects, and is not 'targeter' (the entity casting the ray, typically)
    static void intersectClosest(vec &from, vec &to, physent *targeter, float& dist, LogicEntityPtr& entity);

#ifdef CLIENT
    //! Sets or unsets the state of letting the mouse 'target' entities, i.e., mark them
    //! in a visual manner and let clicking affect that entity
    static void setMouseTargeting(bool on);

    //! Called per-frame, sets worldPosition and targetLogicEntity to their appropriate values
    //! @param forceEntityCheck Set to true to find target entities even if default mouse targeting (hover targeting) is off
    static void determineMouseTarget(bool forceEntityCheck=false);
#endif

    //! Calculate the movement of a physical entity. Takes into account both
    //! actual movement since the last calculation here, and the velocity
    static float calculateMovement(physent* entity);

    //! For a viewer position and a target entity, returns how visible it is, i.e., an
    //! estimate of the size of that entity on the viewer's 'screen'. 100% means the entity
    //! fills the screen. This value is useful e.g. to know how accurate physics need to
    //! be - we can do less physics frames for entities that are only a few pixels large.
    //! Note that this function returns a rough estimate, not a precise calculation.
    //! @param cameraPosition The position of the camera, i.e., the viewer
    //! @param fovx The field of view in the X (horizontal) direction
    //! @param fovy The field of view in the Y (vertical) direction
    //! @param cameraDirection Where the camera points to. A normalized vector.
    //! @param cameraRight The right axis of the camera. A normalized vector.
    //! @param cameraUp. The up axis of the camera. A normalized vector.
    //! @param targetPosition The position of the target.
    //! @param targetRadius The radius of the target (for now we use a single dimension).
    //! @return The visibility percentage, a value between 0 and 100. This is a percentage
    //! of the 2D screen area, i.e., it is in 2D size coordinates.
    static float estimateVisibility(vec& cameraPosition, float fovx, float fovy,
                                    vec& cameraDirection, vec& cameraRight, vec& cameraUp,
                                    vec& targetPosition, float targetRadius);

    //! For a viewer position and a target entity, returns how much it might
    //! change, visibly, from the point of view of a camera's 'screen'. 100% means the entity
    //! will potentially move an area size equal to the entire screen, in 1 second's time.
    //! This value is useful e.g. to know how accurate physics need to
    //! be - we can do less physics frames for entities that will only appear to move very little.
    static float estimatePotentialVisibilityChange(vec& cameraPosition, float fovx, float fovy,
                                                   vec& cameraDirection, vec& cameraRight, vec& cameraUp,
                                                   vec& targetPosition, float targetRadius, vec& targetVelocity);

#ifdef CLIENT
    //! Estimates how much the player (as represented by the camera) can see a target physical entity.
    static float estimatePlayerVisiblity(physent* target);

    //! Estimates how much the player (as represented by the camera) will see a visual change in a target physical entity
    //! in 1 second's time.
    static float estimatePlayerPotentialVisiblityChange(physent* target);
#endif

    //! Calculates how long a physics frame should be for a particular physics entity. On
    //! the client, this depends on how visible the other entity is (no need for 200fps of
    //! physics for something that is only a few pixels large), the screen resolution, etc.
    //! On the server, this will (TODO) depend on how the entity is visible by any of
    //! the players.
    static void calcPhysicsFrames(physent *entity);
};

