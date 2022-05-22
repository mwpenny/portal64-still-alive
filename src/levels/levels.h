#ifndef __LEVEL_LIST_H__
#define __LEVEL_LIST_H__

#include "physics/collision_object.h"
#include "level_definition.h"

int levelCount();
void levelLoad(int index);

extern struct LevelDefinition* gCurrentLevel;

int levelMaterialCount();
int levelMaterialTransparentStart();
Gfx* levelMaterial(int index);
Gfx* levelMaterialDefault();
Gfx* levelMaterialRevert(int index);

int levelQuadIndex(struct CollisionObject* pointer);

int levelCheckDoorwaySides(struct Vector3* position, int currentRoom);

int levelCheckDoorwayCrossings(struct Vector3* position, int currentRoom, int sideMask);

void levelCheckTriggers(struct Vector3* playerPos);

struct Location* levelGetLocation(short index);

#endif