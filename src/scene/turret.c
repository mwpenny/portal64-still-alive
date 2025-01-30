#include "turret.h"

#include "defs.h"
#include "scene/dynamic_scene.h"
#include "util/dynamic_asset_loader.h"

#include "codegen/assets/materials/static.h"
#include "codegen/assets/models/dynamic_animated_model_list.h"

#define TURRET_ORIGIN_Y_OFFSET 0.554f

static struct Vector3 laserOffset = { 0.0f, 0.0425f, 0.0f };

static void turretRender(void* data, struct DynamicRenderDataList* renderList, struct RenderState* renderState) {
    struct Turret* turret = (struct Turret*)data;

    Mtx* matrix = renderStateRequestMatrices(renderState, 1);

    if (!matrix) {
        return;
    }

    // Render without origin offset
    struct Transform originalTransform;
    originalTransform.position = turret->definition->position;
    originalTransform.rotation = turret->definition->rotation;
    originalTransform.scale = gOneVec;
    transformToMatrixL(&originalTransform, matrix, SCENE_SCALE);

    Mtx* armature = renderStateRequestMatrices(renderState, turret->armature.numberOfBones);

    if (!armature) {
        return;
    }

    skCalculateTransforms(&turret->armature, armature);

    dynamicRenderListAddDataTouchingPortal(
        renderList,
        turret->armature.displayList,
        matrix,
        TURRET_INDEX,
        &turret->rigidBody.transform.position,
        armature,
        turret->rigidBody.flags
    );
}

void turretInit(struct Turret* turret, struct TurretDefinition* definition) {
    // TODO:
    // * Physics (collidable and grabbable)
    // * Fizzleable

    turret->definition = definition;

    turret->rigidBody.transform.position = definition->position;
    turret->rigidBody.transform.rotation = definition->rotation;
    turret->rigidBody.transform.scale = gOneVec;
    turret->rigidBody.currentRoom = definition->roomIndex;

    struct Vector3 collisionOffset = { 0.0f, TURRET_ORIGIN_Y_OFFSET, 0.0f };
    quatMultVector(&turret->rigidBody.transform.rotation, &collisionOffset, &collisionOffset);
    vector3Add(&definition->position, &collisionOffset, &turret->rigidBody.transform.position);

    struct SKArmatureWithAnimations* armature = dynamicAssetAnimatedModel(PROPS_TURRET_01_DYNAMIC_ANIMATED_MODEL);
    skArmatureInit(&turret->armature, armature->armature);

    laserInit(&turret->laser, &turret->rigidBody.transform, &laserOffset);

    turret->dynamicId = dynamicSceneAdd(turret, turretRender, &turret->rigidBody.transform.position, 0.75f);
    dynamicSceneSetRoomFlags(turret->dynamicId, ROOM_FLAG_FROM_INDEX(turret->rigidBody.currentRoom));
}

void turretUpdate(struct Turret* turret) {
    laserUpdate(&turret->laser, turret->rigidBody.currentRoom);
}
