#include "security_camera.h"

#include "defs.h"
#include "../physics/collision_box.h"
#include "../physics/collision_scene.h"
#include "dynamic_scene.h"
#include "scene.h"
#include "../util/time.h"
#include "../levels/cutscene_runner.h"

#include "../build/assets/models/props/security_camera.h"
#include "../build/assets/materials/static.h"
#include "../build/src/audio/clips.h"

struct CollisionBox gSecurityCameraCollisionBox = {
    {0.15, 0.3f, 0.35f}
};

struct ColliderTypeData gSecurityCameraCollider = {
    CollisionShapeTypeBox,
    &gSecurityCameraCollisionBox,
    0.0f,
    1.0f,
    &gCollisionBoxCallbacks
};

#define CAMERA_RANGE    10.0f

struct Quaternion gBarBoneRelative = {1.0f, 0.0f, 0.0f, 0.0f};

void securityCameraLookAt(struct SecurityCamera* camera, struct Vector3* target) {
    if (!(camera->rigidBody.flags & RigidBodyIsKinematic)) {
        return;
    }

    struct Vector3 offset;
    vector3Sub(target, &camera->rigidBody.transform.position, &offset);

    float magSqrd = vector3MagSqrd(&offset);

    if (magSqrd > CAMERA_RANGE * CAMERA_RANGE || magSqrd < 0.01f) {
        return;
    }

    float invMag = 1.0f / sqrtf(magSqrd);

    struct Vector2 pitch;
    pitch.y = offset.y * invMag;

    if (fabsf(pitch.y) > 0.99f) {
        pitch.x = 0.0f;
        quatAxisComplex(&gRight, &pitch, &camera->armature.pose[PROPS_SECURITY_CAMERA_HEAD_BONE].rotation);
        return;
    }

    pitch.x = sqrtf(offset.x * offset.x + offset.z * offset.z) * invMag;

    struct Quaternion relativeRotation;
    quatAxisComplex(&gForward, &pitch, &camera->armature.pose[PROPS_SECURITY_CAMERA_HEAD_BONE].rotation);

    struct Vector2 yaw;

    invMag = invMag / pitch.x;

    yaw.x = -offset.z * invMag;
    yaw.y = offset.x * invMag;

    quatAxisComplex(&gUp, &yaw, &relativeRotation);
    quatMultiply(&gBarBoneRelative, &relativeRotation, &camera->armature.pose[PROPS_SECURITY_CAMERA_BAR_BONE].rotation);
}

void securityCameraRender(void* data, struct DynamicRenderDataList* renderList, struct RenderState* renderState) {
    struct SecurityCamera* securityCamera = (struct SecurityCamera*)data;

    Mtx* matrix = renderStateRequestMatrices(renderState, 1);

    if (!matrix) {
        return;
    }

    transformToMatrixL(&securityCamera->rigidBody.transform, matrix, SCENE_SCALE);

    Mtx* armature = renderStateRequestMatrices(renderState, PROPS_SECURITY_CAMERA_DEFAULT_BONES_COUNT);

    if (!armature) {
        return;
    }

    securityCameraLookAt(securityCamera, &gScene.player.lookTransform.position);

    skCalculateTransforms(&securityCamera->armature, armature);

    dynamicRenderListAddData(
        renderList, 
        props_security_camera_model_gfx, 
        matrix, 
        SECURITY_CAMERA_INDEX, 
        &securityCamera->rigidBody.transform.position, 
        armature
    );
}

void securityCameraInit(struct SecurityCamera* securityCamera, struct SecurityCameraDefinition* definition) {
    collisionObjectInit(&securityCamera->collisionObject, &gSecurityCameraCollider, &securityCamera->rigidBody, 1.0f, COLLISION_LAYERS_TANGIBLE);
    rigidBodyMarkKinematic(&securityCamera->rigidBody);
    collisionSceneAddDynamicObject(&securityCamera->collisionObject);

    skArmatureInit(&securityCamera->armature, &props_security_camera_armature);

    securityCamera->rigidBody.transform.position = definition->position;
    securityCamera->rigidBody.transform.rotation = definition->rotation;
    securityCamera->rigidBody.transform.scale = gOneVec;
    securityCamera->rigidBody.currentRoom = definition->roomIndex;

    collisionObjectUpdateBB(&securityCamera->collisionObject);

    securityCamera->dynamicId = dynamicSceneAdd(securityCamera, securityCameraRender, &securityCamera->rigidBody.transform.position, 0.4f);

    dynamicSceneSetRoomFlags(securityCamera->dynamicId, ROOM_FLAG_FROM_INDEX(securityCamera->rigidBody.currentRoom));
}

short gCameraDestroyClips[] = {
    SOUNDS_GENERIC_SECURITY_CAMERA_DESTROYED_1,
    SOUNDS_GENERIC_SECURITY_CAMERA_DESTROYED_2,
    SOUNDS_GENERIC_SECURITY_CAMERA_DESTROYED_3,
    SOUNDS_GENERIC_SECURITY_CAMERA_DESTROYED_4,
    SOUNDS_GENERIC_SECURITY_CAMERA_DESTROYED_5,
};

void securityCamerasCheckPortal(struct SecurityCamera* securityCameras, int cameraCount, struct Box3D* portalBox) {
    for (int i = 0; i < cameraCount; ++i) {
        struct SecurityCamera* camera = &securityCameras[i];
        if (!(camera->rigidBody.flags & RigidBodyIsKinematic)) {
            // already free skip this one
            continue;
        }
        if (box3DHasOverlap(&camera->collisionObject.boundingBox, portalBox)) {
            rigidBodyUnmarkKinematic(&camera->rigidBody, 1.0f, collisionBoxSolidMofI(&gSecurityCameraCollider, 1.0f));
            camera->collisionObject.collisionLayers |= COLLISION_LAYERS_GRABBABLE;
            camera->rigidBody.flags |= RigidBodyFlagsGrabbable;
            short clipIndex = randomInRange(0, sizeof(gCameraDestroyClips) / sizeof(*gCameraDestroyClips));
            cutsceneQueueSoundInChannel(gCameraDestroyClips[clipIndex], CH_GLADOS, 1.0f);
        }
    }
}