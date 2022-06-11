#include "point_constraint.h"

#include "../util/time.h"

void pointConstraintMoveToPoint(struct CollisionObject* object, struct Vector3* worldPoint, float maxImpulse) {
    struct RigidBody* rigidBody = object->body;

    vector3Sub(worldPoint, &rigidBody->transform.position, &rigidBody->velocity);
    struct Vector3 targetVelocity;
    vector3Scale(&rigidBody->velocity, &targetVelocity, 1.0f / FIXED_DELTA_TIME);

    struct Vector3 delta;
    vector3Sub(&targetVelocity, &rigidBody->velocity, &delta);

    // struct ContactManifold* manifold = contactSolverNextManifold(&gContactSolver, object, NULL);

    // while (manifold) {
    //     int isFirst = manifold->shapeA == object;



    //     manifold = contactSolverNextManifold(&gContactSolver, object, manifold);
    // }

    float deltaSqrd = vector3MagSqrd(&delta);
    if (deltaSqrd < maxImpulse * maxImpulse) {
        rigidBody->velocity = targetVelocity;
    } else {
        vector3AddScaled(&rigidBody->velocity, &delta, maxImpulse / sqrtf(deltaSqrd), &rigidBody->velocity);
    }
}

void pointConstraintRotateTo(struct RigidBody* rigidBody, struct Quaternion* worldRotation, float maxImpulse) {
    struct Quaternion inverseRigidBody;
    quatConjugate(&rigidBody->transform.rotation, &inverseRigidBody);

    struct Quaternion deltaAngle;
    quatMultiply(worldRotation, &inverseRigidBody, &deltaAngle);

    struct Vector3 axis;
    float angle;
    quatDecompose(&deltaAngle, &axis, &angle);

    struct Vector3 targetVelocity;
    vector3Scale(&axis, &targetVelocity, angle * (1.0f / FIXED_DELTA_TIME));

    struct Vector3 delta;
    vector3Sub(&targetVelocity, &rigidBody->angularVelocity, &delta);

    float deltaSqrd = vector3MagSqrd(&delta);
    if (deltaSqrd < maxImpulse * maxImpulse) {
        rigidBody->angularVelocity = targetVelocity;
    } else {
        vector3AddScaled(&rigidBody->angularVelocity, &delta, maxImpulse / sqrtf(deltaSqrd), &rigidBody->angularVelocity);
    }
}