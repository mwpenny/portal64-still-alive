#include "point_constraint.h"

#include "../util/time.h"

#define BREAK_CONSTRAINT_DISTANCE 2.0f

int pointConstraintMoveToPoint(struct CollisionObject* object, struct Vector3* worldPoint, float maxImpulse) {
    struct RigidBody* rigidBody = object->body;

    struct Vector3 targetVelocity;
    vector3Sub(worldPoint, &rigidBody->transform.position, &targetVelocity);

    if (vector3MagSqrd(&targetVelocity) > BREAK_CONSTRAINT_DISTANCE * BREAK_CONSTRAINT_DISTANCE) {
        return 0;
    }

    vector3Scale(&targetVelocity, &targetVelocity, 1.0f / FIXED_DELTA_TIME);

    struct ContactManifold* contact = contactSolverNextManifold(&gContactSolver, object, NULL);

    while (contact) {
        struct Vector3 contactNormal;

        if (contact->shapeB == object) {
            contactNormal = contact->normal;
        } else {
            vector3Negate(&contact->normal, &contactNormal);
        }

        float overlap = vector3Dot(&contactNormal, &targetVelocity);

        if (overlap < 0.0f) {
            vector3AddScaled(&targetVelocity, &contact->normal, -overlap * 0.7f, &targetVelocity);
        }

        contact = contactSolverNextManifold(&gContactSolver, object, contact);
    }

    struct Vector3 delta;
    vector3Sub(&targetVelocity, &rigidBody->velocity, &delta);

    float deltaSqrd = vector3MagSqrd(&delta);
    if (deltaSqrd < maxImpulse * maxImpulse) {
        rigidBody->velocity = targetVelocity;
    } else {
        vector3AddScaled(&rigidBody->velocity, &delta, maxImpulse / sqrtf(deltaSqrd), &rigidBody->velocity);
    }

    return 1;
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

void pointConstraintInit(struct PointConstraint* constraint, struct CollisionObject* object, float maxPosImpulse, float maxRotImpulse) {
    constraint->nextConstraint = NULL;
    constraint->object = object;
    constraint->targetPos = object->body->transform.position;
    constraint->targetRot = object->body->transform.rotation;
    constraint->maxPosImpulse = maxPosImpulse;
    constraint->maxRotImpulse = maxRotImpulse;
}

void pointConstraintUpdateTarget(struct PointConstraint* constraint, struct Vector3* worldPoint, struct Quaternion* worldRotation) {
    constraint->targetPos = *worldPoint;
    constraint->targetRot = *worldRotation;
}