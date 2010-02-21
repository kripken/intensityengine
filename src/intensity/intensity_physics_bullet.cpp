
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
#define FROM_SAUER_VEC_NORM(sauervec) ( btVector3(sauervec.x, sauervec.z, sauervec.y) )
#define TO_SAUER_VEC(sauervec, btvec) { sauervec.x = btvec.x()*SAUER_FACTOR; sauervec.y = btvec.z()*SAUER_FACTOR; sauervec.z = btvec.y()*SAUER_FACTOR; }
#define TO_SAUER_QUAT(sauerquat, btquat) { sauerquat.x = -btquat.x(); sauerquat.y = -btquat.z(); sauerquat.z = -btquat.y(); sauerquat.w = btquat.w(); }
#define FROM_SAUER_QUAT(sauerquat) ( btQuaternion(-sauerquat.x, -sauerquat.z, -sauerquat.y, sauerquat.w) )
#define FROM_SAUER_SCALAR(value) ( value/SAUER_FACTOR )


//! Our class for Bullet dynamic entities. Holds everything we need
class IntensityBulletBody : public btRigidBody
{
public:
    btVector3 interpolatedPosition, interpolatedVelocity;
    btQuaternion interpolatedRotation;
    bool isStatic;

    IntensityBulletBody(btScalar mass, btMotionState *motionState, btCollisionShape *collisionShape, const btVector3 &localInertia) : btRigidBody(mass, motionState, collisionShape, localInertia)
    {
        isStatic = (mass == 0);
        interpolatedRotation.setEuler(0,0,0);
    };
};

//! Interface to modify our interpolated values
class IntensityBulletMotionState : public btMotionState
{
public:
    IntensityBulletBody* parent;

    virtual void getWorldTransform (btTransform &worldTrans) const
    { /* printf("getWorldTransform\r\n"); */ } // No need - we set the position/velocity manually

    virtual void setWorldTransform (const btTransform &worldTrans)
    {
        parent->interpolatedPosition = worldTrans.getOrigin();
        parent->interpolatedRotation = worldTrans.getRotation();
        parent->interpolatedVelocity = parent->getLinearVelocity();
    }
};

typedef std::map<physicsHandle, IntensityBulletBody*> handleBodyMap_t;
handleBodyMap_t handleBodyMap;
int handleBodyCounter = 0;


#ifdef CLIENT
    //! The green axis is 'forward'
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
//            particle_flare(sauerTo, sauerFrom, 0, PART_STREAK, VEC_TO_COLOR(toColor));
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

    m_broadPhase = new btDbvtBroadphase();

    m_constraintSolver = new btSequentialImpulseConstraintSolver();

    m_dynamicsWorld = new btDiscreteDynamicsWorld(m_dispatcher, m_broadPhase, m_constraintSolver, m_collisionConfiguration);

    m_dynamicsWorld->setGravity(btVector3(0,-10,0));

    // Debug
    #ifdef CLIENT
        m_debugDrawer = new SauerDebugDrawer();
        m_debugDrawer->setDebugMode(btIDebugDraw::DBG_DrawWireframe);
        m_dynamicsWorld->setDebugDrawer(m_debugDrawer);
    #endif

    m_staticTriangleVertices = NULL;
    m_staticTriangleIndexes = NULL;
    m_indexVertexArrays = NULL;
    m_globalStaticGeometry = NULL;
}

void BulletPhysicsEngine::destroy()
{
    std::vector<physicsHandle> toErase;
    for(handleBodyMap_t::iterator iter = handleBodyMap.begin(); iter != handleBodyMap.end(); iter++)
        toErase.push_back(iter->first);
    for (unsigned int i = 0; i < toErase.size(); i++)
        removeBody(toErase[i]);

    delete m_dynamicsWorld;
    delete m_dispatcher;
    delete m_collisionConfiguration;
    delete m_broadPhase;
    delete m_constraintSolver;

    DELETEP(m_globalStaticGeometry);
    DELETEP(m_indexVertexArrays);
    DELETEP(m_staticTriangleVertices);
    DELETEP(m_staticTriangleIndexes);

    #ifdef CLIENT
        delete m_debugDrawer;
    #endif
}

void BulletPhysicsEngine::clearStaticGeometry()
{
    std::vector<physicsHandle> toErase;
    for(handleBodyMap_t::iterator iter = handleBodyMap.begin(); iter != handleBodyMap.end(); iter++)
    {
        IntensityBulletBody* body = iter->second;
        if (body->isStatic)
            toErase.push_back(iter->first);
    }

    for (unsigned int i = 0; i < toErase.size(); i++)
        removeBody(toErase[i]);

    // Prepare global static
    if (requiresStaticPolygons())
    {
        m_staticTriangles.clear();
    }
}

void BulletPhysicsEngine::addStaticPolygon(std::vector<vec> vertexes)
{
// XXX "Avoid huge or degenerate triangles in a triangle mesh Keep the size of triangles reasonable, say below 10 units/meters."
// - from PDF
// Perhaps we should subdivide triangles that are too big?
// "Also degenerate triangles with large size ratios between each sides or close to zero area can better be avoided."

    // Simple, naive method: a convex hull for each polygon.
    /*
    btConvexHullShape* shape = new btConvexHullShape();
    for (unsigned int i = 0; i < vertexes.size(); i++)
    {
        vec& currVec = vertexes[i];
        btVector3 currBtVec = FROM_SAUER_VEC(currVec);
        shape->addPoint(currBtVec);
    }
    addBody(shape, 0);
    */

    // btBvhTriangleMeshShape method
    assert(vertexes.size() == 3);
    for (unsigned int i = 0; i < vertexes.size(); i++)
    {
        vec& currVec = vertexes[i];
        btVector3 currBtVec = FROM_SAUER_VEC(currVec);
        m_staticTriangles.push_back(currBtVec);
    }
}

void BulletPhysicsEngine::finalizeStaticGeometry()
{
    if (!requiresStaticPolygons()) return;

    assert(m_staticTriangles.size() % 3 == 0);
    int num3 = m_staticTriangles.size();
    int num = num3/3;

    DELETEP(m_staticTriangleVertices);
    m_staticTriangleVertices = new btVector3[num3];
    DELETEP(m_staticTriangleIndexes);
    m_staticTriangleIndexes = new int[num3];

    // TODO: Merge shared verts?
    for (int i = 0; i < num3; i++)
    {
        m_staticTriangleVertices[i] = m_staticTriangles[i];
        m_staticTriangleIndexes[i] = i;
    }

    int vertStride = sizeof(btVector3);
    int indexStride = 3*sizeof(int);
    DELETEP(m_indexVertexArrays);
    m_indexVertexArrays = new btTriangleIndexVertexArray(
        num,
        m_staticTriangleIndexes,
        indexStride,
        num3,
        (btScalar*) &m_staticTriangleVertices[0].x(),
        vertStride
    );

    DELETEP(m_globalStaticGeometry);
    m_globalStaticGeometry = new btBvhTriangleMeshShape(m_indexVertexArrays, true);
    addBody(m_globalStaticGeometry, 0); // We rely on removal of static bodies elsewhere in the code
}

physicsHandle BulletPhysicsEngine::addBody(btCollisionShape *shape, float mass)
{
    btVector3 localInertia(0, 0, 0);
    if (mass > 0)
        shape->calculateLocalInertia(mass, localInertia);
    IntensityBulletMotionState* motionState = new IntensityBulletMotionState();
    IntensityBulletBody* body = new IntensityBulletBody(mass, motionState, shape, localInertia);
    motionState->parent = body;

    m_dynamicsWorld->addRigidBody(body);

    physicsHandle handle = handleBodyCounter;
    handleBodyMap[handle] = body;
    handleBodyCounter += 1; // TODO: Handle overflow etc. etc. etc.

    return handle; // XXX garbage collect ***shape***. Also body also motionstate in previous func, etc.}

void BulletPhysicsEngine::removeBody(physicsHandle handle)
{
    assert(handleBodyMap.count(handle) == 1);
    IntensityBulletBody* body = handleBodyMap[handle];
    assert(body);
    m_dynamicsWorld->removeRigidBody(body);
    handleBodyMap.erase(handle);
}

physicsHandle BulletPhysicsEngine::addConstraint(btTypedConstraint *constraint)
{
// TODO: a map for these, so can remove them etc.
    m_dynamicsWorld->addConstraint(constraint);
    //constraint->setDbgDrawSize(btScalar(5.f));
    return 0; // XXX
}

void BulletPhysicsEngine::removeConstraint(physicsHandle handle)
{
    assert(0);
}

void BulletPhysicsEngine::addStaticCube(vec o, vec r)
{
    btVector3 halfExtents = FROM_SAUER_VEC(r);
    physicsHandle handle = addBody(new btBoxShape(halfExtents), 0);
    setBodyPosition(handle, o);
}

void BulletPhysicsEngine::addStaticConvex(std::vector<vec>& vecs)
{
    unsigned int i;
    vec center(0);
    for (i = 0; i < vecs.size(); i++)
        center.add(vecs[i]);
    center.mul(1.0/vecs.size());

    btConvexHullShape *convex = new btConvexHullShape();
    for (i = 0; i < vecs.size(); i++)
    {
        vec rel = vecs[i];
        rel.sub(center);
        btVector3 btRel = FROM_SAUER_VEC(rel);
        convex->addPoint(btRel);
    }
    physicsHandle handle = addBody(convex, 0);
    setBodyPosition(handle, center);
}

physicsHandle BulletPhysicsEngine::addSphere(float mass, float radius)
{
    return addBody(new btSphereShape( FROM_SAUER_SCALAR(radius) ), mass);
}

physicsHandle BulletPhysicsEngine::addBox(float mass, float rx, float ry, float rz)
{
    vec sauerVec(rx/2, ry/2, rz/2);
    btVector3 halfExtents = FROM_SAUER_VEC( sauerVec );
    return addBody(new btBoxShape(halfExtents), mass);
}

physicsHandle BulletPhysicsEngine::addCapsule(float mass, float radius, float height)
{
    return addBody(new btCapsuleShape(FROM_SAUER_SCALAR(radius), FROM_SAUER_SCALAR(height)), mass);
}

void BulletPhysicsEngine::setBodyPosition(physicsHandle handle, const vec& position)
{
    IntensityBulletBody* body = handleBodyMap[handle];
    assert(body);

    btTransform transform;
    
    transform.setIdentity();
    transform.setOrigin( FROM_SAUER_VEC(position) );
    transform.setRotation(body->getOrientation());
//    transform.setRotation(body->interpolatedRotation); // XXX?
    body->setCenterOfMassTransform(transform);

    // Save in interpolated values, since we might read them soon
    body->interpolatedPosition = transform.getOrigin();

    body->activate();
}

void BulletPhysicsEngine::setBodyRotation(physicsHandle handle, const quat& rotation)
{
    IntensityBulletBody* body = handleBodyMap[handle];
    assert(body);

    btQuaternion btRotation = FROM_SAUER_QUAT(rotation);
    btTransform transform;
    
    transform.setIdentity();
    transform.setOrigin(body->getCenterOfMassTransform().getOrigin());
    transform.setRotation( btRotation );
    body->setCenterOfMassTransform(transform);

    // Save in interpolated values, since we might read them soon
    body->interpolatedRotation = btRotation;

    body->activate();
}

void BulletPhysicsEngine::setBodyVelocity(physicsHandle handle, const vec& velocity)
{
    IntensityBulletBody* body = handleBodyMap[handle];
    assert(body);

    btVector3 btVelocity =  FROM_SAUER_VEC(velocity);
    body->setLinearVelocity( btVelocity );

    // Save in interpolated values, since we might read them soon
    body->interpolatedVelocity = btVelocity;

    body->activate();
}

void BulletPhysicsEngine::addBodyImpulse(physicsHandle handle, const vec& impulse)
{
    IntensityBulletBody* body = handleBodyMap[handle];
    assert(body);

    body->applyCentralImpulse(FROM_SAUER_VEC(impulse));

    body->activate();
}

void BulletPhysicsEngine::getBodyPosition(physicsHandle handle, vec& position)
{
    IntensityBulletBody* body = handleBodyMap[handle];
    assert(body);

    btTransform trans = body->getWorldTransform();
    btVector3 btPosition = trans.getOrigin();
    TO_SAUER_VEC( position, btPosition );
}

void BulletPhysicsEngine::getBodyRotation(physicsHandle handle, quat& rotation)
{
    IntensityBulletBody* body = handleBodyMap[handle];
    assert(body);

    btTransform trans = body->getWorldTransform();
    btQuaternion btRotation = trans.getRotation();
    TO_SAUER_QUAT( rotation, btRotation );
}

void BulletPhysicsEngine::getBodyVelocity(physicsHandle handle, vec& velocity)
{
    IntensityBulletBody* body = handleBodyMap[handle];
    assert(body);

    btVector3 btVelocity = body->getLinearVelocity();
    TO_SAUER_VEC( velocity, btVelocity );
}

void BulletPhysicsEngine::setLinearFactor(physicsHandle handle, vec& factor)
{
    IntensityBulletBody* body = handleBodyMap[handle];
    assert(body);

    body->setLinearFactor(FROM_SAUER_VEC_NORM(factor));
}

void BulletPhysicsEngine::setAngularFactor(physicsHandle handle, vec& factor)
{
    IntensityBulletBody* body = handleBodyMap[handle];
    assert(body);

    body->setAngularFactor(FROM_SAUER_VEC_NORM(factor));
}

physicsHandle BulletPhysicsEngine::addConstraintP2P(physicsHandle handleA, physicsHandle handleB, vec& pivotA, vec& pivotB)
{
    IntensityBulletBody* bodyA = handleBodyMap[handleA];
    IntensityBulletBody* bodyB = handleBodyMap[handleB];
    assert(bodyA);
    assert(bodyB);

    return addConstraint(new btPoint2PointConstraint(
        *bodyA,
        *bodyB,
        FROM_SAUER_VEC(pivotA),
        FROM_SAUER_VEC(pivotB)
    ));
}

VAR(bulletdebug, 0, 0, 1);

void BulletPhysicsEngine::simulate(float seconds)
{
    m_dynamicsWorld->stepSimulation(seconds);

    #ifdef CLIENT
        if (bulletdebug)
        {
            m_debugDrawer->setDebugMode(btIDebugDraw::DBG_DrawWireframe);
            m_dynamicsWorld->debugDrawWorld();
        } else
            m_debugDrawer->setDebugMode(0);
    #endif

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

