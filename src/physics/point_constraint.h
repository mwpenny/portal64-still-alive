#ifndef __POINT_CONSTRAINT_H__
#define __POINT_CONSTRAINT_H__

#include "rigid_body.h"
#include "collision_object.h"
#include "../math/mathf.h"

struct PointConstraint {
    struct PointConstraint* nextConstraint;
    struct CollisionObject* object;
    struct Vector3 targetPos;
    struct Quaternion targetRot;
    float maxPosImpulse;
    float maxRotImpulse;
    int teleportOnBreak;
    float movementScaleFactor;
};

void pointConstraintInit(struct PointConstraint* constraint, struct CollisionObject* object, float maxPosImpulse, float maxRotImpulse, int teleportOnBreak, float movementScaleFactor);
void pointConstraintUpdateTarget(struct PointConstraint* constraint, struct Vector3* worldPoint, struct Quaternion* worldRotation);

int pointConstraintMoveToPoint(struct CollisionObject* object, struct Vector3* worldPoint, float maxImpulse, int teleportOnBreak, float movementScaleFactor);
void pointConstraintRotateTo(struct RigidBody* rigidBody, struct Quaternion* worldRotation, float maxImpulse);

#endif