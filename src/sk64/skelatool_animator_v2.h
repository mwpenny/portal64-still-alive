#ifndef _SKELATOOL_ANIMATOR_H
#define _SKELATOOL_ANIMATOR_H

#include "skelatool_clip.h"
#include "../math/transform.h"

enum SKAnimatorV2Flags {
    SKAnimatorV2FlagsLoop = (1 << 0),
};

struct SKAnimatorV2 {
    struct SKAnimationClip* currentClip;
    float currentTime;
    float blendLerp;
    struct SKAnimationBoneFrame* boneState[2];
    short boneStateFrames[2];
    short latestBoneState;
    short flags;
    short nBones;
};

void skAnimatorV2Init(struct SKAnimatorV2* animator, int nBones);
void skAnimatorV2Cleanup(struct SKAnimatorV2* animator);
void skAnimatorV2Update(struct SKAnimatorV2* animator, struct Transform* transforms, float deltaTime);

void skAnimatorV2RunClip(struct SKAnimatorV2* animator, struct SKAnimationClip* clip, float startTime, int flags);

#endif
