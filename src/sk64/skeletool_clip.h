#ifndef __SKELETOOL_ANIMATION_CLIP_H__
#define __SKELETOOL_ANIMATION_CLIP_H__

#define SK_ANIMATION_CLIP_DURATION(clip) ((clip)->nFrames / (clip)->fps)
#define SK_ANIMATION_CLIP_START(clip, isReversed) ((isReversed) ? SK_ANIMATION_CLIP_DURATION(clip) : 0.0f)

struct SKU16Vector3 {
    short x;
    short y;
    short z;
};

struct SKAnimationBoneFrame {
    struct SKU16Vector3 position;
    struct SKU16Vector3 rotation;
};

struct SKAnimationClip {
    short nFrames;
    short nBones;
    struct SKAnimationBoneFrame* frames;
    float fps;
};

#endif
