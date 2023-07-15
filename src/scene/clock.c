#include "clock.h"

#include "../scene/dynamic_scene.h"
#include "../defs.h"

#include "../levels/levels.h"
#include "../levels/cutscene_runner.h"
#include "../build/assets/models/signage/clock_digits.h"

#include "../build/assets/models/dynamic_model_list.h"

#include "../../build/assets/materials/static.h"

#include "../util/dynamic_asset_loader.h"

#include <math.h>

#define DIGIT_WIDTH     512

u8 gCurrentClockDigits[5];
Vtx* gClockDigits[] = {
    signage_clock_digits_clock_digits_minute_01_color,
    signage_clock_digits_clock_digits_second_10_color,
    signage_clock_digits_clock_digits_second_01_color,
    signage_clock_digits_clock_digits_ms_10_color,
    signage_clock_digits_clock_digits_ms_01_color,
};

void clockSetDigit(int digitIndex, int currDigit) {
    int prevDigit = gCurrentClockDigits[digitIndex];

    if (prevDigit == currDigit) {
        return;
    }

    for (int i = 0; i < 4; ++i) {
        gClockDigits[digitIndex][i].v.tc[0] += (currDigit - prevDigit) * DIGIT_WIDTH;
    }
    gCurrentClockDigits[digitIndex] = (u8)currDigit;
    osWritebackDCache(gClockDigits[digitIndex], sizeof(Vtx) * 4);
}

void clockSetTime(float timeInSeconds) {
    float minutes = floor(timeInSeconds * (1.0f / 60.0f));

    clockSetDigit(0, (int)minutes);

    timeInSeconds -= minutes *= 60.0f;

    float tenSeconds = floor(timeInSeconds * 0.1f);

    clockSetDigit(1, (int)tenSeconds);

    timeInSeconds -= tenSeconds * 10.0f;

    float seconds = floor(timeInSeconds);

    clockSetDigit(2, (int)seconds);

    timeInSeconds -= seconds;

    float tenthsOfSecond = floor(timeInSeconds * 10.0f);

    clockSetDigit(3, (int)tenthsOfSecond);

    timeInSeconds -= tenthsOfSecond * 0.1f;

    clockSetDigit(4, (int)floor(timeInSeconds * 100.0f));
}

void clockRenderRender(void* data, struct DynamicRenderDataList* renderList, struct RenderState* renderState) {
    struct Clock* clock = (struct Clock*)data;

    Mtx* matrix = renderStateRequestMatrices(renderState, 1);

    if (!matrix) {
        return;
    }

    float time = 0.0f;

    if (clock->cutsceneIndex != -1) {
        time = cutsceneEstimateTimeLeft(&gCurrentLevel->cutscenes[clock->cutsceneIndex]);
    }

    clockSetTime(time);    

    transformToMatrixL(&clock->transform, matrix, SCENE_SCALE);

    dynamicRenderListAddData(
        renderList,
        dynamicAssetModel(SIGNAGE_CLOCK_DYNAMIC_MODEL),
        matrix,
        AUTOPORTAL_FRAME_INDEX,
        &clock->transform.position,
        NULL
    );

    dynamicRenderListAddData(
        renderList,
        signage_clock_digits_model_gfx,
        matrix,
        CLOCK_DIGITS_INDEX,
        &clock->transform.position,
        NULL
    );
}

void clockInit(struct Clock* clock, struct ClockDefinition* definition) {
    dynamicAssetPreload(SIGNAGE_CLOCK_DYNAMIC_MODEL);

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