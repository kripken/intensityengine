
// Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
// This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

//! A physics engine that works in a realistic manner, using mass, forces, etc.
//! Basically all standalone physics engines like Bullet should inherit from
//! this. SauerPhysicsEngine, on the other hand, uses Sauerbraten's internal
//! physics method (which does not use realistic masses, forces, etc.)
class RealisticPhysicsEngine : public PhysicsEngine
{
public:
/*
    virtual void init();

    virtual void clear();

    virtual void addStaticPolygon(std::vector<vec> vertexes);

    virtual void* addDynamic(float mass, float radius);
    virtual void removeDynamic(void* handle);
    virtual void setDynamicPosition(void* handle, const vec& position);
    virtual void setDynamicVelocity(void* handle, const vec& velocity);
    virtual void getDynamic(void* handle, vec& position, vec& velocity);

    virtual void simulate(float seconds);
*/
};

