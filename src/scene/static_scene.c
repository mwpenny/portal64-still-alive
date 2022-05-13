#include "static_scene.h"

#include "defs.h"
#include "../math/box3d.h"

int isOutsideFrustrum(struct FrustrumCullingInformation* frustrum, struct BoundingBoxs16* boundingSphere) {
    struct Box3D boxAsFloat;

    boxAsFloat.min.x = boundingSphere->minX  * (1.0f / SCENE_SCALE);
    boxAsFloat.min.y = boundingSphere->minY  * (1.0f / SCENE_SCALE);
    boxAsFloat.min.z = boundingSphere->minZ  * (1.0f / SCENE_SCALE);

    boxAsFloat.max.x = boundingSphere->maxX  * (1.0f / SCENE_SCALE);
    boxAsFloat.max.y = boundingSphere->maxY  * (1.0f / SCENE_SCALE);
    boxAsFloat.max.z = boundingSphere->maxZ  * (1.0f / SCENE_SCALE);

    for (int i = 0; i < CLIPPING_PLANE_COUNT; ++i) {
        struct Vector3 closestPoint;

        closestPoint.x = frustrum->clippingPlanes[i].normal.x > 0.0f ? boxAsFloat.min.x : boxAsFloat.max.x;
        closestPoint.y = frustrum->clippingPlanes[i].normal.y > 0.0f ? boxAsFloat.min.y : boxAsFloat.max.y;
        closestPoint.z = frustrum->clippingPlanes[i].normal.z > 0.0f ? boxAsFloat.min.z : boxAsFloat.max.z;

        if (planePointDistance(&frustrum->clippingPlanes[i], &closestPoint) < 0.0f) {
            return 1;
        }
    }


    return 0;
}