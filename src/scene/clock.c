#include "clock.h"

#include "defs.h"
#include "levels/cutscene_runner.h"
#include "levels/levels.h"
#include "math/mathf.h"
#include "scene/dynamic_scene.h"
#include "system/time.h"
#include "util/dynamic_asset_loader.h"
#include "util/memory.h"

#include "codegen/assets/materials/static.h"
#include "codegen/assets/models/dynamic_model_list.h"
#include "codegen/assets/models/signage/clock_digits.h"

#define DIGIT_WIDTH     512

u8 gCurrentClockDigits[7];
Vtx* gClockDigits[] = {
    signage_clock_digits_clock_digits_hour_01_color,
    signage_clock_digits_clock_digits_minute_10_color,
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

    Vtx* digitPointer = dynamicAssetFixPointer(SIGNAGE_CLOCK_DIGITS_DYNAMIC_MODEL, gClockDigits[digitIndex]);

    for (int i = 0; i < 4; ++i) {
        digitPointer[i].v.tc[0] += (currDigit - prevDigit) * DIGIT_WIDTH;
    }
    gCurrentClockDigits[digitIndex] = (u8)currDigit;
    osWritebackDCache(digitPointer, sizeof(Vtx) * 4);
}

void clockSetTime(float timeInSeconds) {
    float minutes = floorf(timeInSeconds * (1.0f / 60.0f));
    clockSetDigit(0, (int)minutes);
    timeInSeconds -= minutes *= 60.0f;

    float tenSeconds = floorf(timeInSeconds * 0.1f);
    clockSetDigit(1, (int)tenSeconds);
    timeInSeconds -= tenSeconds * 10.0f;

    float seconds = floorf(timeInSeconds);
    clockSetDigit(2, (int)seconds);
    timeInSeconds -= seconds;

    float tenthsOfSecond = floorf(timeInSeconds * 10.0f);
    clockSetDigit(3, (int)tenthsOfSecond);
    timeInSeconds -= tenthsOfSecond * 0.1f;

    float hundredthsOfSecond = floorf(timeInSeconds * 100.f);
    clockSetDigit(4, (int)hundredthsOfSecond);
    timeInSeconds -= hundredthsOfSecond * 0.01f;

    float thousandthsOfSecond = floorf(timeInSeconds * 1000.f);
    clockSetDigit(5, (int)thousandthsOfSecond);
    timeInSeconds -= thousandthsOfSecond * 0.001f;

    float tenThousandthsOfSecond = floorf(timeInSeconds * 10000.f);
    clockSetDigit(6, (int)tenThousandthsOfSecond);
}

void clockRenderRender(void* data, struct DynamicRenderDataList* renderList, struct RenderState* renderState) {
    struct Clock* clock = (struct Clock*)data;

    Mtx* matrix = renderStateRequestMatrices(renderState, 1);

    if (!matrix) {
        return;
    }

    // main menu clock time
    if (clock->timeLeft == -1.0f) {
        clockSetDigit(0, 1);
        clockSetDigit(1, 5);
        clockSetDigit(2, 9);
        clockSetDigit(3, 5);
        clockSetDigit(4, 0);
        clockSetDigit(5, 0);
        clockSetDigit(6, 0);
    } else {
        clockSetTime(clock->timeLeft);    
    }

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
        dynamicAssetModel(SIGNAGE_CLOCK_DIGITS_DYNAMIC_MODEL),
        matrix,
        CLOCK_DIGITS_INDEX,
        &clock->transform.position,
        NULL
    );
}

void clockInit(struct Clock* clock, struct ClockDefinition* definition) {
    dynamicAssetModelPreload(SIGNAGE_CLOCK_DYNAMIC_MODEL);
    dynamicAssetModelPreload(SIGNAGE_CLOCK_DIGITS_DYNAMIC_MODEL);

    clock->transform.position = definition->position;
    clock->transform.rotation = definition->rotation;
    clock->transform.scale = gOneVec;
    clock->roomIndex = definition->roomIndex;
    clock->timeLeft = definition->duration;

    int dynamicId = dynamicSceneAdd(clock, clockRenderRender, &clock->transform.position, 0.8f);
    dynamicSceneSetRoomFlags(dynamicId, ROOM_FLAG_FROM_INDEX(clock->roomIndex));
    
    zeroMemory(gCurrentClockDigits, sizeof(gCurrentClockDigits));
}

void clockUpdate(struct Clock* clock) {
    if (clock->timeLeft > 0.0f) {
        clock->timeLeft -= FIXED_DELTA_TIME;

        if (clock->timeLeft < 0.0f) {
            clock->timeLeft = 0.0f;
        }
    }
}