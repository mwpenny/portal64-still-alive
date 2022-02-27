
#include "skelatool_armature.h"
#include "skelatool_defs.h"
#include "util/memory.h"
#include "util/rom.h"

void skArmatureInit(struct SKArmature* object, Gfx* displayList, u32 numberOfBones, struct Transform* initialPose, unsigned short* boneParentIndex) {
    object->displayList = displayList;
    object->numberOfBones = numberOfBones;
    object->boneTransforms = malloc(sizeof(Mtx) * numberOfBones);
    if (initialPose) {
        romCopy((void*)initialPose, (void*)object->boneTransforms, sizeof(Mtx) * numberOfBones);
    }
    object->boneParentIndex = boneParentIndex;
}

void skCleanupObject(struct SKArmature* object) {
    free(object->boneTransforms);
    object->boneTransforms = 0;
    object->numberOfBones = 0;
}

void skRenderObject(struct SKArmature* object, struct RenderState* intoState) {
    if (!object->displayList) {
        return;
    }

    Mtx* boneMatrices = renderStateRequestMatrices(intoState, object->numberOfBones);

    if (boneMatrices) {
        skCalculateTransforms(object, boneMatrices);
        gSPSegment(intoState->dl++, MATRIX_TRANSFORM_SEGMENT,  osVirtualToPhysical(boneMatrices));
        gSPDisplayList(intoState->dl++, object->displayList);
    }
}

void skCalculateTransforms(struct SKArmature* object, Mtx* into) {
    for (int i = 0; i < object->numberOfBones; ++i) {
        transformToMatrixL(&object->boneTransforms[i], &into[i]);
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