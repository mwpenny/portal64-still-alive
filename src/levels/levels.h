#ifndef __LEVEL_LIST_H__
#define __LEVEL_LIST_H__

#include "physics/collision_object.h"
#include "level_definition.h"

#define NO_QUEUED_LEVEL -2
#define NEXT_LEVEL      -1

int levelCount();
void levelLoad(int index);

void levelQueueLoad(int index, struct Transform* relativeExitTransform, struct Vector3* relativeVelocity);
int levelGetQueued();
struct Transform* levelRelativeTransform();
struct Vector3* levelRelativeVelocity();

extern struct LevelDefinition* gCurrentLevel;

int levelMaterialCount();
int levelMaterialTransparentStart();
Gfx* levelMaterial(int index);
Gfx* levelMaterialDefault();
Gfx* levelMaterialRevert(int index);

int levelQuadIndex(struct CollisionObject* pointer);

void levelCheckTriggers(struct Vector3* playerPos);

struct Location* levelGetLocation(short index);

#endif