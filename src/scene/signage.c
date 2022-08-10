#include "signage.h"

#include "../scene/dynamic_scene.h"
#include "../levels/levels.h"
#include "../defs.h"

#include "../build/assets/models/props/signage.h"
#include "../build/assets/models/props/cylinder_test.h"
#include "../../build/assets/materials/static.h"

int gCurrentSignageIndex = -1;

void signageSetLargeDigit(Vtx* vertices, int nextDigit, int currentDigit) {
    int uOffset = (nextDigit - currentDigit) * (14 << 5);

    for (int i = 0; i < 4; ++i) {
        ((Vtx*)K0_TO_K1(vertices))[i].v.tc[0] += uOffset;
    }
}

void signageCheckIndex(int neededIndex) {
    if (gCurrentSignageIndex == neededIndex) {
        return;
    }

    if (gCurrentSignageIndex == -1) {
        gCurrentSignageIndex = 0;
    }

    int prevTenDigit = gCurrentSignageIndex / 10;
    int prevOneDigit = gCurrentSignageIndex - prevTenDigit * 10; 

    int tenDigit = neededIndex / 10;
    int oneDigit = neededIndex - tenDigit * 10;
    
    gCurrentSignageIndex = neededIndex;

    signageSetLargeDigit(props_signage_signage_num00_digit_0_color, oneDigit, prevOneDigit);
    signageSetLargeDigit(props_signage_signage_num00_digit_10_color, tenDigit, prevTenDigit);
}

void signageRender(void* data, struct RenderScene* renderScene) {
    struct Signage* signage = (struct Signage*)data;

    if (!RENDER_SCENE_IS_ROOM_VISIBLE(renderScene, signage->roomIndex)) {
        return;
    }

    signageCheckIndex(signage->testChamberNumber);

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
    signage->testChamberNumber = definition->testChamberNumber;

    dynamicSceneAdd(signage, signageRender, &signage->transform, 1.7f);
}