#include "door.h"

#include "../graphics/render_scene.h"
#include "defs.h"
#include "../scene/dynamic_scene.h"
#include "signals.h"
#include "../math/mathf.h"
#include "../util/time.h"
#include "../physics/collision_box.h"
#include "../physics/collision_scene.h"
#include "../util/dynamic_asset_loader.h"

#include "../build/assets/materials/static.h"
#include "../../build/assets/models/dynamic_animated_model_list.h"

#include "../build/assets/models/props/door_01.h"
#include "../build/assets/models/props/door_02.h"

#define OPEN_VELOCITY   8.0f

#define OPEN_WIDTH  0.625

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
        1.0f,
        {0.0f, 0.0f, 0.0f, 1.0f},
    },
    [DoorType02] = {
        PROPS_DOOR_02_DYNAMIC_ANIMATED_MODEL,
        PROPS_DOOR_02_ARMATURE_OPEN_CLIP_INDEX,
        PROPS_DOOR_02_ARMATURE_CLOSE_CLIP_INDEX,
        PROPS_DOOR_02_ARMATURE_OPENED_CLIP_INDEX,
        DOOR_02_INDEX,
        PROPS_DOOR_02_DOOR_BONE,
        3.0f,
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
    collisionObjectInit(&door->collisionObject, &gDoorCollider, &door->rigidBody, 1.0f, COLLISION_LAYERS_TANGIBLE|COLLISION_LAYERS_STATIC);
    rigidBodyMarkKinematic(&door->rigidBody);
    collisionSceneAddDynamicObject(&door->collisionObject);

    struct DoorTypeDefinition* typeDefinition = &gDoorTypeDefinitions[doorDefinition->doorType];
    struct SKArmatureWithAnimations* armature = dynamicAssetAnimatedModel(typeDefinition->armatureIndex);

    skArmatureInit(&door->armature, armature->armature);
    skAnimatorInit(&door->animator, armature->armature->numberOfBones);

    door->rigidBody.transform.position = doorDefinition->location;
    door->rigidBody.transform.position.y += 1.0f;
    quatMultiply(&doorDefinition->rotation, &typeDefinition->relativeRotation, &door->rigidBody.transform.rotation);
    door->rigidBody.transform.scale = gOneVec;

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
        if (signal) {
            skAnimatorRunClip(&door->animator, dynamicAssetClip(typeDefinition->armatureIndex, typeDefinition->openClipIndex), 0.0f, 0);
        } else {
            skAnimatorRunClip(&door->animator, dynamicAssetClip(typeDefinition->armatureIndex, typeDefinition->closeClipIndex), 0.0f, 0);
        }

        soundPlayerPlay(soundsDoor, 3.0f, 0.5f, &door->rigidBody.transform.position, &gZeroVec);

        if (signal) {
            door->flags |= DoorFlagsIsOpen;
        } else {
            door->flags &= ~DoorFlagsIsOpen;
        }
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
        door->collisionObject.collisionLayers = isDoorwayOpen ? 0 : (COLLISION_LAYERS_TANGIBLE | COLLISION_LAYERS_STATIC);
    } else {
        struct Vector3 finalPos;
        skCalculateBonePosition(&door->armature, typeDefinition->colliderBoneIndex, &gZeroVec, &finalPos);

        door->rigidBody.transform.position.y = 
            door->doorDefinition->location.y + 
            1.0f +
            finalPos.z * (1.0f / SCENE_SCALE);
    }
}

void doorCheckForOpenState(struct Door* door) {
    struct DoorTypeDefinition* typeDefinition = &gDoorTypeDefinitions[door->doorDefinition->doorType];

    int signal = signalsRead(door->signalIndex);
    if (signal) {
        skAnimatorRunClip(&door->animator, dynamicAssetClip(typeDefinition->armatureIndex, typeDefinition->openedClipIndex), 0.0f, 0);
    }
}