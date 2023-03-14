#include "box_dropper.h"

#include "../scene/dynamic_scene.h"
#include "../models/models.h"
#include "../defs.h"
#include "../physics/config.h"
#include "../util/time.h"
#include "signals.h"
#include "../decor/decor_object_list.h"

#include "../../build/assets/materials/static.h"
#include "../../build/assets/models/cube/cube.h"
#include "../../build/assets/models/props/box_dropper.h"

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
        props_box_dropper_model_gfx,
        matrix,
        box_dropper_material_index,
        &dropper->transform.position,
        armature
    );

    dynamicRenderListAddData(
        renderList, 
        box_dropper_glass_gfx, 
        matrix, 
        box_dropper_glass_material_index, 
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
            cube_cube_model_gfx, 
            pendingBoxMatrix, 
            CUBE_INDEX, 
            &dropper->transform.position, 
            NULL
        );  
    }
}

void boxDropperInit(struct BoxDropper* dropper, struct BoxDropperDefinition* definition) {
    dropper->dynamicId = dynamicSceneAdd(dropper, boxDropperRender, &dropper->transform.position, 1.5f);

    dropper->transform.position = definition->position;
    quatIdent(&dropper->transform.rotation);
    dropper->transform.scale = gOneVec;

    dropper->roomIndex = definition->roomIndex;
    dropper->signalIndex = definition->signalIndex;

    skArmatureInit(&dropper->armature, &props_box_dropper_armature);

    skAnimatorInit(&dropper->animator, PROPS_BOX_DROPPER_DEFAULT_BONES_COUNT);

    dropper->flags = 0;
    dropper->reloadTimer = DROOPER_RELOAD_TIME;

    dynamicSceneSetRoomFlags(dropper->dynamicId, ROOM_FLAG_FROM_INDEX(dropper->roomIndex));
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
            decorObjectClenaup(&dropper->activeCube);
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
        skAnimatorRunClip(&dropper->animator, &props_box_dropper_Armature_DropCube_clip, 0.0f, 0);
        soundPlayerPlay(soundsReleaseCube, 5.0f, 0.5f, &dropper->activeCube.rigidBody.transform.position, &gZeroVec);

        dropper->flags &= ~BoxDropperFlagsCubeRequested;
        dropper->flags |= BoxDropperFlagsCubeIsActive;

        dropper->reloadTimer = DROOPER_RELOAD_TIME;
    }

    dropper->flags &= ~BoxDropperFlagsSignalWasSet;
    if (signalIsSet) {
        dropper->flags |= BoxDropperFlagsSignalWasSet;
    }

}