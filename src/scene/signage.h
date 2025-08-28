#ifndef __SCENE_SIGNAGE_H__
#define __SCENE_SIGNAGE_H__

#include "levels/level_definition.h"
#include "math/transform.h"

struct Signage {
    struct Transform transform;
    short roomIndex;
    short testChamberNumber;
    short currentFrame;
    ALSndId currentSoundId;
};

void signageInit(struct Signage* signage, struct SignageDefinition* definition);
void signageUpdate(struct Signage* signage);
void signageActivate(struct Signage* signage);
void signageDeactivate(struct Signage* signage);

#endif