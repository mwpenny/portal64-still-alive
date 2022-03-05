#include "rigid_body.h"
#include "../util/time.h"


void rigidBodyInit(struct RigidBody* rigidBody, struct ColliderTypeData* collider, float mass) {
    rigidBody->collisionType = collider->type;
    rigidBody->collisionShape = collider->data;

    transformInitIdentity(&rigidBody->transform);
    rigidBody->velocity = gZeroVec;
    rigidBody->angularVelocity = gZeroVec;

    rigidBody->mass = mass;
    rigidBody->mass = 1.0f / mass;

    rigidBody->momentOfInertia = collider->mofICalculator(collider, mass);
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

void rigidBodyUpdate(struct RigidBody* rigidBody) {
    vector3AddScaled(&rigidBody->transform.position, &rigidBody->velocity, FIXED_DELTA_TIME, &rigidBody->transform.position);
    quatApplyAngularVelocity(&rigidBody->transform.rotation, &rigidBody->angularVelocity, FIXED_DELTA_TIME, &rigidBody->transform.rotation);
}