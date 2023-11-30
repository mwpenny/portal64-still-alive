#ifndef __LEVEL_LIST_H__
#define __LEVEL_LIST_H__

#include "physics/collision_object.h"
#include "level_definition.h"

#define CREDITS_MENU    -5
#define INTRO_MENU      -4
#define MAIN_MENU       -3
#define NO_QUEUED_LEVEL -2
#define NEXT_LEVEL      -1

int levelCount();
void levelLoad(int index);

void levelQueueLoad(int index, struct Transform* relativeExitTransform, struct Vector3* relativeVelocity);
void levelLoadLastCheckpoint();
int levelGetQueued();
struct Transform* levelRelativeTransform();
struct Vector3* levelRelativeVelocity();

extern struct LevelDefinition* gCurrentLevel;
extern int gCurrentLevelIndex;

int levelMaterialCount();
int levelMaterialTransparentStart();
Gfx* levelMaterial(int index);
Gfx* levelMaterialDefault();
Gfx* levelMaterialRevert(int index);

int levelQuadIndex(struct CollisionObject* pointer);

int getChamberDisplayNumberFromLevelIndex(int levelIndex, int roomIndex);
void levelClearQueuedLevel();
int getLevelIndexFromChamberDisplayNumber(int chamberNumber);

struct Location* levelGetLocation(short index);

#endif