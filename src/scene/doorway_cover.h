#ifndef __DOORWAY_COVER_H__
#define __DOORWAY_COVER_H__

#include <stdint.h>

#include "graphics/color.h"
#include "levels/level_definition.h"
#include "math/vector3.h"
#include "physics/world.h"
#include "player/player.h"
#include "scene/portal.h"

struct DoorwayCover {
    uint64_t roomFlags;

    struct Doorway* forDoorway;
    struct DoorwayCoverDefinition* definition;
    short dynamicId;
};

void doorwayCoverInit(struct DoorwayCover* cover, struct DoorwayCoverDefinition* definition, struct World* world);
int doorwayCoverIsOpaqueFromView(struct DoorwayCover* cover, struct Vector3* viewPosition);

#endif
