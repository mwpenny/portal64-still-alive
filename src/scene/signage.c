#include "signage.h"

#include "audio/soundplayer.h"
#include "audio/clips.h"
#include "defs.h"
#include "graphics/color.h"
#include "levels/levels.h"
#include "scene/dynamic_scene.h"
#include "scene/scene.h"
#include "system/time.h"

#include "codegen/assets/materials/static.h"
#include "codegen/assets/models/props/cylinder_test.h"
#include "codegen/assets/models/props/signage.h"

static struct Coloru8 gBacklightColors[] = {
    {46, 47, 49, 49},
    {132, 135, 140, 145},
    {242, 245, 247, 255},
};

static struct Coloru8 gLCDBlackColors[] = {
    {46, 47, 49, 49},
    {132, 135, 140, 145},
    {0, 0, 0, 255},
    {0, 0, 0, 255},  // This color signals to turn on the progress bar
};

static struct Coloru8 gSymbolOffColors[] = {
    {46, 47, 49, 49},
    {132, 135, 140, 145},
    {40, 40, 40, 255},
    {212, 212, 212, 255},
};

static struct Coloru8 gSymbolOnColors[] = {
    {46, 47, 49, 49},
    {132, 135, 140, 145},
    {212, 212, 212, 255},
    {0, 0, 0, 255},
};

struct SignStateFrame {
    u8 backlightColor : 2;
    u8 lcdColor       : 2;
    u8 symbolOffColor : 2;
    u8 symbolOnColor  : 2;
};

#define PROGRESS_ENABLE_LCD_COLOR_INDEX 3

static struct SignStateFrame gSignageFrames[] = {
    // off
    {.backlightColor = 0, .lcdColor = 0, .symbolOffColor = 0, .symbolOnColor = 0},
    // backlight half on
    {.backlightColor = 1, .lcdColor = 1, .symbolOffColor = 1, .symbolOnColor = 1},
    {.backlightColor = 1, .lcdColor = 1, .symbolOffColor = 1, .symbolOnColor = 1},
    // lcd on
    {.backlightColor = 1, .lcdColor = 2, .symbolOffColor = 1, .symbolOnColor = 1},
    {.backlightColor = 1, .lcdColor = 2, .symbolOffColor = 1, .symbolOnColor = 1},
    {.backlightColor = 1, .lcdColor = 2, .symbolOffColor = 1, .symbolOnColor = 1},
    {.backlightColor = 1, .lcdColor = 2, .symbolOffColor = 1, .symbolOnColor = 1},
    {.backlightColor = 1, .lcdColor = 2, .symbolOffColor = 1, .symbolOnColor = 1},
    {.backlightColor = 1, .lcdColor = 2, .symbolOffColor = 1, .symbolOnColor = 1},
    // backlight full on
    {.backlightColor = 2, .lcdColor = 2, .symbolOffColor = 3, .symbolOnColor = 2},
    {.backlightColor = 2, .lcdColor = 2, .symbolOffColor = 3, .symbolOnColor = 2},
    {.backlightColor = 2, .lcdColor = 2, .symbolOffColor = 3, .symbolOnColor = 2},
    {.backlightColor = 2, .lcdColor = 2, .symbolOffColor = 3, .symbolOnColor = 2},
    {.backlightColor = 2, .lcdColor = 2, .symbolOffColor = 3, .symbolOnColor = 2},
    {.backlightColor = 2, .lcdColor = 2, .symbolOffColor = 3, .symbolOnColor = 2},
    // backlight flicker
    {.backlightColor = 1, .lcdColor = 2, .symbolOffColor = 1, .symbolOnColor = 1},
    {.backlightColor = 1, .lcdColor = 2, .symbolOffColor = 1, .symbolOnColor = 1},
    {.backlightColor = 1, .lcdColor = 2, .symbolOffColor = 1, .symbolOnColor = 1},
    // backlight full back on
    {.backlightColor = 2, .lcdColor = 2, .symbolOffColor = 3, .symbolOnColor = 2},
    {.backlightColor = 2, .lcdColor = 2, .symbolOffColor = 3, .symbolOnColor = 2},
    {.backlightColor = 2, .lcdColor = 2, .symbolOffColor = 3, .symbolOnColor = 2},
    {.backlightColor = 2, .lcdColor = 2, .symbolOffColor = 3, .symbolOnColor = 2},
    {.backlightColor = 2, .lcdColor = 2, .symbolOffColor = 3, .symbolOnColor = 2},
    // warning on
    {.backlightColor = 2, .lcdColor = 2, .symbolOffColor = 3, .symbolOnColor = 3},
    {.backlightColor = 2, .lcdColor = 2, .symbolOffColor = 3, .symbolOnColor = 3},
    {.backlightColor = 2, .lcdColor = 2, .symbolOffColor = 3, .symbolOnColor = 3},
    {.backlightColor = 2, .lcdColor = 2, .symbolOffColor = 3, .symbolOnColor = 3},
    {.backlightColor = 2, .lcdColor = 2, .symbolOffColor = 3, .symbolOnColor = 3},
    {.backlightColor = 2, .lcdColor = 2, .symbolOffColor = 3, .symbolOnColor = 3},
    {.backlightColor = 2, .lcdColor = 2, .symbolOffColor = 3, .symbolOnColor = 3},
    {.backlightColor = 2, .lcdColor = 2, .symbolOffColor = 3, .symbolOnColor = 3},
    {.backlightColor = 2, .lcdColor = 2, .symbolOffColor = 3, .symbolOnColor = 3},
    {.backlightColor = 2, .lcdColor = 2, .symbolOffColor = 3, .symbolOnColor = 3},
    {.backlightColor = 2, .lcdColor = 2, .symbolOffColor = 3, .symbolOnColor = 3},
    {.backlightColor = 2, .lcdColor = 2, .symbolOffColor = 3, .symbolOnColor = 3},
    {.backlightColor = 2, .lcdColor = 2, .symbolOffColor = 3, .symbolOnColor = 3},
    {.backlightColor = 2, .lcdColor = 2, .symbolOffColor = 3, .symbolOnColor = 3},
    {.backlightColor = 2, .lcdColor = 2, .symbolOffColor = 3, .symbolOnColor = 3},
    {.backlightColor = 2, .lcdColor = 2, .symbolOffColor = 3, .symbolOnColor = 3},
    // progress on
    {.backlightColor = 2, .lcdColor = PROGRESS_ENABLE_LCD_COLOR_INDEX, .symbolOffColor = 3, .symbolOnColor = 3},
};

#define SIGNAGE_HUM_VOLUME            0.6f
#define SIGNAGE_HUM_MENU_MULTIPLIER   1.7f
#define SIGNAGE_HUM_FADE_START_TIME   10.0f
#define SIGNAGE_HUM_FADE_TIME         40.0f
#define SIGNAGE_FRAME_COUNT           (sizeof(gSignageFrames) / sizeof(*gSignageFrames))

static float gHumFadeElapTime = 0.0f;
static float gHumFadeVolume = 0.0f;
static ALSndId gHumSoundLoopId = SOUND_ID_NONE;

static short gCurrentSignageIndex = -1;
static struct SignStateFrame gCurrentSignageFrame = {3, 3, 3, 3};

static void signageSetLargeDigit(Vtx* vertices, int nextDigit, int currentDigit) {
    int uOffset = (nextDigit - currentDigit) * (14 << 5);

    for (int i = 0; i < 4; ++i) {
        ((Vtx*)K0_TO_K1(vertices))[i].v.tc[0] += uOffset;
    }
}

#define U_COORD_FOR_DIGIT(digit) ((digit & 0x1) ? (18 << 5) : (4 << 5))

static void signageSetSmallDigit(Vtx* vertices, int nextDigit, int currentDigit) {
    // 14 x 18 + 4 + 0

    int uOffset = U_COORD_FOR_DIGIT(nextDigit) - U_COORD_FOR_DIGIT(currentDigit);
    int vOffset = ((nextDigit >> 1) - (currentDigit >> 1)) * (18 << 5);

    for (int i = 0; i < 4; ++i) {
        ((Vtx*)K0_TO_K1(vertices))[i].v.tc[0] += uOffset;
        ((Vtx*)K0_TO_K1(vertices))[i].v.tc[1] += vOffset;
    }
}

#define PROGRESS_MAX_INDEX  19

#define PROGRESS_X_START    55
#define PROGRESS_X_LENGTH   142

#define PROGRESS_U_START    0
#define PROGRESS_U_LENGTH   2400

static void signageSetProgress(int index, struct SignStateFrame signState) {
    float progressAmount = (signState.lcdColor == PROGRESS_ENABLE_LCD_COLOR_INDEX) ?
        (float)index / (float)PROGRESS_MAX_INDEX :
        0;

    // Model X coordinates are decreasing and so we must subtract
    short xCoord = PROGRESS_X_START - (short)(progressAmount * (float)PROGRESS_X_LENGTH);
    short uCoord = PROGRESS_U_START + (short)(progressAmount * (float)PROGRESS_U_LENGTH);

    Vtx* vertices = (Vtx*)K0_TO_K1(props_signage_signage_num00_progress_color);
    vertices[0].v.ob[0] = xCoord;
    vertices[0].v.tc[0] = uCoord;

    vertices[1].v.ob[0] = xCoord;
    vertices[1].v.tc[0] = uCoord;
}

static Vtx* gDenominatorVertices[] = {
    props_signage_signage_num00_sdigit_denom_slash_color,
    props_signage_signage_num00_sdigit_denom_0_color,
    props_signage_signage_num00_sdigit_denom_10_color,
};

static void signageSetVertexColor(Vtx* vertices, struct Coloru8* color) {
    for (int vIndex = 0; vIndex < 4; ++vIndex) {
        ((Vtx*)K0_TO_K1(vertices))[vIndex].v.cn[0] = color->r;
        ((Vtx*)K0_TO_K1(vertices))[vIndex].v.cn[1] = color->g;
        ((Vtx*)K0_TO_K1(vertices))[vIndex].v.cn[2] = color->b;
        ((Vtx*)K0_TO_K1(vertices))[vIndex].v.cn[3] = color->a;
    }
}

static void signageSetDenominator(struct SignStateFrame signState) {
    struct Coloru8 useColor = gSymbolOnColors[signState.symbolOnColor];

    for (int i = 0; i < 3; ++i) {
        signageSetVertexColor(gDenominatorVertices[i], &useColor);
    }
}

static Vtx* gWarningVertices[] = {
    props_signage_signage_num00_warn_0_color,
    props_signage_signage_num00_warn_1_color,
    props_signage_signage_num00_warn_2_color,
    props_signage_signage_num00_warn_3_color,
    props_signage_signage_num00_warn_4_color,
    props_signage_signage_num00_warn_5_color,
    props_signage_signage_num00_warn_6_color,
    props_signage_signage_num00_warn_7_color,
    props_signage_signage_num00_warn_8_color,
    props_signage_signage_num00_warn_9_color,
};

enum LevelWarnings {
    LevelWarningsCubeDispense = (1 << 0),
    LevelWarningsCubeHit = (1 << 1),
    LevelWarningsBallHit = (1 << 2),
    LevelWarningsBallCollect = (1 << 3),
    LevelWarningsLiquid = (1 << 4),
    LevelWarningsSpeedyIn = (1 << 5),
    LevelWarningsSpeedyOut = (1 << 6),
    LevelWarningsTurret = (1 << 7),
    LevelWarningsDrinking = (1 << 8),
    LevelWarningsCake = (1 << 9),
};

static short gLevelWarnings[] = {
    LevelWarningsCubeDispense | LevelWarningsCubeHit,
    0,
    0,
    0,
    LevelWarningsCubeDispense | LevelWarningsCubeHit,
    LevelWarningsCubeHit,
    LevelWarningsBallHit | LevelWarningsBallCollect,
    LevelWarningsBallHit | LevelWarningsBallCollect,
    LevelWarningsBallHit | LevelWarningsBallCollect | LevelWarningsLiquid | LevelWarningsDrinking,
    LevelWarningsCubeDispense | LevelWarningsCubeHit,
    LevelWarningsSpeedyIn | LevelWarningsSpeedyOut,
    LevelWarningsBallHit | LevelWarningsBallCollect | LevelWarningsLiquid | LevelWarningsDrinking,
    LevelWarningsCubeDispense | LevelWarningsCubeHit | LevelWarningsSpeedyIn | LevelWarningsSpeedyOut,
    LevelWarningsCubeHit | LevelWarningsBallHit | LevelWarningsBallCollect,
    LevelWarningsCubeHit | LevelWarningsBallHit | LevelWarningsBallCollect | LevelWarningsLiquid | LevelWarningsDrinking | LevelWarningsSpeedyIn | LevelWarningsSpeedyOut,
    LevelWarningsBallHit | LevelWarningsBallCollect | LevelWarningsLiquid | LevelWarningsSpeedyIn | LevelWarningsSpeedyOut | LevelWarningsDrinking,
    LevelWarningsCubeDispense | LevelWarningsCubeHit | LevelWarningsTurret,
};

static void signageSetWarnings(int warningMask, struct SignStateFrame signState) {
    for (int i = 0; i < 10; ++i) {
        struct Coloru8 useColor = (1 << i) & warningMask ?
            gSymbolOnColors[signState.symbolOnColor] :
            gSymbolOffColors[signState.symbolOffColor];

        signageSetVertexColor(gWarningVertices[i], &useColor);
    }
}

static void signageCheckIndex(int neededIndex, struct SignStateFrame signState) {
    if (gCurrentSignageIndex == neededIndex && *((u8*)&gCurrentSignageFrame) == *((u8*)&signState)) {
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
    gCurrentSignageFrame = signState;
    signageSetLargeDigit(props_signage_signage_num00_digit_0_color, oneDigit, prevOneDigit);
    signageSetLargeDigit(props_signage_signage_num00_digit_10_color, tenDigit, prevTenDigit);
    signageSetSmallDigit(props_signage_signage_num00_sdigit_0_color, oneDigit, prevOneDigit);
    signageSetSmallDigit(props_signage_signage_num00_sdigit_10_color, tenDigit, prevTenDigit);

    signageSetProgress(neededIndex, signState);
    signageSetDenominator(signState);
    signageSetWarnings(gLevelWarnings[neededIndex], signState);
}

static void signageRender(void* data, struct DynamicRenderDataList* renderList, struct RenderState* renderState) {
    struct Signage* signage = (struct Signage*)data;

    int frameIndex = signage->currentFrame;

    if (frameIndex == -1) {
        frameIndex = 0;
    }

    struct SignStateFrame frame = gSignageFrames[frameIndex];

    signageCheckIndex(signage->testChamberNumber, frame);

    Mtx* matrix = renderStateRequestMatrices(renderState, 1);

    if (!matrix) {
        return;
    }

    Gfx* model = renderStateAllocateDLChunk(renderState, 4);
    Gfx* dl = model;

    struct Coloru8 backlightColor = gBacklightColors[frame.backlightColor];
    struct Coloru8 lcdColor = gLCDBlackColors[frame.lcdColor];

    gDPSetPrimColor(dl++, 255, 255, backlightColor.r, backlightColor.g, backlightColor.b, backlightColor.a);
    gDPSetEnvColor(dl++, lcdColor.r, lcdColor.g, lcdColor.b, lcdColor.a);
    gSPDisplayList(dl++, props_signage_model_gfx);
    gSPEndDisplayList(dl++);

    transformToMatrixL(&signage->transform, matrix, SCENE_SCALE);

    dynamicRenderListAddData(
        renderList,
        model,
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
    signage->currentFrame = -1;

    // Stop current sound if one if playing.
    if (gHumSoundLoopId != SOUND_ID_NONE) {
        soundPlayerStop(gHumSoundLoopId);
        gHumSoundLoopId = SOUND_ID_NONE;
    }

    gHumFadeElapTime = 0.0f;
    gHumFadeVolume = 0.0f;

    int dynamicId = dynamicSceneAdd(signage, signageRender, &signage->transform.position, 1.7f);

    dynamicSceneSetRoomFlags(dynamicId, ROOM_FLAG_FROM_INDEX(definition->roomIndex));
}


void signageUpdate(struct Signage* signage) {

    if (signage->currentFrame >= 0 && signage->currentFrame + 1 < SIGNAGE_FRAME_COUNT) {
        ++signage->currentFrame;

        if (gHumSoundLoopId != SOUND_ID_NONE) {

            // Flicker the hum sound on and off with the backlight
            struct SignStateFrame frame = gSignageFrames[signage->currentFrame];
            float humVolume = SIGNAGE_HUM_VOLUME * (frame.backlightColor == 2);

            if (gScene.mainMenuMode) {
                humVolume *= SIGNAGE_HUM_MENU_MULTIPLIER;

                if (gHumFadeVolume != humVolume) {
                    gHumFadeVolume = humVolume;
                }
            }

            soundPlayerAdjustVolume(gHumSoundLoopId, humVolume);
        }
    }

    // If we are at the main menu gradually fade out the sign hum sound after several seconds
    if (gScene.mainMenuMode && gHumSoundLoopId != SOUND_ID_NONE) {
        gHumFadeElapTime += FIXED_DELTA_TIME;

        if (gHumFadeElapTime > SIGNAGE_HUM_FADE_START_TIME) {
            gHumFadeVolume = mathfMoveTowards(gHumFadeVolume, 0.0f, FIXED_DELTA_TIME / SIGNAGE_HUM_FADE_TIME);
            soundPlayerAdjustVolume(gHumSoundLoopId, gHumFadeVolume);

            // Stop the sound and fade logic once it is inaudible
            if (gHumFadeVolume < 0.2f) {
                soundPlayerStop(gHumSoundLoopId);
                gHumSoundLoopId = SOUND_ID_NONE;
            }
        }
    }
}

void signageActivate(struct Signage* signage) {
    if (signage->currentFrame == -1) {
        signage->currentFrame = 0;

        gHumSoundLoopId = soundPlayerPlay(soundsSignageHum, SIGNAGE_HUM_VOLUME, 1.0f, &signage->transform.position, &gZeroVec, SoundTypeAll);
    }
}
