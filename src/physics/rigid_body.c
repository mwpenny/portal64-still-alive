#include "rigid_body.h"
#include "../util/time.h"
#include "../physics/config.h"
#include "contact_solver.h"
#include "defs.h"
#include <math.h>

void rigidBodyInit(struct RigidBody* rigidBody, struct ColliderTypeData* collider, float mass) {
    rigidBody->collider = collider;

    transformInitIdentity(&rigidBody->transform);
    rigidBody->velocity = gZeroVec;
    rigidBody->angularVelocity = gZeroVec;

    rigidBody->mass = mass;
    rigidBody->massInv = 1.0f / mass;

    rigidBody->momentOfInertia = collider->callbacks->mofICalculator(collider, mass);
    rigidBody->momentOfInertiaInv = 1.0f / rigidBody->momentOfInertia;
}

void rigidBodyAppyImpulse(struct RigidBody* rigidBody, struct Vector3* worldPoint, struct Vector3* impulse) {
    struct Vector3 offset;
    vector3Sub(worldPoint, &rigidBody->transform.position, &offset);

    struct Vector3 torque;
    vector3Cross(&offset, impulse, &torque);

    vector3AddScaled(&rigidBody->angularVelocity, &torque, rigidBody->momentOfInertiaInv, &rigidBody->angularVelocity);
    vector3AddScaled(&rigidBody->velocity, impulse, rigidBody->massInv, &rigidBody->velocity);
}

#define ENERGY_SCALE_PER_STEP   0.99f

void rigidBodyUpdate(struct RigidBody* rigidBody) {
    rigidBody->velocity.y += GRAVITY_CONSTANT * FIXED_DELTA_TIME;

    vector3AddScaled(&rigidBody->transform.position, &rigidBody->velocity, FIXED_DELTA_TIME, &rigidBody->transform.position);
    quatApplyAngularVelocity(&rigidBody->transform.rotation, &rigidBody->angularVelocity, FIXED_DELTA_TIME, &rigidBody->transform.rotation);

    vector3Scale(&rigidBody->velocity, &rigidBody->velocity, ENERGY_SCALE_PER_STEP);
    vector3Scale(&rigidBody->angularVelocity, &rigidBody->angularVelocity, ENERGY_SCALE_PER_STEP);
}

void rigidBodyVelocityAtLocalPoint(struct RigidBody* rigidBody, struct Vector3* localPoint, struct Vector3* worldVelocity) {
    vector3Cross(&rigidBody->angularVelocity, localPoint, worldVelocity);
    vector3Add(worldVelocity, &rigidBody->velocity, worldVelocity);
}

void rigidBodyVelocityAtWorldPoint(struct RigidBody* rigidBody, struct Vector3* worldPoint, struct Vector3* worldVelocity) {
    struct Vector3 relativePos;
    vector3Sub(worldPoint, &rigidBody->transform.position, &relativePos);
    vector3Cross(&rigidBody->angularVelocity, &relativePos, worldVelocity);
    vector3Add(worldVelocity, &rigidBody->velocity, worldVelocity);
}

float rigidBodyMassInverseAtLocalPoint(struct RigidBody* rigidBody, struct Vector3* localPoint, struct Vector3* normal) {
    struct Vector3 crossPoint;
    vector3Cross(localPoint, normal, &crossPoint);
    return rigidBody->massInv + rigidBody->momentOfInertiaInv * vector3MagSqrd(&crossPoint);
}

void rigidBodyResolveContact(struct RigidBody* bodyA, struct RigidBody* bodyB, struct ContactPoint* contactPoint) {
    struct Vector3 aVelocity;
    struct Vector3 bVelocity;

    struct Vector3 localPositionA;
    struct Vector3 localPositionB;

    if (bodyA) {
        vector3Sub(&contactPoint->point, &bodyA->transform.position, &localPositionA);
        rigidBodyVelocityAtLocalPoint(bodyA, &localPositionA, &aVelocity);
    } else {
        aVelocity = gZeroVec;
    }

    if (bodyB) {
        vector3Sub(&contactPoint->point, &bodyB->transform.position, &localPositionB);
        rigidBodyVelocityAtLocalPoint(bodyB, &localPositionB, &bVelocity);
    } else {
        bVelocity = gZeroVec;
    }

    struct Vector3 relativeVelocity;
    vector3Sub(&aVelocity, &bVelocity, &relativeVelocity);

    float massInverse = 0.0f;
    float frictionMassInverse = 0.0f;
    float bounce = 1.0f;
    float friction = 0.0f;

    float normalVelocity = vector3Dot(&relativeVelocity, &contactPoint->normal);

    struct Vector3 tangentVelocity;
    vector3AddScaled(&relativeVelocity, &contactPoint->normal, -normalVelocity, &tangentVelocity);

    struct Vector3 tangentDirection;
    vector3Normalize(&tangentVelocity, &tangentDirection);

    if (bodyA) {
        massInverse += rigidBodyMassInverseAtLocalPoint(bodyA, &localPositionA, &contactPoint->normal);
        frictionMassInverse += rigidBodyMassInverseAtLocalPoint(bodyA, &localPositionA, &tangentDirection);
        bounce = bodyA->collider->bounce;
        friction = bodyA->collider->friction;
    }

    if (bodyB) {
        massInverse += rigidBodyMassInverseAtLocalPoint(bodyB, &localPositionA, &contactPoint->normal);
        frictionMassInverse += rigidBodyMassInverseAtLocalPoint(bodyB, &localPositionB, &tangentDirection);
        bounce = MIN(bounce, bodyB->collider->bounce);
        friction = MAX(friction, bodyB->collider->friction);
    }

    if (massInverse == 0.0f) {
        // two immovable objects
        return;
    }

    struct Vector3 tangentImpulse;

    float tangentSpeed = sqrt(vector3MagSqrd(&tangentVelocity));

    massInverse = 1.0f / massInverse;
    frictionMassInverse = 1.0f / frictionMassInverse;

    float impulse = -(1.0f + bounce) * normalVelocity * massInverse;
    impulse += contactPoint->intersectionDepth * (0.5f * FIXED_DELTA_TIME);

    float frictionImpulse = impulse * friction;
    float tangentImpulseMag = tangentSpeed * frictionMassInverse;

    if (tangentImpulseMag > frictionImpulse) {
        tangentImpulseMag = frictionImpulse;
    }

    if (tangentImpulseMag > 0.00001f) {
        vector3Scale(&tangentDirection, &tangentImpulse, -tangentImpulseMag);
    }

    struct Vector3 impulseVector;
    vector3AddScaled(&tangentImpulse, &contactPoint->normal, impulse, &impulseVector);
    if (bodyA) {
        rigidBodyAppyImpulse(bodyA, &contactPoint->point, &impulseVector);
    }

    if (bodyB) {
        vector3Scale(&impulseVector, &impulseVector, -impulse);
        rigidBodyAppyImpulse(bodyB, &contactPoint->point, &impulseVector);
    }
}

void rigidBodyCollideWithPlane(struct RigidBody* rigidBody, struct Plane* plane, struct ContactSolver* contactSolver) {
    CollideWithPlane planeCollider = rigidBody->collider->callbacks->collideWithPlane;

    if (!planeCollider) {
        return;
    }

    struct ContactConstraintState* contact = &contactSolver->contacts[contactSolver->contactCount];

    // int prevCount = contact->contactCount;

    int collisionCount = planeCollider(rigidBody->collider->data, &rigidBody->transform, plane, contact);

    if (collisionCount) {
        contact->bodyB = rigidBody;
        // if (contact->bodyB != rigidBody || contact->contactCount != prevCount) {

        //     for (int i = 0; i < contact->contactCount; ++i) {
        //     }
        // }
        ++contactSolver->contactCount;
    } else {
        contact->bodyB = NULL;
    }
}