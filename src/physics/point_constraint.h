#ifndef __POINT_CONSTRAINT_H__
#define __POINT_CONSTRAINT_H__

#include "rigid_body.h"

void pointConstraintMoveToPoint(struct RigidBody* rigidBody, struct Vector3* worldPoint, float maxImpulse);
void pointConstraintRotateTo(struct RigidBody* rigidBody, struct Quaternion* worldRotation, float maxImpulse);

#endif