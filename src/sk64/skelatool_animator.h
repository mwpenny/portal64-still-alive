#ifndef _SKELATOOL_ANIMATOR_H
#define _SKELATOOL_ANIMATOR_H

#include "skelatool_clip.h"
#include "../math/transform.h"

enum SKAnimatorFlags {
    SKAnimatorFlagsLoop = (1 << 0),
    SKAnimatorFlagsDone = (1 << 1),
};

struct SKAnimator {
    struct SKAnimationClip* currentClip;
    float currentTime;
    float blendLerp;
    struct SKAnimationBoneFrame* boneState[2];
    short boneStateFrames[2];
    short nextFrameStateIndex;
    short flags;
    short nBones;
};

void skAnimatorInit(struct SKAnimator* animator, int nBones);
void skAnimatorCleanup(struct SKAnimator* animator);
void skAnimatorUpdate(struct SKAnimator* animator, struct Transform* transforms, float deltaTime);

void skAnimatorRunClip(struct SKAnimator* animator, struct SKAnimationClip* clip, float startTime, int flags);

int skAnimatorIsRunning(struct SKAnimator* animator);

void skAnimatorSync();

#define SK_SEGMENT_COUNT 16

void skSetSegmentLocation(unsigned segmentNumber, unsigned segmentLocatoin);
u32 skTranslateSegment(unsigned address);

struct SKAnimatorBlender {
    struct SKAnimator from;
    struct SKAnimator to;

    float blendLerp;
};

void skBlenderInit(struct SKAnimatorBlender* blender, int nBones);
void skBlenderCleanup(struct SKAnimatorBlender* animator);
void skBlenderUpdate(struct SKAnimatorBlender* blender, struct Transform* transforms, float deltaTime);

#endif
