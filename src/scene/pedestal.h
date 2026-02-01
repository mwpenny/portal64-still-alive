#ifndef __SCENE_PEDESTAL_H__
#define __SCENE_PEDESTAL_H__

#include "audio/clips.h"
#include "audio/soundplayer.h"
#include "levels/level_definition.h"
#include "sk64/skeletool_armature.h"
#include "sk64/skeletool_animator.h"

enum PedestalFlags {
    PedestalFlagsDown = (1 << 0),
    PedestalFlagsIsPointing = (1 << 1),
    PedestalFlagsAlreadyMoving  = (1 << 2),
    PedestalFlagsPlayShootingSound = (1 << 3),
};

struct Pedestal {
    struct Transform transform;
    struct SKArmature armature; 
    struct SKAnimator animator;

    short dynamicId;
    short roomIndex;

    short flags;

    struct Vector2 targetRotation;
    struct Vector2 currentRotation;
};

void pedestalInit(struct Pedestal* pedestal, struct PedestalDefinition* definition);
void pedestalUpdate(struct Pedestal* pedestal);

void pedestalHide(struct Pedestal* pedestal);
void pedestalPointAt(struct Pedestal* pedestal, struct Vector3* target, int playShootingSound);

void pedestalSetDown(struct Pedestal* pedestal);

#endif