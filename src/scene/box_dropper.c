#include "box_dropper.h"

#include "../scene/dynamic_scene.h"
#include "../defs.h"
#include "../physics/config.h"
#include "../util/time.h"
#include "signals.h"
#include "../decor/decor_object_list.h"
#include "../util/dynamic_asset_loader.h"

#include "../../build/assets/materials/static.h"
#include "../../build/assets/models/props/box_dropper.h"
#include "../../build/assets/models/dynamic_model_list.h"
#include "../../build/assets/models/dynamic_animated_model_list.h"
#include "hud.h"
#include "scene.h"

#define DROOPER_RELOAD_TIME     2.0f
#define DROPPER_DROP_TIME       0.5f

void boxDropperFakePos(struct BoxDropper* dropper, struct Transform* result) {
    *result = dropper->transform;

    result->position.y += 0.4f;

    if (dropper->reloadTimer) {
        float timeSinceDrop = (DROPPER_DROP_TIME - dropper->reloadTimer);
        result->position.y -= GRAVITY_CONSTANT * DROPPER_DROP_TIME * DROPPER_DROP_TIME;
        result->position.y += GRAVITY_CONSTANT * timeSinceDrop * timeSinceDrop;
    }
}


void boxDropperRender(void* data, struct DynamicRenderDataList* renderList, struct RenderState* renderState) {
    struct BoxDropper* dropper = (struct BoxDropper*)data;

    Mtx* matrix = renderStateRequestMatrices(renderState, 1);

    if (!matrix) {
        return;
    }

    transformToMatrixL(&dropper->transform, matrix, SCENE_SCALE);

    Mtx* armature = renderStateRequestMatrices(renderState, PROPS_BOX_DROPPER_DEFAULT_BONES_COUNT);

    if (!armature) {
        return;
    }

    skCalculateTransforms(&dropper->armature, armature);

    dynamicRenderListAddData(
        renderList,
        dropper->armature.displayList,
        matrix,
        DEFAULT_INDEX,
        &dropper->transform.position,
        armature
    );

    dynamicRenderListAddData(
        renderList, 
        dynamicAssetModel(PROPS_BOX_DROPPER_GLASS_DYNAMIC_MODEL), 
        matrix, 
        GLASSWINDOW_FROSTED_002_INDEX, 
        &dropper->transform.position, 
        NULL
    );

    if (dropper->reloadTimer < DROPPER_DROP_TIME) {
        struct Transform pendingCubePos;
        boxDropperFakePos(dropper, &pendingCubePos);

        Mtx* pendingBoxMatrix = renderStateRequestMatrices(renderState, 1);

        if (!pendingBoxMatrix) {
            return;
        }

        transformToMatrixL(&pendingCubePos, pendingBoxMatrix, SCENE_SCALE);

        dynamicRenderListAddData(
            renderList, 
            dynamicAssetModel(CUBE_CUBE_DYNAMIC_MODEL), 
            pendingBoxMatrix, 
            CUBE_INDEX, 
            &dropper->transform.position, 
            NULL
        );  
    }
}

void boxDropperInit(struct BoxDropper* dropper, struct BoxDropperDefinition* definition) {
    struct SKArmatureWithAnimations* armature = dynamicAssetAnimatedModel(PROPS_BOX_DROPPER_DYNAMIC_ANIMATED_MODEL);

    dropper->dynamicId = dynamicSceneAdd(dropper, boxDropperRender, &dropper->transform.position, 1.5f);

    dropper->transform.position = definition->position;
    quatIdent(&dropper->transform.rotation);
    dropper->transform.scale = gOneVec;

    dropper->roomIndex = definition->roomIndex;
    dropper->signalIndex = definition->signalIndex;

    skArmatureInit(&dropper->armature, armature->armature);

    skAnimatorInit(&dropper->animator, PROPS_BOX_DROPPER_DEFAULT_BONES_COUNT);

    dropper->flags = 0;
    dropper->reloadTimer = DROOPER_RELOAD_TIME;

    dynamicSceneSetRoomFlags(dropper->dynamicId, ROOM_FLAG_FROM_INDEX(dropper->roomIndex));

    dynamicAssetModelPreload(CUBE_CUBE_DYNAMIC_MODEL);
    dynamicAssetModelPreload(PROPS_BOX_DROPPER_GLASS_DYNAMIC_MODEL);
}

void boxDropperUpdate(struct BoxDropper* dropper) {
    skAnimatorUpdate(&dropper->animator, dropper->armature.pose, FIXED_DELTA_TIME);

    if (dropper->reloadTimer > 0.0f) {
        dropper->reloadTimer -= FIXED_DELTA_TIME;
        if (dropper->reloadTimer < 0.0f) {
            dropper->reloadTimer = 0.0f;
        }
    }

    if (dropper->flags & BoxDropperFlagsCubeIsActive) {
        if (!decorObjectUpdate(&dropper->activeCube)) {
            decorObjectCleanup(&dropper->activeCube);
            dropper->flags &= ~BoxDropperFlagsCubeIsActive;
        }
    }

    int signalIsSet = signalsRead(dropper->signalIndex);

    if (signalIsSet && !(dropper->flags & BoxDropperFlagsSignalWasSet)) {
        if (dropper->flags & BoxDropperFlagsCubeIsActive) {
            dropper->activeCube.rigidBody.flags |= RigidBodyFizzled;
        }
        dropper->flags |= BoxDropperFlagsCubeRequested;
    }

    if (signalIsSet && !(dropper->flags & BoxDropperFlagsCubeIsActive)) {
        dropper->flags |= BoxDropperFlagsCubeRequested;
    }

    if (((dropper->flags & (BoxDropperFlagsCubeIsActive | BoxDropperFlagsCubeRequested)) == BoxDropperFlagsCubeRequested)) {
        struct Transform pendingCubePos;
        boxDropperFakePos(dropper, &pendingCubePos);

        decorObjectInit(&dropper->activeCube, decorObjectDefinitionForId(DECOR_TYPE_CUBE_UNIMPORTANT), &pendingCubePos, dropper->roomIndex);
        skAnimatorRunClip(&dropper->animator, dynamicAssetClip(PROPS_BOX_DROPPER_DYNAMIC_ANIMATED_MODEL, PROPS_BOX_DROPPER_ARMATURE_DROPCUBE_CLIP_INDEX), 0.0f, 0);
        soundPlayerPlay(soundsReleaseCube, 5.0f, 0.5f, &dropper->activeCube.rigidBody.transform.position, &gZeroVec, SoundTypeAll);
        hudShowSubtitle(&gScene.hud, ESCAPE_CAKE_RIDE_1, SubtitleTypeCaption);

        dropper->flags &= ~BoxDropperFlagsCubeRequested;
        dropper->flags |= BoxDropperFlagsCubeIsActive;

        dropper->reloadTimer = DROOPER_RELOAD_TIME;
    }

    dropper->flags &= ~BoxDropperFlagsSignalWasSet;
    if (signalIsSet) {
        dropper->flags |= BoxDropperFlagsSignalWasSet;
    }

}