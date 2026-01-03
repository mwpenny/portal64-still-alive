#ifndef __LEVELS_H__
#define __LEVELS_H__

#include "level_definition.h"

#define CREDITS_MENU    -5
#define INTRO_MENU      -4
#define MAIN_MENU       -3
#define NO_QUEUED_LEVEL -2
#define NEXT_LEVEL      -1

extern struct LevelDefinition* gCurrentLevel;
extern int gCurrentLevelIndex;

void levelQueueLoad(int index, struct Transform* relativeTransform, struct Vector3* relativeVelocity, int useCheckpoint);
void levelQueueReload();
int levelGetQueued();
void levelClearQueued();

void levelLoad(int index);
struct Transform* levelRelativeTransform();
struct Vector3* levelRelativeVelocity();

int levelCount();
int getChamberIndexFromLevelIndex(int levelIndex, int roomIndex);
int getLevelIndexFromChamberIndex(int chamberIndex);

int levelMaterialCount();
int levelMaterialTransparentStart();
Gfx* levelMaterial(int index);
Gfx* levelMaterialDefault();
Gfx* levelMaterialRevert(int index);

int levelQuadIndex(struct CollisionObject* pointer);
struct Location* levelGetLocation(short index);

#endif
