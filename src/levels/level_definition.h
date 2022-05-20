#ifndef __LEVEL_DEFINITION_H__
#define __LEVEL_DEFINITION_H__

#include "../physics/collision_scene.h"
#include "../scene/portal_surface.h"
#include "../math/boxs16.h"
#include "../math/box3d.h"

struct StaticContentElement {
    Gfx* displayList;
    u8 materialIndex;
};

struct BoundingSphere {
    short x, y, z;
    short radius;
};

enum TriggerCutsceneIndex {
    TRIGGER_START,
};

struct Trigger {
    enum TriggerCutsceneIndex triggerIndex;
    struct Box3D box;
};

struct LevelDefinition {
    struct CollisionObject* collisionQuads;
    struct StaticContentElement *staticContent;
    struct BoundingBoxs16* staticBoundingBoxes;
    struct PortalSurface* portalSurfaces;
    // maps index of a collisionQuads to indices in portalSurfaces
    struct PortalSurfaceMapping* portalSurfaceMapping;
    struct Trigger* triggers;
    short collisionQuadCount;
    short staticContentCount;
    short portalSurfaceCount;
    short triggerCount;
};

#endif