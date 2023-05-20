
#include "skelatool_armature.h"
#include "skelatool_defs.h"
#include "util/memory.h"
#include "util/rom.h"

void skArmatureInit(struct SKArmature* object, struct SKArmatureDefinition* definition) {
    object->displayList = definition->displayList;
    object->numberOfBones = definition->numberOfBones;
    object->numberOfAttachments = definition->numberOfAttachments;

    unsigned transformSize = sizeof(Mtx) * definition->numberOfBones;

    object->pose = malloc(transformSize);
    if (definition->pose) {
        if (IS_KSEG0(definition->pose)) {
            memCopy(object->pose, definition->pose, transformSize);
        } else {
            romCopy((void*)definition->pose, (void*)object->pose, transformSize);
        }
    }
    object->boneParentIndex = definition->boneParentIndex;
}

void skCleanupObject(struct SKArmature* object) {
    free(object->pose);
    object->pose = 0;
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
        transformToMatrixL(&object->pose[i], &into[i], 1.0f);
    }
}

void skCalculateBonePosition(struct SKArmature* object, unsigned short boneIndex, struct Vector3* bonePosition, struct Vector3* out) {
    if (!object->boneParentIndex) {
        return;
    }
    *out = *bonePosition;

    while (boneIndex < object->numberOfBones) {
        transformPoint(&object->pose[boneIndex], out, out);
        boneIndex = object->boneParentIndex[boneIndex];
    }
}

void skCalculateBoneRotation(struct SKArmature* object, unsigned short boneIndex, struct Quaternion* out) {
    if (!object->boneParentIndex) {
        return;
    }

    quatIdent(out);

    while (boneIndex < object->numberOfBones) {
        struct Quaternion next;
        quatMultiply(&object->pose[boneIndex].rotation, out, &next);
        *out = next;
        boneIndex = object->boneParentIndex[boneIndex];
    }
}