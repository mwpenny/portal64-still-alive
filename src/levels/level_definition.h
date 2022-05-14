#ifndef __LEVEL_DEFINITION_H__
#define __LEVEL_DEFINITION_H__

#include "../physics/collision_scene.h"
#include "../scene/portal_surface.h"

struct StaticContentElement {
    Gfx* displayList;
    u8 materialIndex;
};

struct BoundingSphere {
    short x, y, z;
    short radius;
};

struct BoundingBoxs16 {
    short minX, minY, minZ;
    short maxX, maxY, maxZ;
};

struct LevelDefinition {
    struct CollisionObject* collisionQuads;
    struct StaticContentElement *staticContent;
    struct BoundingBoxs16* staticBoundingBoxes;
    struct PortalSurface* portalSurfaces;
    // maps index of a collisionQuads to indices in portalSurfaces
    struct PortalSurfaceMapping* portalSurfaceMapping;
    short collisionQuadCount;
    short staticContentCount;
    short portalSurfaceCount;
};

#endif