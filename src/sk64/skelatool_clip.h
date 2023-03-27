#ifndef _SKELATOOL_ANIMATION_CLIP_H
#define _SKELATOOL_ANIMATION_CLIP_H

#define SK_ANIMATION_EVENT_END      0xFFFF
#define SK_ANIMATION_EVENT_START    0xFFFE

#define SK_ANIMATION_CLIP_DURATION(clip) ((clip)->nFrames / (clip)->fps)

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