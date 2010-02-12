
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


// Sauer coordinates are in 'cubes', not metres as in Bullet
#define SAUER_FACTOR 17.0
// Reverse y and z axes
#define FROM_SAUER_VEC(sauervec) ( btVector3(sauervec.x/SAUER_FACTOR, sauervec.z/SAUER_FACTOR, sauervec.y/SAUER_FACTOR) )
#define TO_SAUER_VEC(sauervec, btvec) { sauervec.x = btvec.x()*SAUER_FACTOR; sauervec.y = btvec.z()*SAUER_FACTOR; sauervec.z = btvec.y()*SAUER_FACTOR; }
#define FROM_SAUER_SCALAR(value) ( value/SAUER_FACTOR )


#ifdef CLIENT
    class SauerDebugDrawer : public btIDebugDraw
    {
        int m_debugMode;

    public:
        SauerDebugDrawer() : m_debugMode(0) { };

        virtual void drawLine(const btVector3 &from, const btVector3 &to, const btVector3 &fromColor, const btVector3 &toColor)
        {
            vec sauerFrom, sauerTo;
            TO_SAUER_VEC( sauerFrom, from );
            TO_SAUER_VEC( sauerTo, to );

            #define VEC_TO_COLOR(it) \
                ((int((it.x()*0.5+0.5)*255)<<16) + (int((it.y()*0.5+0.5)*255)<<8) + int((it.z()*0.5+0.5)*255))
            particle_flare(sauerFrom, sauerTo, 0, PART_STREAK, VEC_TO_COLOR(fromColor));
            particle_flare(sauerTo, sauerFrom, 0, PART_STREAK, VEC_TO_COLOR(toColor));
        }

        virtual void drawLine(const btVector3 &from, const btVector3 &to, const btVector3 &color)
        {
            drawLine(from, to, color, color);
        }

        virtual void draw3dText (const btVector3 &location, const char *textString)
        {
            vec sauerLocation;
            TO_SAUER_VEC( sauerLocation, location );
            particle_text(sauerLocation, textString, PART_TEXT, 0);
        }

        virtual void drawContactPoint (const btVector3 &PointOnB, const btVector3 &normalOnB, btScalar distance, int lifeTime, const btVector3 &color) { }
        virtual void reportErrorWarning (const char *warningString) { printf("Bullet warning: %s\r\n", warningString); }
        virtual void setDebugMode(int debugMode) { m_debugMode = debugMode; }
        virtual int getDebugMode() const { return m_debugMode; }
    };
#endif

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

    // Debug
    #ifdef CLIENT
        SauerDebugDrawer* debug = new SauerDebugDrawer(); // XXX leak
        debug->setDebugMode(btIDebugDraw::DBG_DrawWireframe);
        m_dynamicsWorld->setDebugDrawer(debug);
    #endif
}

void BulletPhysicsEngine::destroy()
{
}

void BulletPhysicsEngine::clearStaticPolygons()
{
}

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

physicsHandle BulletPhysicsEngine::addDynamic(btCollisionShape *shape, float mass)
{
    IntensityBulletMotionState* motionState = new IntensityBulletMotionState();
	IntensityBulletDynamic* body = new IntensityBulletDynamic(mass, motionState, shape); // No mass === static geometry
    motionState->parent = body;
	m_dynamicsWorld->addRigidBody(body);

    physicsHandle handle = handleDynamicCounter;
    handleDynamicMap[handle] = body;
    handleDynamicCounter += 1; // TODO: Handle overflow etc. etc. etc.

    return handle; // XXX garbage collect ***shape***. Also body also motionstate in previous func, etc.}

physicsHandle BulletPhysicsEngine::addDynamicSphere(float mass, float radius)
{
    return addDynamic(new btSphereShape( FROM_SAUER_SCALAR(radius) ), mass);
}

physicsHandle BulletPhysicsEngine::addDynamicBox(float mass, float rx, float ry, float rz)
{
    btVector3 halfExtents(FROM_SAUER_SCALAR(rx/2), FROM_SAUER_SCALAR(ry/2), FROM_SAUER_SCALAR(rz/2));
    return addDynamic(new btBoxShape(halfExtents), mass);
}

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

//    btVector3 btPosition = body->getCenterOfMassPosition();
//    btVector3 btVelocity = body->getLinearVelocity();
    btVector3 btPosition = body->interpolatedPosition;
    btVector3 btVelocity = body->interpolatedVelocity;
    TO_SAUER_VEC( position, btPosition );
    TO_SAUER_VEC( velocity, btVelocity );
}

VAR(bulletdebug, 0, 0, 1);

void BulletPhysicsEngine::simulate(float seconds)
{
    m_dynamicsWorld->stepSimulation(seconds);

    if (bulletdebug) m_dynamicsWorld->debugDrawWorld();

/*
This will happen in predictplayer: So need to counter it?
        d->o = d->newpos;
        d->yaw = d->newyaw;
        d->pitch = d->newpitch;
*/
// XXX TODO:    game::predictplayer(*, false); - interpolate remote players
}





// Utilities

void jiggle()
{
    player->o.z += 5;
}

COMMAND(jiggle, "");

