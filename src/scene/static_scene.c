#include "static_scene.h"

#include "defs.h"

int isOutsideFrustrum(struct FrustrumCullingInformation* frustrum, struct BoundingSphere* boundingSphere) {
    struct Vector3 spherePos;

    spherePos.x = boundingSphere->x * (1.0f / SCENE_SCALE);
    spherePos.y = boundingSphere->y * (1.0f / SCENE_SCALE);
    spherePos.z = boundingSphere->z * (1.0f / SCENE_SCALE);

    vector3Sub(&spherePos, &frustrum->cameraPosition, &spherePos);
    struct Vector3 crossDirection;
    vector3Cross(&frustrum->frustrumDirection, &spherePos, &crossDirection);

    float distance = sqrtf(vector3MagSqrd(&crossDirection)) * frustrum->cosFrustumAngle - vector3Dot(&frustrum->frustrumDirection, &crossDirection) * frustrum->sinFrustrumAngle;
    return distance > boundingSphere->radius * (1.0f / SCENE_SCALE);
}