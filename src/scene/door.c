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

void doorRender(void* data, struct RenderScene* renderScene) {
    struct Door* door = (struct Door*)data;
    Mtx* matrix = renderStateRequestMatrices(renderScene->renderState, 1);
    struct Transform originalTransform;
    originalTransform.position = door->doorDefinition->location;
    originalTransform.rotation = door->doorDefinition->rotation;
    originalTransform.scale = gOneVec;

    transformToMatrixL(&originalTransform, matrix, SCENE_SCALE);

    props_door_01_default_bones[PROPS_DOOR_01_DOORL_BONE].position.x = door->openAmount * -0.625f * SCENE_SCALE;
    props_door_01_default_bones[PROPS_DOOR_01_DOORR_BONE].position.x = door->openAmount * 0.625f * SCENE_SCALE;

    Mtx* armature = renderStateRequestMatrices(renderScene->renderState, PROPS_DOOR_01_DEFAULT_BONES_COUNT);
    transformToMatrixL(&props_door_01_default_bones[PROPS_DOOR_01_FRAME_BONE], &armature[PROPS_DOOR_01_FRAME_BONE], 1.0f);
    transformToMatrixL(&props_door_01_default_bones[PROPS_DOOR_01_DOORL_BONE], &armature[PROPS_DOOR_01_DOORL_BONE], 1.0f);
    transformToMatrixL(&props_door_01_default_bones[PROPS_DOOR_01_DOORR_BONE], &armature[PROPS_DOOR_01_DOORR_BONE], 1.0f);

    renderSceneAdd(renderScene, door_01_gfx, matrix, door_01_material_index, &door->rigidBody.transform.position, armature);
}

void doorInit(struct Door* door, struct DoorDefinition* doorDefinition, struct World* world) {
    collisionObjectInit(&door->collisionObject, &gDoorCollider, &door->rigidBody, 1.0f, COLLISION_LAYERS_TANGIBLE);
    rigidBodyMarkKinematic(&door->rigidBody);
    collisionSceneAddDynamicObject(&door->collisionObject);

    door->rigidBody.transform.position = doorDefinition->location;
    door->rigidBody.transform.position.y += 1.0f;
    door->rigidBody.transform.rotation = doorDefinition->rotation;
    door->rigidBody.transform.scale = gOneVec;

    collisionObjectUpdateBB(&door->collisionObject);

    door->dynamicId = dynamicSceneAdd(door, doorRender, &door->rigidBody.transform, 1.7f);
    door->signalIndex = doorDefinition->signalIndex;
    door->openAmount = 0.0f;

    if (doorDefinition->doorwayIndex >= 0 && doorDefinition->doorwayIndex < world->doorwayCount) {
        door->forDoorway = &world->doorways[doorDefinition->doorwayIndex];
        door->forDoorway->flags &= ~DoorwayFlagsOpen;
    } else {
        door->forDoorway = NULL;
    }

    door->doorDefinition = doorDefinition;
}

void doorUpdate(struct Door* door) {
    float targetOpenAmount = signalsRead(door->signalIndex) ? 1.0f : 0.0f;
    door->openAmount = mathfMoveTowards(door->openAmount, targetOpenAmount, OPEN_VELOCITY * FIXED_DELTA_TIME);

    if (door->forDoorway) {
        if (door->openAmount == 0.0f) {
            door->forDoorway->flags &= ~DoorwayFlagsOpen;
            door->collisionObject.collisionLayers = COLLISION_LAYERS_TANGIBLE;
        } else {
            door->forDoorway->flags |= DoorwayFlagsOpen;
            door->collisionObject.collisionLayers = 0;
        }
    }
}