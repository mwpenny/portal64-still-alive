#ifndef __SCENE_PEDESTAL_H__
#define __SCENE_PEDESTAL_H__

#include "../sk64/skelatool_armature.h"
#include "../sk64/skelatool_animator.h"
#include "../levels/level_definition.h"
#include "../audio/soundplayer.h"
#include "../audio/clips.h"

enum PedestalFlags {
    PedestalFlagsDown = (1 << 0),
    PedestalFlagsIsPointing = (1 << 1),
    PedestalFlagsAlreadyMoving  = (1 << 2),
};

struct Pedestal {
    struct Transform transform;
    struct SKArmature armature; 
    struct SKAnimator animator;

    short dynamicId;
    short roomIndex;

    short flags;

    struct Vector3 pointAt;
    struct Vector2 currentRotation;
};

void pedestalInit(struct Pedestal* pedestal, struct PedestalDefinition* definition);
void pedestalUpdate(struct Pedestal* pedestal);

void pedestalHide(struct Pedestal* pedestal);
void pedestalPointAt(struct Pedestal* pedestal, struct Vector3* target);

void pedestalSetDown(struct Pedestal* pedestal);

#endif