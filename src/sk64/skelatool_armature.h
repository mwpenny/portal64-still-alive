#ifndef __SKELATOOL_OBJECT_H
#define __SKELATOOL_OBJECT_H

#include <ultra64.h>
#include "math/transform.h"
#include "graphics/renderstate.h"

#define NO_BONE_PARENT      0xFFFF

struct SKArmatureDefinition {
    Gfx* displayList;
    struct Transform* pose;
    unsigned short* boneParentIndex;
    u16 numberOfBones;
    u16 numberOfAttachments;
};

struct SKArmature {
    Gfx* displayList;
    struct Transform* pose;
    unsigned short* boneParentIndex;
    u16 numberOfBones;
    u16 numberOfAttachments;
};

void skArmatureInit(struct SKArmature* object, struct SKArmatureDefinition* definition);
Gfx* skBuildAttachments(struct SKArmature* object, Gfx** attachments, struct RenderState* renderState);
void skRenderObject(struct SKArmature* object, Gfx** attachements, struct RenderState* intoState);
void skCalculateTransforms(struct SKArmature* object, Mtx* into);
void skCleanupObject(struct SKArmature* object);
void skCalculateBonePosition(struct SKArmature* object, unsigned short boneIndex, struct Vector3* bonePosition, struct Vector3* out);
void skCalculateBoneRotation(struct SKArmature* object, unsigned short boneIndex, struct Quaternion* out);

#endif