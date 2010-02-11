
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

#include "engine.h"

#include "intensity_physics.h"
#include "intensity_physics_realistic.h"
#include "intensity_physics_bullet.h"


void BulletPhysicsEngine::init()
{
	m_collisionConfiguration = new btDefaultCollisionConfiguration();

	m_dispatcher = new btCollisionDispatcher(m_collisionConfiguration);

	btVector3 worldMin(-1000,-1000,-1000); // XXX
	btVector3 worldMax(1000,1000,1000);
	btAxisSweep3* sweepBP = new btAxisSweep3(worldMin,worldMax); // TODO: Consider btDbvtBroadphase, may be faster
	m_overlappingPairCache = sweepBP;

	m_constraintSolver = new btSequentialImpulseConstraintSolver();

    m_dynamicsWorld = new btDiscreteDynamicsWorld(m_dispatcher, m_overlappingPairCache, m_constraintSolver, m_collisionConfiguration);
}

void BulletPhysicsEngine::destroy()
{
}

void BulletPhysicsEngine::clearStaticPolygons()
{
}

// Sauer coordinates are in 'cubes', not metres as in Bullet
#define SAUER_FACTOR 17.0
// Reverse y and z axes
#define FROM_SAUER_VEC(sauervec) ( btVector3(sauervec.x/SAUER_FACTOR, sauervec.z/SAUER_FACTOR, sauervec.y/SAUER_FACTOR) )
#define TO_SAUER_VEC(sauervec, btvec) { sauervec.x = btvec.x()*SAUER_FACTOR; sauervec.y = btvec.z()*SAUER_FACTOR; sauervec.z = btvec.y()*SAUER_FACTOR; }
#define FROM_SAUER_SCALAR(value) ( value/SAUER_FACTOR )

void BulletPhysicsEngine::addStaticPolygon(std::vector<vec> vertexes)
{
//    btBvhTriangleMeshShape shape =  TODO: Use a single one of these for all the world geomerty? Should be much faster. Can
//                                          even be saved to disk.

// XXX "Avoid huge or degenerate triangles in a triangle mesh Keep the size of triangles reasonable, say below 10 units/meters."
// - from PDF
// Perhaps we should subdivide triangles that are too big?
// "Also degenerate triangles with large size ratios between each sides or close to zero area can better be avoided."

	btConvexHullShape* shape = new btConvexHullShape();
    for (unsigned int i = 0; i < vertexes.size(); i++)
    {
        vec& currVec = vertexes[i];
        btVector3 currBtVec = FROM_SAUER_VEC(currVec);
        shape->addPoint(currBtVec);
    }

	btRigidBody* body = new btRigidBody(0, NULL, shape); // No mass === static geometry

	m_dynamicsWorld->addRigidBody(body);
}


//! Our class for Bullet dynamic entities. Holds everything we need
class IntensityBulletDynamic : public btRigidBody
{

//When rigidbodies are created, they will retrieve the initial worldtransform from the btMotionState,
//using btMotionState::getWorldTransform. When the simulation is running, using
//stepSimulation, the new worldtransform is updated for active rigidbodies using the
//btMotionState::setWorldTransform.


public:
    btVector3 interpolatedPosition, interpolatedVelocity;

    IntensityBulletDynamic(btScalar mass, btMotionState *motionState, btCollisionShape *collisionShape, const btVector3 &localInertia=btVector3(0, 0, 0)) : btRigidBody(mass, motionState, collisionShape, localInertia) { };
};

//! Interface to modify our interpolated values
class IntensityBulletMotionState : public btMotionState
{
public:
    IntensityBulletDynamic* parent;

    virtual void getWorldTransform (btTransform &worldTrans) const
    { printf("getWorldTransform\r\n"); } // No need - we set the position/velocity manually

    virtual void setWorldTransform (const btTransform &worldTrans)
    {
        parent->interpolatedPosition = worldTrans.getOrigin();
        parent->interpolatedVelocity = parent->getLinearVelocity();
    }
};

typedef std::map<physicsHandle, IntensityBulletDynamic*> handleDynamicMap_t;
handleDynamicMap_t handleDynamicMap;
int handleDynamicCounter = 0;

physicsHandle BulletPhysicsEngine::addDynamic(float mass, float radius)
{
    // TODO: You can share Shapes, to save memory etc.
	btSphereShape* shape = new btSphereShape( FROM_SAUER_SCALAR(radius) ); // TODO: Use correct radius
    IntensityBulletMotionState* motionState = new IntensityBulletMotionState();
	IntensityBulletDynamic* body = new IntensityBulletDynamic(mass, motionState, shape); // No mass === static geometry
    motionState->parent = body;
	m_dynamicsWorld->addRigidBody(body);

    physicsHandle handle = handleDynamicCounter;
    handleDynamicMap[handle] = body;
    handleDynamicCounter += 1; // TODO: Handle overflow etc. etc. etc.

    return handle; // XXX garbage collect ***shape***. Also body also motionstate in previous func, etc.}

void BulletPhysicsEngine::removeDynamic(physicsHandle handle)
{
    IntensityBulletDynamic* body = handleDynamicMap[handle];
    m_dynamicsWorld->removeRigidBody(body);
    handleDynamicMap.erase(handle);
    // TODO: Counter stuff
}

void BulletPhysicsEngine::setDynamicPosition(physicsHandle handle, const vec& position)
{
    IntensityBulletDynamic* body = handleDynamicMap[handle];

    btTransform transform;
    
    transform.setIdentity();
    transform.setOrigin( FROM_SAUER_VEC(position) );
    body->setCenterOfMassTransform(transform);

    // Save in interpolated values, since we might read them soon
    body->interpolatedPosition = transform.getOrigin();

    body->activate();
}

void BulletPhysicsEngine::setDynamicVelocity(physicsHandle handle, const vec& velocity)
{
    IntensityBulletDynamic* body = handleDynamicMap[handle];

    btVector3 btVelocity =  FROM_SAUER_VEC(velocity);
    body->setLinearVelocity( btVelocity );

    // Save in interpolated values, since we might read them soon
    body->interpolatedVelocity = btVelocity;

    body->activate();
}

void BulletPhysicsEngine::getDynamic(physicsHandle handle, vec& position, vec& velocity)
{
    IntensityBulletDynamic* body = handleDynamicMap[handle];

    btVector3 btPosition = body->getCenterOfMassPosition();
    btVector3 btVelocity = body->getLinearVelocity();
//    btVector3 btPosition = body->interpolatedPosition;
//    btVector3 btVelocity = body->interpolatedVelocity;
    TO_SAUER_VEC( position, btPosition );
    TO_SAUER_VEC( velocity, btVelocity );
}

void BulletPhysicsEngine::simulate(float seconds)
{
    m_dynamicsWorld->stepSimulation(seconds);
}





// Utilities

void jiggle()
{
    player->o.z += 5;
}

COMMAND(jiggle, "");

