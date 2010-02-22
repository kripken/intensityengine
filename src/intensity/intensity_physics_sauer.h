
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


class SauerPhysicsEngine : public PhysicsEngine
{
public:
    virtual void init();

    virtual void destroy();

    virtual physicsHandle addSphere(float mass, float radius) { assert(0); return 0; };
    virtual physicsHandle addBox(float mass, float rx, float ry, float rz) { assert(0); return 0; };
    virtual physicsHandle addCapsule(float mass, float radius, float height) { assert(0); return 0; };

    virtual void removeBody(physicsHandle handle) { assert(0); };
    virtual void removeConstraint(physicsHandle handle) { assert(0); };

    virtual void setBodyEntity(physicsHandle handle, CLogicEntity* entity) { assert(0); };

    virtual void setBodyPosition(physicsHandle handle, const vec& position) { assert(0); };
    virtual void setBodyRotation(physicsHandle handle, const quat& rotation) { assert(0); };
    virtual void setBodyVelocity(physicsHandle handle, const vec& velocity) { assert(0); };
    virtual void addBodyImpulse(physicsHandle handle, const vec& impulse) { assert(0); };

    virtual void getBodyPosition(physicsHandle handle, vec& position) { assert(0); };
    virtual void getBodyRotation(physicsHandle handle, quat& rotation) { assert(0); };
    virtual void getBodyVelocity(physicsHandle handle, vec& velocity) { assert(0); };

    virtual void setLinearFactor(physicsHandle handle, vec& factor) { assert(0); };
    virtual void setAngularFactor(physicsHandle handle, vec& factor) { assert(0); };

    virtual physicsHandle addConstraintP2P(physicsHandle handleA, physicsHandle handleB, vec& pivotA, vec& pivotB) { assert(0); };

    virtual void simulate(float seconds);

    virtual bool isColliding(vec& position, float radius, CLogicEntity *ignore=NULL);
};

