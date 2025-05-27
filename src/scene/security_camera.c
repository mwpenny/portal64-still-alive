#include "security_camera.h"

#include "decor/decor_object.h"
#include "defs.h"
#include "dynamic_scene.h"
#include "levels/cutscene_runner.h"
#include "physics/collision_box.h"
#include "physics/collision_scene.h"
#include "scene.h"
#include "system/time.h"
#include "util/dynamic_asset_loader.h"

#include "codegen/assets/audio/clips.h"
#include "codegen/assets/materials/static.h"
#include "codegen/assets/models/dynamic_animated_model_list.h"
#include "codegen/assets/models/props/security_camera.h"

struct CollisionBox gSecurityCameraCollisionBox = {
    {0.15f, 0.3f, 0.35f}
};

struct ColliderTypeData gSecurityCameraCollider = {
    CollisionShapeTypeBox,
    &gSecurityCameraCollisionBox,
    0.0f,
    1.0f,
    &gCollisionBoxCallbacks
};

#define CAMERA_RANGE            10.0f
#define CAMERA_RIGID_BODY_MASS  1.0f

#define CAMERA_COLLISION_LAYERS (COLLISION_LAYERS_TANGIBLE | COLLISION_LAYERS_FIZZLER | COLLISION_LAYERS_BLOCK_TURRET_SHOTS)

struct Quaternion gBarBoneRelative = {1.0f, 0.0f, 0.0f, 0.0f};

void securityCameraLookAt(struct SecurityCamera* camera, struct Vector3* target) {
    if (securityCameraIsDetached(camera)) {
        return;
    }

    struct Vector3 offset;
    vector3Sub(target, &camera->rigidBody.transform.position, &offset);

    // don't look directly at the player
    offset.y -= 1.0f;

    float magSqrd = vector3MagSqrd(&offset);

    if (magSqrd > CAMERA_RANGE * CAMERA_RANGE || magSqrd < 0.01f) {
        return;
    }

    struct Quaternion invRotation;
    quatConjugate(&camera->rigidBody.transform.rotation, &invRotation);
    quatMultVector(&invRotation, &offset, &offset);

    float invMag = 1.0f / sqrtf(magSqrd);

    struct Vector2 pitch;
    pitch.y = offset.y * invMag;

    struct Quaternion targetPose;

    if (fabsf(pitch.y) > 0.99f) {
        pitch.x = 0.0f;
        quatAxisComplex(&gForward, &pitch, &targetPose);
        quatLerp(&camera->armature.pose[PROPS_SECURITY_CAMERA_HEAD_BONE].rotation, &targetPose, 0.1f, &camera->armature.pose[PROPS_SECURITY_CAMERA_HEAD_BONE].rotation);
        return;
    }

    pitch.x = -sqrtf(offset.x * offset.x + offset.z * offset.z) * invMag;

    struct Quaternion relativeRotation;
    quatAxisComplex(&gForward, &pitch, &targetPose);
    quatLerp(&camera->armature.pose[PROPS_SECURITY_CAMERA_HEAD_BONE].rotation, &targetPose, 0.1f, &camera->armature.pose[PROPS_SECURITY_CAMERA_HEAD_BONE].rotation);

    struct Vector2 yaw;

    invMag = invMag / pitch.x;

    yaw.x = -offset.x * invMag;
    yaw.y = -offset.z * invMag;

    quatAxisComplex(&gUp, &yaw, &relativeRotation);
    quatMultiply(&gBarBoneRelative, &relativeRotation, &targetPose);
    quatLerp(&camera->armature.pose[PROPS_SECURITY_CAMERA_BAR_BONE].rotation, &targetPose, 0.1f, &camera->armature.pose[PROPS_SECURITY_CAMERA_BAR_BONE].rotation);
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

    dynamicRenderListAddDataTouchingPortal(
        renderList, 
        decorBuildFizzleGfx(securityCamera->armature.displayList, securityCamera->fizzleTime, renderState), 
        matrix, 
        securityCamera->fizzleTime > 0.0f ? SECURITY_CAMERA_FIZZLED_INDEX : SECURITY_CAMERA_INDEX, 
        &securityCamera->rigidBody.transform.position, 
        armature,
        securityCamera->rigidBody.flags
    );
}

void securityCameraInit(struct SecurityCamera* securityCamera, struct SecurityCameraDefinition* definition) {
    struct SKArmatureWithAnimations* armature = dynamicAssetAnimatedModel(PROPS_SECURITY_CAMERA_DYNAMIC_ANIMATED_MODEL);

    collisionObjectInit(&securityCamera->collisionObject, &gSecurityCameraCollider, &securityCamera->rigidBody, 1.0f, CAMERA_COLLISION_LAYERS);
    rigidBodyMarkKinematic(&securityCamera->rigidBody);
    collisionSceneAddDynamicObject(&securityCamera->collisionObject);

    skArmatureInit(&securityCamera->armature, armature->armature);

    securityCamera->rigidBody.transform.position = definition->position;
    securityCamera->rigidBody.transform.rotation = definition->rotation;
    securityCamera->rigidBody.transform.scale = gOneVec;
    securityCamera->rigidBody.currentRoom = definition->roomIndex;

    collisionObjectUpdateBB(&securityCamera->collisionObject);

    securityCamera->dynamicId = dynamicSceneAdd(securityCamera, securityCameraRender, &securityCamera->rigidBody.transform.position, 0.4f);

    dynamicSceneSetRoomFlags(securityCamera->dynamicId, ROOM_FLAG_FROM_INDEX(securityCamera->rigidBody.currentRoom));

    securityCamera->fizzleTime = 0.0f;
}

void securityCameraUpdate(struct SecurityCamera* securityCamera) {
    if (securityCamera->dynamicId == INVALID_DYNAMIC_OBJECT) {
        return;
    }

    if (securityCamera->collisionObject.flags & COLLISION_OBJECT_PLAYER_STANDING) {
        securityCamera->collisionObject.flags &= ~COLLISION_OBJECT_PLAYER_STANDING;
    }
    if (decorObjectUpdateFizzler(&securityCamera->collisionObject, &securityCamera->fizzleTime) == FizzleCheckResultEnd) {
        dynamicSceneRemove(securityCamera->dynamicId);
        collisionSceneRemoveDynamicObject(&securityCamera->collisionObject);
        securityCamera->dynamicId = INVALID_DYNAMIC_OBJECT;
    }

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
        if (securityCameraIsDetached(camera)) {
            // already free skip this one
            continue;
        }
        if (box3DHasOverlap(&camera->collisionObject.boundingBox, portalBox)) {
            securityCameraDetach(camera);

            if (!cutsceneRunnerIsChannelPlaying(CH_GLADOS)) {
                short clipIndex = randomInRange(0, sizeof(gCameraDestroyClips) / sizeof(*gCameraDestroyClips));
                cutsceneQueueSoundInChannel(gCameraDestroyClips[clipIndex], 1.0f, CH_GLADOS, StringIdNone);
            }
        }
    }
}

void securityCameraDetach(struct SecurityCamera* securityCamera) {
    rigidBodyUnmarkKinematic(&securityCamera->rigidBody, CAMERA_RIGID_BODY_MASS, collisionBoxSolidMofI(&gSecurityCameraCollider, CAMERA_RIGID_BODY_MASS));
    securityCamera->collisionObject.collisionLayers |= COLLISION_LAYERS_GRABBABLE;
    securityCamera->rigidBody.flags |= RigidBodyFlagsGrabbable;
}

int securityCameraIsDetached(struct SecurityCamera* securityCamera) {
    return !(securityCamera->rigidBody.flags & RigidBodyIsKinematic);
}