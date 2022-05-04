#ifndef __LEVEL_DEFINITION_H__
#define __LEVEL_DEFINITION_H__

#include "../physics/collision_scene.h"

struct StaticContentElement {
    Gfx* displayList;
    u8 materialIndex;
};

struct LevelDefinition {
    struct CollisionObject* collisionQuads;
    struct StaticContentElement *staticContent;
    short collisionQuadCount;
    short staticContentCount;
};

#endif