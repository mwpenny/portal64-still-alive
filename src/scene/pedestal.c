#include "pedestal.h"

#include "defs.h"
#include "scene/dynamic_scene.h"
#include "scene/hud.h"
#include "scene/scene.h"
#include "system/time.h"
#include "util/dynamic_asset_loader.h"

#include "codegen/assets/materials/static.h"
#include "codegen/assets/models/dynamic_animated_model_list.h"
#include "codegen/assets/models/pedestal.h"
#include "codegen/assets/models/portal_gun/w_portalgun.h"

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

    Gfx* gunAttachment = portal_gun_w_portalgun_model_gfx;
    Gfx* attachments = skBuildAttachments(&pedestal->armature, (pedestal->flags & PedestalFlagsDown) ? NULL : &gunAttachment, renderState);

    Gfx* objectRender = renderStateAllocateDLChunk(renderState, 4);
    Gfx* dl = objectRender;

    if (attachments) {
        gSPSegment(dl++, BONE_ATTACHMENT_SEGMENT,  osVirtualToPhysical(attachments));
    }
    gSPSegment(dl++, MATRIX_TRANSFORM_SEGMENT,  osVirtualToPhysical(armature));
    gSPDisplayList(dl++, pedestal->armature.displayList);
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
    struct SKArmatureWithAnimations* armature = dynamicAssetAnimatedModel(PEDESTAL_DYNAMIC_ANIMATED_MODEL);

    transformInitIdentity(&pedestal->transform);

    pedestal->transform.position = definition->position;
    pedestal->roomIndex = definition->roomIndex;

    skArmatureInit(&pedestal->armature, armature->armature);

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
                soundPlayerPlay(soundsPedestalShooting, 5.0f, 0.5f, &pedestal->transform.position, &gZeroVec, SoundTypeAll);
            }
            pedestal->flags &= ~PedestalFlagsIsPointing;
        }
        else{
            if (!(pedestal->flags & PedestalFlagsAlreadyMoving) && !(pedestal->flags & PedestalFlagsDown)){
                soundPlayerPlay(soundsPedestalMoving, 5.0f, 0.5f, &pedestal->transform.position, &gZeroVec, SoundTypeAll);
                hudShowSubtitle(&gScene.hud, PORTALGUN_PEDESTAL_ROTATE, SubtitleTypeCaption);
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
    soundPlayerPlay(soundsReleaseCube, 3.0f, 0.5f, &pedestal->transform.position, &gZeroVec, SoundTypeAll);
    hudShowSubtitle(&gScene.hud, WEAPON_PORTALGUN_POWERUP, SubtitleTypeCaption);
    pedestal->flags |= PedestalFlagsDown;
    skAnimatorRunClip(&pedestal->animator, dynamicAssetClip(PEDESTAL_DYNAMIC_ANIMATED_MODEL, PEDESTAL_ARMATURE_HIDE_CLIP_INDEX), 0.0f, 0);
}

void pedestalPointAt(struct Pedestal* pedestal, struct Vector3* target) {
    pedestal->pointAt = *target;
    pedestal->flags |= PedestalFlagsIsPointing;
}

void pedestalSetDown(struct Pedestal* pedestal) {
    skAnimatorRunClip(&pedestal->animator, dynamicAssetClip(PEDESTAL_DYNAMIC_ANIMATED_MODEL, PEDESTAL_ARMATURE_HIDDEN_CLIP_INDEX), 0.0f, 0);
}