
#ifndef __STATIC_SCENE_H__
#define __STATIC_SCENE_H__

#include <ultra64.h>
#include "../math/transform.h"
#include "../math/box3d.h"
#include "../levels/level_definition.h"
#include "../scene/camera.h"

enum StaticSceneEntryFlags {
    StaticSceneEntryFlagsHidden,
};

struct StaticSceneEntry {
    enum StaticSceneEntryFlags flags;
    Gfx* displayList;
    Gfx* material;
    struct Transform transform;
    struct Box3D* bounds;
};

struct StaticSceneIndexNode {
    u16 nextIndexNode;
    u16 entryIndex;
};

struct StaticScene {
    struct StaticSceneEntry* entries;
    u16* entryIndexGrid;
    u16 gridWidth;
    u16 gridHeight;
};

void staticSceneInit();

#endif