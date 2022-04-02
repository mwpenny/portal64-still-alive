#include "rigid_body.h"
#include "../util/time.h"
#include "../physics/config.h"
#include "contact_solver.h"
#include "defs.h"
#include <math.h>

void rigidBodyInit(struct RigidBody* rigidBody, float mass, float momentOfIniteria) {
    transformInitIdentity(&rigidBody->transform);
    rigidBody->velocity = gZeroVec;
    rigidBody->angularVelocity = gZeroVec;

    rigidBody->mass = mass;
    rigidBody->massInv = 1.0f / mass;

    rigidBody->momentOfInertia = momentOfIniteria;
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