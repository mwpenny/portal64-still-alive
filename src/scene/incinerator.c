#include "incinerator.h"

#include "scene/dynamic_scene.h"
#include "util/dynamic_asset_loader.h"
#include "util/frame_time.h"

#include "codegen/assets/materials/static.h"
#include "codegen/assets/models/dynamic_animated_model_list.h"
#include "codegen/assets/models/props_bts/glados_aperturedoor.h"

// Avoids z-fighting
#define INCINERATOR_SCALE 1024.0f

void incineratorRender(void* data, struct DynamicRenderDataList* renderList, struct RenderState* renderState) {
    struct Incinerator* incinerator = (struct Incinerator*)data;

    Mtx* matrix = renderStateRequestMatrices(renderState, 1);
    if (!matrix) {
        return;
    }

    transformToMatrixL(&incinerator->transform, matrix, SCENE_SCALE);

    Mtx* armature = renderStateRequestMatrices(renderState, incinerator->armature.numberOfBones);
    if (!armature) {
        return;
    }

    skCalculateTransforms(&incinerator->armature, armature);

    dynamicRenderListAddData(
        renderList,
        incinerator->armature.displayList,
        matrix,
        INCINERATOR_INDEX,
        &incinerator->transform.position,
        armature
    );
}

void incineratorInit(struct Incinerator* incinerator, struct IncineratorDefinition* definition) {
    struct SKArmatureWithAnimations* armature = dynamicAssetAnimatedModel(PROPS_BTS_GLADOS_APERTUREDOOR_DYNAMIC_ANIMATED_MODEL);
    skArmatureInit(&incinerator->armature, armature->armature);
    skAnimatorInit(&incinerator->animator, armature->armature->numberOfBones);

    incinerator->transform.position = definition->position;
    incinerator->transform.rotation = definition->rotation;
    vector3Scale(&gOneVec, &incinerator->transform.scale, SCENE_SCALE / INCINERATOR_SCALE);

    incinerator->signalIndex = definition->signalIndex;
    incinerator->isOpen = 0;

    incinerator->dynamicId = dynamicSceneAdd(
        incinerator,
        incineratorRender,
        &incinerator->transform.position,
        2.0f
    );
    dynamicSceneSetRoomFlags(incinerator->dynamicId, ROOM_FLAG_FROM_INDEX(definition->roomIndex));
}

void incineratorUpdate(struct Incinerator* incinerator) {
    int animationDirection = incinerator->isOpen ? 1 : -1;
    skAnimatorUpdate(
        &incinerator->animator,
        incinerator->armature.pose,
        FIXED_DELTA_TIME * animationDirection
    );

    if (signalsRead(incinerator->signalIndex) != incinerator->isOpen) {
        struct SKAnimationClip* clip = dynamicAssetClip(
            PROPS_BTS_GLADOS_APERTUREDOOR_DYNAMIC_ANIMATED_MODEL,
            PROPS_BTS_GLADOS_APERTUREDOOR_ARMATURE_OPEN_CLIP_INDEX
        );

        float startTime = SK_ANIMATION_CLIP_START(clip, incinerator->isOpen);
        skAnimatorEnsureClipRunning(&incinerator->animator, clip, startTime, 0);

        incinerator->isOpen ^= 1;
    }
}
