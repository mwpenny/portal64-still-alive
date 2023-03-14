#ifndef _SCENE_BOX_DROPPER_H__
#define _SCENE_BOX_DROPPER_H__

#include "../math/transform.h"
#include "../sk64/skelatool_armature.h"
#include "../sk64/skelatool_animator.h"
#include "../levels/level_definition.h"
#include "../audio/clips.h"
#include "../audio/soundplayer.h"

#include "../decor/decor_object.h"

enum BoxDropperFlags {
    BoxDropperFlagsCubeRequested = (1 << 0),
    BoxDropperFlagsCubeIsActive = (1 << 1),
    BoxDropperFlagsSignalWasSet = (1 << 2),
};

struct BoxDropper {
    struct Transform transform;
    struct SKArmature armature;
    struct SKAnimator animator;

    struct DecorObject activeCube;
    float reloadTimer;

    short dynamicId;
    short roomIndex;
    short signalIndex;
    short flags;

};

void boxDropperInit(struct BoxDropper* dropper, struct BoxDropperDefinition* definition);

void boxDropperUpdate(struct BoxDropper* dropper);

#endif