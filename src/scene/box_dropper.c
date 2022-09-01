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


void boxDropperRender(void* data, struct RenderScene* renderScene) {
    struct BoxDropper* dropper = (struct BoxDropper*)data;

    if (!RENDER_SCENE_IS_ROOM_VISIBLE(renderScene, dropper->roomIndex)) {
        return;
    }

    Mtx* matrix = renderStateRequestMatrices(renderScene->renderState, 1);
    transformToMatrixL(&dropper->transform, matrix, SCENE_SCALE);

    Mtx* armature = renderStateRequestMatrices(renderScene->renderState, PROPS_BOX_DROPPER_DEFAULT_BONES_COUNT);
    skCalculateTransforms(&dropper->armature, armature);

    renderSceneAdd(
        renderScene,
        props_box_dropper_model_gfx,
        matrix,
        box_dropper_material_index,
        &dropper->transform.position,
        armature
    );

    renderSceneAdd(
        renderScene, 
        box_dropper_glass_gfx, 
        matrix, 
        box_dropper_glass_material_index, 
        &dropper->transform.position, 
        NULL
    );

    if (dropper->reloadTimer < DROPPER_DROP_TIME) {
        struct Transform pendingCubePos;
        boxDropperFakePos(dropper, &pendingCubePos);

        Mtx* pendingBoxMatrix = renderStateRequestMatrices(renderScene->renderState, 1);
        transformToMatrixL(&pendingCubePos, pendingBoxMatrix, SCENE_SCALE);

        renderSceneAdd(
            renderScene, 
            cube_cube_model_gfx, 
            pendingBoxMatrix, 
            CUBE_INDEX, 
            &dropper->transform.position, 
            NULL
        );  
    }
}

void boxDropperInit(struct BoxDropper* dropper, struct BoxDropperDefinition* definition) {
    dropper->dynamicId = dynamicSceneAdd(dropper, boxDropperRender, &dropper->transform, 1.5f);

    dropper->transform.position = definition->position;
    quatIdent(&dropper->transform.rotation);
    dropper->transform.scale = gOneVec;

    dropper->roomIndex = definition->roomIndex;
    dropper->signalIndex = definition->signalIndex;

    skArmatureInit(
        &dropper->armature, 
        props_box_dropper_model_gfx, 
        PROPS_BOX_DROPPER_DEFAULT_BONES_COUNT, 
        props_box_dropper_default_bones, 
        props_box_dropper_bone_parent, 
        PROPS_BOX_DROPPER_ATTACHMENT_COUNT
    );

    skAnimatorInit(&dropper->animator, PROPS_BOX_DROPPER_DEFAULT_BONES_COUNT, NULL, NULL);

    dropper->flags = 0;
    dropper->reloadTimer = DROOPER_RELOAD_TIME;
}

void boxDropperUpdate(struct BoxDropper* dropper) {
    skAnimatorUpdate(&dropper->animator, dropper->armature.boneTransforms, 1.0f);

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

        decorObjectInit(&dropper->activeCube, decorObjectDefinitionForId(DECOR_TYPE_CUBE), &pendingCubePos, dropper->roomIndex);
        skAnimatorRunClip(&dropper->animator, &props_box_dropper_animations[PROPS_BOX_DROPPER_PROPS_BOX_DROPPER_ARMATURE_DROPCUBE_INDEX], 0);

        dropper->flags &= ~BoxDropperFlagsCubeRequested;
        dropper->flags |= BoxDropperFlagsCubeIsActive;

        dropper->reloadTimer = DROOPER_RELOAD_TIME;
    }

    dropper->flags &= ~BoxDropperFlagsSignalWasSet;
    if (signalIsSet) {
        dropper->flags |= BoxDropperFlagsSignalWasSet;
    }

}