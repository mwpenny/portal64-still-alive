#include "signage.h"

#include "../scene/dynamic_scene.h"
#include "../levels/levels.h"
#include "../defs.h"

#include "../build/assets/models/props/signage.h"
#include "../build/assets/models/props/cylinder_test.h"
#include "../../build/assets/materials/static.h"

void signageRender(void* data, struct RenderScene* renderScene) {
    struct Signage* signage = (struct Signage*)data;
    Mtx* matrix = renderStateRequestMatrices(renderScene->renderState, 1);
    transformToMatrixL(&signage->transform, matrix, SCENE_SCALE);

    renderSceneAdd(
        renderScene,
        props_signage_model_gfx,
        matrix,
        DEFAULT_INDEX,
        &signage->transform.position,
        NULL
    );
}

void signageInit(struct Signage* signage, struct SignageDefinition* definition) {
    signage->transform.position = definition->position;
    signage->transform.rotation = definition->rotation;
    signage->transform.scale = gOneVec;
    signage->roomIndex = definition->roomIndex;

    dynamicSceneAdd(signage, signageRender, &signage->transform, 1.7f);
}