#ifndef __LEVEL_LIST_H__
#define __LEVEL_LIST_H__

#include "physics/collision_object.h"
#include "level_definition.h"

int levelCount();
void levelLoad(int index);

extern struct LevelDefinition* gCurrentLevel;

int levelMaterialCount();
Gfx* levelMaterial(int index);
Gfx* levelMaterialRevert(int index);

int levelQuadIndex(struct CollisionObject* pointer);

#endif