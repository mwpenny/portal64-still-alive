#include "turret.h"

#include "defs.h"
#include "scene/dynamic_scene.h"
#include "util/dynamic_asset_loader.h"

#include "codegen/assets/materials/static.h"
#include "codegen/assets/models/dynamic_animated_model_list.h"

void turretRender(void* data, struct DynamicRenderDataList* renderList, struct RenderState* renderState) {
    struct Turret* turret = (struct Turret*)data;

    Mtx* matrix = renderStateRequestMatrices(renderState, 1);

    if (!matrix) {
        return;
    }

    transformToMatrixL(&turret->rigidBody.transform, matrix, SCENE_SCALE);

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

    struct SKArmatureWithAnimations* armature = dynamicAssetAnimatedModel(PROPS_TURRET_01_DYNAMIC_ANIMATED_MODEL);
    skArmatureInit(&turret->armature, armature->armature);

    turret->rigidBody.transform.position = definition->position;
    turret->rigidBody.transform.rotation = definition->rotation;
    turret->rigidBody.transform.scale = gOneVec;
    turret->rigidBody.currentRoom = definition->roomIndex;

    // TODO: radius
    turret->dynamicId = dynamicSceneAdd(turret, turretRender, &turret->rigidBody.transform.position, 1.5f);

    dynamicSceneSetRoomFlags(turret->dynamicId, ROOM_FLAG_FROM_INDEX(turret->rigidBody.currentRoom));
}

void turretUpdate(struct Turret* turret) {
    // TODO
}
