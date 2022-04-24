#ifndef __LEVEL_DEFINITION_H__
#define __LEVEL_DEFINITION_H__

#include "../physics/collision_scene.h"

struct LevelDefinition {
    struct CollisionObject* collisionQuads;
    short collisionQuadCount;
};

#endif