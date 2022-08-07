#ifndef __SKELATOOL_OBJECT_H
#define __SKELATOOL_OBJECT_H

#include <ultra64.h>
#include "math/transform.h"
#include "graphics/renderstate.h"

#define NO_BONE_PARENT      0xFFFF

struct SKArmature {
    Gfx* displayList;
    struct Transform* boneTransforms;
    u16 numberOfBones;
    u16 numberOfAttachments;
    unsigned short* boneParentIndex;
};

void skArmatureInit(struct SKArmature* object, Gfx* displayList, u32 numberOfBones, struct Transform* initialPose, unsigned short* boneParentIndex, u32 numberOfAttachments);
Gfx* skBuildAttachments(struct SKArmature* object, Gfx** attachments, struct RenderState* renderState);
void skRenderObject(struct SKArmature* object, Gfx** attachements, struct RenderState* intoState);
void skCalculateTransforms(struct SKArmature* object, Mtx* into);
void skCleanupObject(struct SKArmature* object);
void skCalculateBonePosition(struct SKArmature* object, unsigned short boneIndex, struct Vector3* bonePosition, struct Vector3* out);

#endif