#ifndef __GRAB_ROTATION_H__
#define __GRAB_ROTATION_H__

#include "../decor/decor_object_list.h"
#include "../physics/collision_object.h"
#include "../math/transform.h"

enum GrabRotationFlags {
    GrabRotationSnapToCubeNormals = (1 << 0),
    GrabRotationTurnTowardsPlayer = (1 << 1),
    GrabRotationUseZLookDirection = (1 << 2),
};

enum GrabRotationFlags grabRotationFlagsForDecorId(const int decorId);
enum GrabRotationFlags grabRotationFlagsForDecorObjectDef(struct DecorObjectDefinition* decorObjectDef);
enum GrabRotationFlags grabRotationFlagsForCollisionObject(struct CollisionObject* collisionObject);

void grabRotationGetBase(const enum GrabRotationFlags flags, struct Quaternion* objectRotationIn, struct Quaternion* lookRotationIn, struct Quaternion* grabRotationBaseOut);
void grabRotationUpdate(const enum GrabRotationFlags flags, struct Quaternion* lookRotationDeltaIn, struct Quaternion* forwardRotationIn, struct Quaternion* grabRotationBaseIn, struct Quaternion* grabRotationOut);

#endif
