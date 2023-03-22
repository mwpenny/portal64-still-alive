#include "door.h"

#include "../graphics/render_scene.h"
#include "defs.h"
#include "../models/models.h"
#include "../scene/dynamic_scene.h"
#include "signals.h"
#include "../math/mathf.h"
#include "../util/time.h"
#include "../physics/collision_box.h"
#include "../physics/collision_scene.h"

#include "../build/assets/models/props/door_01.h"

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

void doorRender(void* data, struct DynamicRenderDataList* renderList, struct RenderState* renderState) {
    struct Door* door = (struct Door*)data;
    Mtx* matrix = renderStateRequestMatrices(renderState, 1);

    if (!matrix) {
        return;
    }

    struct Transform originalTransform;
    originalTransform.position = door->doorDefinition->location;
    originalTransform.rotation = door->doorDefinition->rotation;
    originalTransform.scale = gOneVec;

    transformToMatrixL(&originalTransform, matrix, SCENE_SCALE);

    props_door_01_default_bones[PROPS_DOOR_01_DOORL_BONE].position.x = door->openAmount * -0.625f * SCENE_SCALE;
    props_door_01_default_bones[PROPS_DOOR_01_DOORR_BONE].position.x = door->openAmount * 0.625f * SCENE_SCALE;

    Mtx* armature = renderStateRequestMatrices(renderState, PROPS_DOOR_01_DEFAULT_BONES_COUNT);

    if (!armature) {
        return;
    }

    transformToMatrixL(&props_door_01_default_bones[PROPS_DOOR_01_FRAME_BONE], &armature[PROPS_DOOR_01_FRAME_BONE], 1.0f);
    transformToMatrixL(&props_door_01_default_bones[PROPS_DOOR_01_DOORL_BONE], &armature[PROPS_DOOR_01_DOORL_BONE], 1.0f);
    transformToMatrixL(&props_door_01_default_bones[PROPS_DOOR_01_DOORR_BONE], &armature[PROPS_DOOR_01_DOORR_BONE], 1.0f);

    dynamicRenderListAddData(renderList, door_01_gfx, matrix, door_01_material_index, &door->rigidBody.transform.position, armature);
}

void doorInit(struct Door* door, struct DoorDefinition* doorDefinition, struct World* world) {
    collisionObjectInit(&door->collisionObject, &gDoorCollider, &door->rigidBody, 1.0f, COLLISION_LAYERS_TANGIBLE|COLLISION_LAYERS_STATIC);
    rigidBodyMarkKinematic(&door->rigidBody);
    collisionSceneAddDynamicObject(&door->collisionObject);

    door->rigidBody.transform.position = doorDefinition->location;
    door->rigidBody.transform.position.y += 1.0f;
    door->rigidBody.transform.rotation = doorDefinition->rotation;
    door->rigidBody.transform.scale = gOneVec;

    collisionObjectUpdateBB(&door->collisionObject);

    door->dynamicId = dynamicSceneAdd(door, doorRender, &door->rigidBody.transform.position, 1.7f);
    door->signalIndex = doorDefinition->signalIndex;
    door->openAmount = 0.0f;

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
    float targetOpenAmount = signalsRead(door->signalIndex) ? 1.0f : 0.0f;
    door->openAmount = mathfMoveTowards(door->openAmount, targetOpenAmount, OPEN_VELOCITY * FIXED_DELTA_TIME);
    

    if (door->forDoorway) {
        if (door->openAmount == 0.0f) {
            door->forDoorway->flags &= ~DoorwayFlagsOpen;
        } else {
            door->forDoorway->flags |= DoorwayFlagsOpen;
        }
    }

    if (door->openAmount == 0.0f) {
        door->collisionObject.collisionLayers = COLLISION_LAYERS_TANGIBLE|COLLISION_LAYERS_STATIC;
        door->flags &= ~DoorFlagsJustOpened;
        if (!(door->flags & DoorFlagsJustClosed)){
            soundPlayerPlay(soundsDoor, 3.0f, 0.5f, &door->rigidBody.transform.position, &gZeroVec);
            door->flags |= DoorFlagsJustClosed;
        }
    } else {
        door->flags &= ~DoorFlagsJustClosed;
        if (!(door->flags & DoorFlagsJustOpened)){
            soundPlayerPlay(soundsDoor, 3.0f, 0.5f, &door->rigidBody.transform.position, &gZeroVec);
            door->flags |= DoorFlagsJustOpened;
        }
        door->collisionObject.collisionLayers = 0;
    }
}