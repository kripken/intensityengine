
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


#include "btBulletDynamicsCommon.h"


class BulletPhysicsEngine : public RealisticPhysicsEngine
{
    btBroadphaseInterface*	m_overlappingPairCache;
	btCollisionDispatcher*	m_dispatcher;
	btConstraintSolver*	m_constraintSolver;
	btDefaultCollisionConfiguration* m_collisionConfiguration;
    btDynamicsWorld *m_dynamicsWorld;

    //! Adds a bullet shape as a dynamic element. Takes ownership of 'shape'.
    physicsHandle addDynamic(btCollisionShape *shape, float mass);

public:
    virtual void init();

    virtual void destroy();

    virtual void clearStaticPolygons();
    virtual bool requiresStaticPolygons() { return false; };
    virtual void addStaticPolygon(std::vector<vec> vertexes);

    virtual bool requiresStaticCubes() { return true; };
    virtual void addStaticCube(vec o, vec r);

    virtual physicsHandle addDynamicSphere(float mass, float radius);
    virtual physicsHandle addDynamicBox(float mass, float rx, float ry, float rz);
    virtual void removeDynamic(physicsHandle handle);
    virtual void setDynamicPosition(physicsHandle handle, const vec& position);
    virtual void setDynamicVelocity(physicsHandle handle, const vec& velocity);
    virtual void getDynamic(physicsHandle handle, vec& position, vec& velocity);

    virtual void simulate(float seconds);
};

