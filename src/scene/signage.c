#include "signage.h"

#include "../scene/dynamic_scene.h"
#include "../levels/levels.h"
#include "../defs.h"
#include "../graphics/color.h"

#include "../build/assets/models/props/signage.h"
#include "../build/assets/models/props/signage_off.h"
#include "../build/assets/models/props/cylinder_test.h"
#include "../../build/assets/materials/static.h"

                                                            
#include <stdlib.h>   

int gCurrentSignageIndex = -1;

void signageSetLargeDigit(Vtx* vertices, int nextDigit, int currentDigit) {
    int uOffset = (nextDigit - currentDigit) * (14 << 5);

    for (int i = 0; i < 4; ++i) {
        ((Vtx*)K0_TO_K1(vertices))[i].v.tc[0] += uOffset;
    }
}

#define U_COORD_FOR_DIGIT(digit) (((digit & 0x1) ? (18 << 5) : (4 << 5)) + ((digit) >= 8 ? (2 << 5) : 0))

void signageSetSmallDigit(Vtx* vertices, int nextDigit, int currentDigit) {
    // 14 x 18 + 6 + 0 (+4 + 0 for 8 and 9)

    int uOffset = U_COORD_FOR_DIGIT(nextDigit) - U_COORD_FOR_DIGIT(currentDigit);
    int vOffset = ((nextDigit >> 1) - (currentDigit >> 1)) * (18 << 5);

    for (int i = 0; i < 4; ++i) {
        ((Vtx*)K0_TO_K1(vertices))[i].v.tc[0] += uOffset;
        ((Vtx*)K0_TO_K1(vertices))[i].v.tc[1] += vOffset;
    }
}

Vtx* gWarningVertices[] = {
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

short gLevelWarnings[] = {
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
};

static struct Coloru8 gSignageOnColor = {0, 0, 0, 255};
static struct Coloru8 gSignageOffColor = {212, 212, 212, 255};

void signageSetWarnings(int warningMask) {
    for (int i = 0; i < 10; ++i) {
        struct Coloru8 useColor = ((1 << i) & warningMask) ? gSignageOnColor : gSignageOffColor;

        for (int vIndex = 0; vIndex < 4; ++vIndex) {
            ((Vtx*)K0_TO_K1(gWarningVertices[i]))[vIndex].v.cn[0] = useColor.r;
            ((Vtx*)K0_TO_K1(gWarningVertices[i]))[vIndex].v.cn[1] = useColor.g;
            ((Vtx*)K0_TO_K1(gWarningVertices[i]))[vIndex].v.cn[2] = useColor.b;
            ((Vtx*)K0_TO_K1(gWarningVertices[i]))[vIndex].v.cn[3] = useColor.a;

        }
        
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
        signageSetSmallDigit(props_signage_signage_num00_sdigit_0_color, oneDigit, prevOneDigit);
        signageSetSmallDigit(props_signage_signage_num00_sdigit_10_color, tenDigit, prevTenDigit);

        signageSetWarnings(gLevelWarnings[neededIndex]);
}

void signageRender(void* data, struct DynamicRenderDataList* renderList, struct RenderState* renderState) {
    struct Signage* signage = (struct Signage*)data;

    float n = ((float)rand()/RAND_MAX)*(float)(1.0); 
    int signOn = 1;
    if (n <= signage->flickerChance){signOn = 0;}
    if (signage->flickerChance > 0.0001){signage->flickerChance = signage->flickerChance*0.97;}
    if (signOn){signageCheckIndex(signage->testChamberNumber);}

    Mtx* matrix = renderStateRequestMatrices(renderState, 1);

    if (!matrix) {
        return;
    }

    transformToMatrixL(&signage->transform, matrix, SCENE_SCALE);

    
    if (signOn){
        dynamicRenderListAddData(
        renderList,
        props_signage_model_gfx,
        matrix,
        DEFAULT_INDEX,
        &signage->transform.position,
        NULL
        );
    }
    else{
        dynamicRenderListAddData(
        renderList,
        props_signage_off_model_gfx,
        matrix,
        DEFAULT_INDEX,
        &signage->transform.position,
        NULL
        );
    }



}

void signageInit(struct Signage* signage, struct SignageDefinition* definition) {
    signage->transform.position = definition->position;
    signage->transform.rotation = definition->rotation;
    signage->transform.scale = gOneVec;
    signage->roomIndex = definition->roomIndex;
    signage->testChamberNumber = definition->testChamberNumber;
    signage->flickerChance = 1.0;

    int dynamicId = dynamicSceneAdd(signage, signageRender, &signage->transform.position, 1.7f);

    dynamicSceneSetRoomFlags(dynamicId, ROOM_FLAG_FROM_INDEX(definition->roomIndex));
}