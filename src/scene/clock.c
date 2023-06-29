#include "clock.h"

#include "../scene/dynamic_scene.h"
#include "../defs.h"

#include "../build/assets/models/signage/clock.h"
#include "../build/assets/models/signage/clock_digits.h"

#include "../../build/assets/materials/static.h"

void clockRenderRender(void* data, struct DynamicRenderDataList* renderList, struct RenderState* renderState) {
    struct Clock* clock = (struct Clock*)data;

    Mtx* matrix = renderStateRequestMatrices(renderState, 1);

    if (!matrix) {
        return;
    }

    transformToMatrixL(&clock->transform, matrix, SCENE_SCALE);

    dynamicRenderListAddData(
        renderList,
        signage_clock_model_gfx,
        matrix,
        AUTOPORTAL_FRAME_INDEX,
        &clock->transform.position,
        NULL
    );

    dynamicRenderListAddData(
        renderList,
        signage_clock_model_gfx,
        matrix,
        CLOCK_DIGITS_INDEX,
        &clock->transform.position,
        NULL
    );
}

void clockInit(struct Clock* clock, struct ClockDefinition* definition) {
    clock->transform.position = definition->position;
    clock->transform.rotation = definition->rotation;
    clock->transform.scale = gOneVec;
    clock->roomIndex = definition->roomIndex;
    clock->cutsceneIndex = definition->cutsceneIndex;

    int dynamicId = dynamicSceneAdd(clock, clockRenderRender, &clock->transform.position, 0.8f);
    dynamicSceneSetRoomFlags(dynamicId, ROOM_FLAG_FROM_INDEX(clock->roomIndex));
}

void clockUpdate(struct Clock* clock) {
    
}