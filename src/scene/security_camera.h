#ifndef __SECURITY_CAMERA_H__
#define __SECURITY_CAMERA_H__

#include "../physics/collision_object.h"
#include "../levels/level_definition.h"
#include "../sk64/skelatool_armature.h"

struct SecurityCamera {
    struct CollisionObject collisionObject;
    struct RigidBody rigidBody;
    struct SKArmature armature;
    short dynamicId;
};

void securityCameraInit(struct SecurityCamera* securityCamera, struct SecurityCameraDefinition* definition);

#endif