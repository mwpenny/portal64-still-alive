#include "pedestal.h"

#include "../scene/dynamic_scene.h"
#include "../defs.h"
#include "../models/models.h"
#include "../util/time.h"

#include "../build/assets/materials/static.h"
#include "../build/assets/models/pedestal.h"

struct Vector2 gMaxPedistalRotation;
#define MAX_PEDISTAL_ROTATION_DEGREES_PER_SEC   (M_PI / 3.0f)

void pedestalRender(void* data, struct DynamicRenderDataList* renderList, struct RenderState* renderState) {
    struct Pedestal* pedestal = (struct Pedestal*)data;

    Mtx* matrix = renderStateRequestMatrices(renderState, 1);

    if (!matrix) {
        return;
    }

    transformToMatrixL(&pedestal->transform, matrix, SCENE_SCALE);

    Mtx* armature = renderStateRequestMatrices(renderState, pedestal->armature.numberOfBones);

    if (!armature) {
        return;
    }

    skCalculateTransforms(&pedestal->armature, armature);

    Gfx* attachments = skBuildAttachments(&pedestal->armature, (pedestal->flags & PedestalFlagsDown) ? NULL : &w_portal_gun_gfx, renderState);

    Gfx* objectRender = renderStateAllocateDLChunk(renderState, 4);
    Gfx* dl = objectRender;

    if (attachments) {
        gSPSegment(dl++, BONE_ATTACHMENT_SEGMENT,  osVirtualToPhysical(attachments));
    }
    gSPSegment(dl++, MATRIX_TRANSFORM_SEGMENT,  osVirtualToPhysical(armature));
    gSPDisplayList(dl++, pedestal_model_gfx);
    gSPEndDisplayList(dl++);

    dynamicRenderListAddData(
        renderList,
        objectRender,
        matrix,
        DEFAULT_INDEX,
        &pedestal->transform.position,
        NULL
    );
}

void pedestalInit(struct Pedestal* pedestal, struct PedestalDefinition* definition) {
    transformInitIdentity(&pedestal->transform);

    pedestal->transform.position = definition->position;
    pedestal->roomIndex = definition->roomIndex;

    skArmatureInit(&pedestal->armature, &pedestal_armature);

    skAnimatorInit(&pedestal->animator, PEDESTAL_DEFAULT_BONES_COUNT);

    pedestal->dynamicId = dynamicSceneAdd(pedestal, pedestalRender, &pedestal->transform.position, 0.8f);

    dynamicSceneSetRoomFlags(pedestal->dynamicId, ROOM_FLAG_FROM_INDEX(definition->roomIndex));

    pedestal->flags = 0;

    gMaxPedistalRotation.x = cosf(MAX_PEDISTAL_ROTATION_DEGREES_PER_SEC * FIXED_DELTA_TIME);
    gMaxPedistalRotation.y = sinf(MAX_PEDISTAL_ROTATION_DEGREES_PER_SEC * FIXED_DELTA_TIME);

    pedestal->currentRotation.x = 1.0f;
    pedestal->currentRotation.y = 0.0f;
}

void pedestalDetermineHolderAngle(struct Pedestal* pedestal, struct Vector2* output) {
    output->x = pedestal->pointAt.z - pedestal->transform.position.z;
    output->y = pedestal->pointAt.x - pedestal->transform.position.x;
    vector2Normalize(output, output);
}

void pedestalUpdate(struct Pedestal* pedestal) {
    skAnimatorUpdate(&pedestal->animator, pedestal->armature.pose, FIXED_DELTA_TIME);

    if (pedestal->flags & PedestalFlagsIsPointing) {
        struct Vector2 target;
        pedestalDetermineHolderAngle(pedestal, &target);

        if (vector2RotateTowards(&pedestal->currentRotation, &target, &gMaxPedistalRotation, &pedestal->currentRotation)) {
            if (!(pedestal->flags & PedestalFlagsDown)){
                soundPlayerPlay(soundsPedestalShooting, 5.0f, 0.5f, &pedestal->transform.position, &gZeroVec);
            }
            pedestal->flags &= ~PedestalFlagsIsPointing;
        }
        else{
            if (!(pedestal->flags & PedestalFlagsAlreadyMoving) && !(pedestal->flags & PedestalFlagsDown)){
                soundPlayerPlay(soundsPedestalMoving, 5.0f, 0.5f, &pedestal->transform.position, &gZeroVec);
                pedestal->flags |= PedestalFlagsAlreadyMoving;
            }
        }
        
    }
    else{
        pedestal->flags &= ~PedestalFlagsAlreadyMoving;
    }
    
    quatAxisComplex(&gUp, &pedestal->currentRotation, &pedestal->armature.pose[PEDESTAL_HOLDER_BONE].rotation);
}

void pedestalHide(struct Pedestal* pedestal) {
    soundPlayerPlay(soundsReleaseCube, 3.0f, 0.5f, &pedestal->transform.position, &gZeroVec);
    pedestal->flags |= PedestalFlagsDown;
    skAnimatorRunClip(&pedestal->animator, &pedestal_Armature_Hide_clip, 0.0f, 0);
}

void pedestalPointAt(struct Pedestal* pedestal, struct Vector3* target) {
    pedestal->pointAt = *target;
    pedestal->flags |= PedestalFlagsIsPointing;
}

void pedestalSetDown(struct Pedestal* pedestal) {
    skAnimatorRunClip(&pedestal->animator, &pedestal_Armature_Hidden_clip, 0.0f, 0);
}