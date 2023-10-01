#include "point_constraint.h"

#include "../util/time.h"

#define BREAK_CONSTRAINT_DISTANCE 1.25f
#define CLAMP_CONSTRAINT_DISTANCE 0.07f

void pointConstraintTargetVelocity(struct RigidBody* rigidBody, struct Vector3* targetVelocity, float maxImpulse, float movementScaleFactor) {
    struct Vector3 delta;
    vector3Sub(targetVelocity, &rigidBody->velocity, &delta);
    
    float deltaSqrd = vector3MagSqrd(&delta);
    if (deltaSqrd < maxImpulse * maxImpulse) {
        vector3Scale(targetVelocity, &rigidBody->velocity, movementScaleFactor);
    } else {
        vector3AddScaled(&rigidBody->velocity, &delta, maxImpulse / sqrtf(deltaSqrd), &rigidBody->velocity);
    }
}

int pointConstraintMoveToPoint(struct CollisionObject* object, struct Vector3* worldPoint, float maxImpulse, float movementScaleFactor) {
    struct RigidBody* rigidBody = object->body;

    struct Vector3 targetVelocity;
    vector3Sub(worldPoint, &rigidBody->transform.position, &targetVelocity);

    if (vector3MagSqrd(&targetVelocity) > BREAK_CONSTRAINT_DISTANCE * BREAK_CONSTRAINT_DISTANCE) {
        return 0;
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
            vector3AddScaled(&targetVelocity, &contact->normal, -overlap * 0.85f, &targetVelocity);
        }

        contact = contactSolverNextManifold(&gContactSolver, object, contact);
    }

    pointConstraintTargetVelocity(rigidBody, &targetVelocity, maxImpulse, movementScaleFactor);

    return 1;
}

void pointConstraintRotateTo(struct RigidBody* rigidBody, struct Quaternion* worldRotation, float maxImpulse) {
    struct Quaternion inverseRigidBody;
    quatConjugate(&rigidBody->transform.rotation, &inverseRigidBody);

    struct Quaternion deltaAngle;
    quatMultiply(worldRotation, &inverseRigidBody, &deltaAngle);

    if (deltaAngle.w < 0.0f) {
        quatNegate(&deltaAngle, &deltaAngle);
    }

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

void pointConstraintInit(struct PointConstraint* constraint, struct CollisionObject* object, float maxPosImpulse, float maxRotImpulse, float movementScaleFactor) {
    constraint->nextConstraint = NULL;
    constraint->object = object;
    constraint->targetPos = object->body->transform.position;
    constraint->targetRot = object->body->transform.rotation;
    constraint->maxPosImpulse = maxPosImpulse;
    constraint->maxRotImpulse = maxRotImpulse;
    constraint->movementScaleFactor = movementScaleFactor;
    constraint->object->body->flags &= ~RigidBodyIsSleeping;
    constraint->object->body->sleepFrames = IDLE_SLEEP_FRAMES;
}

void pointConstraintUpdateTarget(struct PointConstraint* constraint, struct Vector3* worldPoint, struct Quaternion* worldRotation) {
    constraint->targetPos = *worldPoint;
    constraint->targetRot = *worldRotation;
    constraint->object->body->flags &= ~RigidBodyIsSleeping;
    constraint->object->body->sleepFrames = IDLE_SLEEP_FRAMES;
}