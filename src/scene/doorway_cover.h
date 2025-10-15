#ifndef __DOORWAY_COVER_H__
#define __DOORWAY_COVER_H__

#include "graphics/color.h"
#include "levels/level_definition.h"
#include "math/vector3.h"
#include "physics/world.h"
#include "player/player.h"

struct DoorwayCover {
    struct Doorway* forDoorway;
    struct DoorwayCoverDefinition* definition;
    short dynamicId;
    float opacity;
};

void doorwayCoverInit(struct DoorwayCover* cover, struct DoorwayCoverDefinition* definition, struct World* world);
void doorwayCoverUpdate(struct DoorwayCover* cover, struct Player* player);

#endif
