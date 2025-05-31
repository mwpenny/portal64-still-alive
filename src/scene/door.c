#include "door.h"

#include "defs.h"
#include "graphics/render_scene.h"
#include "hud.h"
#include "math/mathf.h"
#include "physics/collision_box.h"
#include "physics/collision_scene.h"
#include "scene.h"
#include "scene/dynamic_scene.h"
#include "signals.h"
#include "system/time.h"
#include "util/dynamic_asset_loader.h"

#include "codegen/assets/materials/static.h"
#include "codegen/assets/models/dynamic_animated_model_list.h"
#include "codegen/assets/models/props/door_01.h"
#include "codegen/assets/models/props/door_02.h"

#define DOOR_COLLISION_Y_OFFSET 1.0f
#define DOOR_COLLISION_LAYERS (COLLISION_LAYERS_TANGIBLE | COLLISION_LAYERS_STATIC | COLLISION_LAYERS_BLOCK_BALL | COLLISION_LAYERS_BLOCK_TURRET_SIGHT)

struct CollisionBox gDoorCollisionBox = {
    {1.0f, 1.0f, 0.1125f}
};

struct ColliderTypeData gDoorCollider = {
    CollisionShapeTypeBox,
    &gDoorCollisionBox,
    0.0f,
    0.5f,
    &gCollisionBoxCallbacks,  
};

struct DoorTypeDefinition gDoorTypeDefinitions[] = {
    [DoorType01] = {
        PROPS_DOOR_01_DYNAMIC_ANIMATED_MODEL,
        PROPS_DOOR_01_ARMATURE_OPEN_CLIP_INDEX,
        PROPS_DOOR_01_ARMATURE_CLOSE_CLIP_INDEX,
        PROPS_DOOR_01_ARMATURE_OPENED_CLIP_INDEX,
        DOOR_01_INDEX,
        -1,
        {0.0f, 0.0f, 0.0f, 1.0f},
    },
    [DoorType02] = {
        PROPS_DOOR_02_DYNAMIC_ANIMATED_MODEL,
        PROPS_DOOR_02_ARMATURE_OPEN_CLIP_INDEX,
        PROPS_DOOR_02_ARMATURE_CLOSE_CLIP_INDEX,
        PROPS_DOOR_02_ARMATURE_OPENED_CLIP_INDEX,
        DOOR_02_INDEX,
        PROPS_DOOR_02_DOOR_BONE,
        {0.707106781f, 0.0f, 0.0f, 0.707106781f},
    },
};

void doorRender(void* data, struct DynamicRenderDataList* renderList, struct RenderState* renderState) {
    struct Door* door = (struct Door*)data;
    Mtx* matrix = renderStateRequestMatrices(renderState, 1);
    struct DoorTypeDefinition* typeDefinition = &gDoorTypeDefinitions[door->doorDefinition->doorType];

    if (!matrix) {
        return;
    }

    struct Transform originalTransform;
    originalTransform.position = door->doorDefinition->location;
    originalTransform.rotation = door->doorDefinition->rotation;
    originalTransform.scale = gOneVec;

    transformToMatrixL(&originalTransform, matrix, SCENE_SCALE);

    Mtx* armature = renderStateRequestMatrices(renderState, door->armature.numberOfBones);

    if (!armature) {
        return;
    }

    skCalculateTransforms(&door->armature, armature);

    dynamicRenderListAddData(renderList, door->armature.displayList, matrix, typeDefinition->materialIndex, &door->rigidBody.transform.position, armature);
}

void doorInit(struct Door* door, struct DoorDefinition* doorDefinition, struct World* world) {
    collisionObjectInit(&door->collisionObject, &gDoorCollider, &door->rigidBody, 1.0f, DOOR_COLLISION_LAYERS);
    rigidBodyMarkKinematic(&door->rigidBody);
    collisionSceneAddDynamicObject(&door->collisionObject);

    struct DoorTypeDefinition* typeDefinition = &gDoorTypeDefinitions[doorDefinition->doorType];
    struct SKArmatureWithAnimations* armature = dynamicAssetAnimatedModel(typeDefinition->armatureIndex);

    skArmatureInit(&door->armature, armature->armature);
    skAnimatorInit(&door->animator, armature->armature->numberOfBones);

    quatMultiply(&doorDefinition->rotation, &typeDefinition->relativeRotation, &door->rigidBody.transform.rotation);
    door->rigidBody.transform.scale = gOneVec;

    struct Vector3 collisionOffset = { 0.0f, DOOR_COLLISION_Y_OFFSET, 0.0f };
    quatMultVector(&door->rigidBody.transform.rotation, &collisionOffset, &collisionOffset);
    vector3Add(&doorDefinition->location, &collisionOffset, &door->rigidBody.transform.position);

    collisionObjectUpdateBB(&door->collisionObject);

    door->dynamicId = dynamicSceneAdd(door, doorRender, &door->rigidBody.transform.position, 1.7f);
    door->signalIndex = doorDefinition->signalIndex;

    if (doorDefinition->doorwayIndex >= 0 && doorDefinition->doorwayIndex < world->doorwayCount) {
        door->forDoorway = &world->doorways[doorDefinition->doorwayIndex];
        door->forDoorway->flags &= ~DoorwayFlagsOpen;
        dynamicSceneSetRoomFlags(door->dynamicId, ROOM_FLAG_FROM_INDEX(door->forDoorway->roomA) | ROOM_FLAG_FROM_INDEX(door->forDoorway->roomB));
    } else {
        door->forDoorway = NULL;
    }
    door->flags = 0;

    door->doorDefinition = doorDefinition;

}

void doorUpdate(struct Door* door) {
    struct DoorTypeDefinition* typeDefinition = &gDoorTypeDefinitions[door->doorDefinition->doorType];

    int signal = signalsRead(door->signalIndex);
    skAnimatorUpdate(&door->animator, door->armature.pose, FIXED_DELTA_TIME);

    int isOpen = (door->flags & DoorFlagsIsOpen) != 0;

    if (isOpen != signal) {
        short clipIndex;
        if (signal) {
            clipIndex = typeDefinition->openClipIndex;
            door->flags |= DoorFlagsIsOpen;
        } else {
            clipIndex = typeDefinition->closeClipIndex;
            door->flags &= ~DoorFlagsIsOpen;
        }

        skAnimatorRunClip(&door->animator, dynamicAssetClip(typeDefinition->armatureIndex, clipIndex), 0.0f, 0);

        soundPlayerPlay(soundsDoor, 3.0f, 0.5f, &door->rigidBody.transform.position, &gZeroVec, SoundTypeAll);
        hudShowSubtitle(&gScene.hud, PORTAL_DOORCLOSE, SubtitleTypeCaption);
    }

    int isDoorwayOpen = skAnimatorIsRunning(&door->animator) || isOpen;

    if (door->forDoorway) {
        if (isDoorwayOpen) {
            door->forDoorway->flags |= DoorwayFlagsOpen;
        } else {
            door->forDoorway->flags &= ~DoorwayFlagsOpen;
        }
    }

    if (typeDefinition->colliderBoneIndex == -1) {
        door->collisionObject.collisionLayers = isDoorwayOpen ? 0 : DOOR_COLLISION_LAYERS;
    } else {
        struct Vector3 finalPos;
        skCalculateBonePosition(&door->armature, typeDefinition->colliderBoneIndex, &gZeroVec, &finalPos);
        finalPos.x = 0.0f;
        finalPos.y = DOOR_COLLISION_Y_OFFSET + (finalPos.z * (1.0f / SCENE_SCALE));
        finalPos.z = 0.0f;

        door->rigidBody.transform.position = door->doorDefinition->location;
        quatMultVector(&door->rigidBody.transform.rotation, &finalPos, &finalPos);
        vector3Add(&door->rigidBody.transform.position, &finalPos, &door->rigidBody.transform.position);
    }
}

void doorCheckForOpenState(struct Door* door) {
    struct DoorTypeDefinition* typeDefinition = &gDoorTypeDefinitions[door->doorDefinition->doorType];

    int signal = signalsRead(door->signalIndex);
    if (signal) {
        door->flags |= DoorFlagsIsOpen;
        skAnimatorRunClip(&door->animator, dynamicAssetClip(typeDefinition->armatureIndex, typeDefinition->openedClipIndex), 0.0f, 0);
        skAnimatorUpdate(&door->animator, door->armature.pose, FIXED_DELTA_TIME);
    }
}