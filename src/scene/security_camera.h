#ifndef __SECURITY_CAMERA_H__
#define __SECURITY_CAMERA_H__

#include <stdint.h>

#include "levels/level_definition.h"
#include "physics/collision_object.h"
#include "sk64/skeletool_armature.h"

struct SecurityCamera {
    struct CollisionObject collisionObject;
    struct RigidBody rigidBody;
    struct SKArmature armature;
    uint8_t index;

    short dynamicId;
    float fizzleTime;
};

void securityCameraInit(struct SecurityCamera* securityCamera, struct SecurityCameraDefinition* definition, int index);
int securityCameraUpdate(struct SecurityCamera* securityCamera);
void securityCameraOnDeserialize(struct SecurityCamera* securityCamera);

struct SecurityCamera* securityCameraNew(struct SecurityCameraDefinition* definition, int index);
void securityCameraDelete(struct SecurityCamera* securityCamera);

void securityCameraCheckPortal(struct SecurityCamera* securityCamera, struct Box3D* portalBox);

void securityCameraDetach(struct SecurityCamera* securityCamera);
int securityCameraIsDetached(struct SecurityCamera* securityCamera);

#endif
