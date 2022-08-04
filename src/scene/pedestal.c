#include "pedestal.h"

#include "../scene/dynamic_scene.h"
#include "../defs.h"

#include "../build/assets/materials/static.h"
#include "../build/assets/models/pedestal.h"

void pedestalRender(void* data, struct RenderScene* renderScene) {
    struct Pedestal* pedestal = (struct Pedestal*)data;

    Mtx* matrix = renderStateRequestMatrices(renderScene->renderState, 1);
    transformToMatrixL(&pedestal->transform, matrix, SCENE_SCALE);

    Mtx* armature = renderStateRequestMatrices(renderScene->renderState, pedestal->armature.numberOfBones);

    skCalculateTransforms(&pedestal->armature, armature);

    renderSceneAdd(
        renderScene,
        pedestal->armature.displayList,
        matrix,
        DEFAULT_INDEX,
        &pedestal->transform.position,
        armature
    );
}

void pedestalInit(struct Pedestal* pedestal, struct PedestalDefinition* definition) {
    transformInitIdentity(&pedestal->transform);

    pedestal->transform.position = definition->position;
    pedestal->roomIndex = definition->roomIndex;

    skArmatureInit(
        &pedestal->armature,
        pedestal_model_gfx,
        PEDESTAL_DEFAULT_BONES_COUNT,
        pedestal_default_bones,
        pedestal_bone_parent
    );

    pedestal->dynamicId = dynamicSceneAdd(pedestal, pedestalRender, &pedestal->transform, 0.8f);
}