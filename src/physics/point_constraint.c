#include "point_constraint.h"

#include "../util/time.h"

#define BREAK_CONSTRAINT_DISTANCE 2.0f
#define CLAMP_CONSTRAINT_DISTANCE 0.07f

int pointConstraintMoveToPoint(struct CollisionObject* object, struct Vector3* worldPoint, float maxImpulse, int teleportOnBreak, float movementScaleFactor) {
    struct RigidBody* rigidBody = object->body;

    struct Vector3 targetVelocity;
    vector3Sub(worldPoint, &rigidBody->transform.position, &targetVelocity);

    if (vector3MagSqrd(&targetVelocity) > BREAK_CONSTRAINT_DISTANCE * BREAK_CONSTRAINT_DISTANCE) {
        if (teleportOnBreak){
            object->body->transform.position = *worldPoint;
            return 1;
        }
        return 0;
    }
    if (teleportOnBreak){
        if (fabsf(sqrtf(vector3DistSqrd(worldPoint, &rigidBody->transform.position))) > CLAMP_CONSTRAINT_DISTANCE){
            while(sqrtf(vector3DistSqrd(worldPoint, &rigidBody->transform.position)) > CLAMP_CONSTRAINT_DISTANCE){
                vector3Lerp(&rigidBody->transform.position, worldPoint, 0.01, &rigidBody->transform.position);
            }       
            vector3Sub(worldPoint, &rigidBody->transform.position, &targetVelocity);
            vector3Scale(&targetVelocity, &targetVelocity, (1.0f / FIXED_DELTA_TIME));
            vector3Scale(&targetVelocity, &targetVelocity, 0.5);
            rigidBody->velocity = targetVelocity;
            return 1;
        }
    }
    
    vector3Scale(&targetVelocity, &targetVelocity, (1.0f / FIXED_DELTA_TIME));

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
        vector3Scale(&targetVelocity, &targetVelocity, movementScaleFactor);
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

void pointConstraintInit(struct PointConstraint* constraint, struct CollisionObject* object, float maxPosImpulse, float maxRotImpulse, int teleportOnBreak, float movementScaleFactor) {
    constraint->nextConstraint = NULL;
    constraint->object = object;
    constraint->targetPos = object->body->transform.position;
    constraint->targetRot = object->body->transform.rotation;
    constraint->maxPosImpulse = maxPosImpulse;
    constraint->maxRotImpulse = maxRotImpulse;
    constraint->teleportOnBreak = teleportOnBreak;
    constraint->movementScaleFactor = movementScaleFactor;
}

void pointConstraintUpdateTarget(struct PointConstraint* constraint, struct Vector3* worldPoint, struct Quaternion* worldRotation) {
    constraint->targetPos = *worldPoint;
    constraint->targetRot = *worldRotation;
}