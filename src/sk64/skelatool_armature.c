
#include "skelatool_armature.h"
#include "skelatool_defs.h"
#include "util/memory.h"
#include "util/rom.h"

void skArmatureInit(struct SKArmature* object, Gfx* displayList, u32 numberOfBones, struct Transform* initialPose, unsigned short* boneParentIndex, u32 numberOfAttachments) {
    object->displayList = displayList;
    object->numberOfBones = numberOfBones;
    object->numberOfAttachments = numberOfAttachments;

    unsigned transformSize = sizeof(Mtx) * numberOfBones;

    object->boneTransforms = malloc(transformSize);
    if (initialPose) {
        if (IS_KSEG0(initialPose)) {
            memCopy(object->boneTransforms, initialPose, transformSize);
        } else {
            romCopy((void*)initialPose, (void*)object->boneTransforms, transformSize);
        }
    }
    object->boneParentIndex = boneParentIndex;
}

void skCleanupObject(struct SKArmature* object) {
    free(object->boneTransforms);
    object->boneTransforms = 0;
    object->numberOfBones = 0;
}

Gfx* skBuildAttachments(struct SKArmature* object, Gfx** attachments, struct RenderState* renderState) {
    if (object->numberOfAttachments == 0) {
        return NULL;
    }

    if (object->numberOfAttachments == 1 && attachments) {
        return *attachments;
    }

    Gfx* jumpTable = renderStateAllocateDLChunk(renderState, object->numberOfAttachments);
    Gfx* dl = jumpTable;

    for (unsigned i = 0; i < object->numberOfAttachments; ++i) {
        if (attachments && attachments[i]) {
            gSPBranchList(dl++, attachments[i]);
        } else {
            gSPEndDisplayList(dl++);
        }
    }

    return jumpTable;
}

void skRenderObject(struct SKArmature* object, Gfx** attachements, struct RenderState* intoState) {
    if (!object->displayList) {
        return;
    }

    Mtx* boneMatrices = renderStateRequestMatrices(intoState, object->numberOfBones);

    if (!boneMatrices) {
        return;
    }

    Gfx* jumpTable = skBuildAttachments(object, attachements, intoState);

    if (!jumpTable && object->numberOfAttachments) {
        return;
    }

    if (jumpTable) {
        gSPSegment(intoState->dl++, BONE_ATTACHMENT_SEGMENT,  osVirtualToPhysical(jumpTable));
    }

    skCalculateTransforms(object, boneMatrices);
    gSPSegment(intoState->dl++, MATRIX_TRANSFORM_SEGMENT,  osVirtualToPhysical(boneMatrices));
    gSPDisplayList(intoState->dl++, object->displayList);
}

void skCalculateTransforms(struct SKArmature* object, Mtx* into) {
    for (int i = 0; i < object->numberOfBones; ++i) {
        transformToMatrixL(&object->boneTransforms[i], &into[i], 1.0f);
    }
}

void skCalculateBonePosition(struct SKArmature* object, unsigned short boneIndex, struct Vector3* bonePosition, struct Vector3* out) {
    if (!object->boneParentIndex) {
        return;
    }
    *out = *bonePosition;

    while (boneIndex < object->numberOfBones) {
        transformPoint(&object->boneTransforms[boneIndex], out, out);
        boneIndex = object->boneParentIndex[boneIndex];
    }
}