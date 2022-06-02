#include "collision_cylinder.h"

#include "../math/mathf.h"
#include "contact_solver.h"
#include "collision_quad.h"
#include "raycasting.h"
#include "line.h"
#include "../math/vector2.h"

struct ColliderCallbacks gCollisionCylinderCallbacks = {
    NULL,
    raycastCylinder,
    collisionCylinderSolidMofI,
    collisionCylinderBoundingBox,
    collisionCylinderMinkowsiSum,
};

float collisionCylinderSolidMofI(struct ColliderTypeData* typeData, float mass) {
    struct CollisionCylinder* cylinder = (struct CollisionCylinder*)typeData->data;

    float rr = cylinder->radius * cylinder->radius;

    float topResult = 0.5f * mass * rr;
    float sideResult = (1.0f / 12.0f) * mass * (3.0f * rr + 4.0f * cylinder->halfHeight * cylinder->halfHeight);

    return MAX(topResult, sideResult);
}

void collisionCylinderBoundingBox(struct ColliderTypeData* typeData, struct Transform* transform, struct Box3D* box) {
    struct CollisionCylinder* cylinder = (struct CollisionCylinder*)typeData->data;
    struct Vector3 radius;
    radius.x = cylinder->radius;
    radius.y = cylinder->halfHeight;
    radius.z = cylinder->radius;
    struct Vector3 halfSize;
    quatRotatedBoundingBoxSize(&transform->rotation, &radius, &halfSize);
    vector3Sub(&transform->position, &halfSize, &box->min);
    vector3Add(&transform->position, &halfSize, &box->max);
}

int collisionCylinderMinkowsiSum(void* data, struct Basis* basis, struct Vector3* direction, struct Vector3* output) {
    struct CollisionCylinder* cylinder = (struct CollisionCylinder*)data;
    int centerDir = vector3Dot(&basis->y, direction) > 0.0f;

    vector3Scale(&basis->y, output, centerDir > 0.0f ? cylinder->halfHeight : -cylinder->halfHeight);

    struct Vector2 crossDirectionCheck;
    crossDirectionCheck.x = vector3Dot(&basis->x, direction);
    crossDirectionCheck.y = vector3Dot(&basis->z, direction);


    float dotDiff = vector2Dot(&crossDirectionCheck, &cylinder->edgeVectors[0]);
    int closest = 0;

    for (int i = 1; i < cylinder->edgeCount; ++i) {
        float dotCheck = vector2Dot(&crossDirectionCheck, &cylinder->edgeVectors[i]);

        if (fabsf(dotCheck) > fabsf(dotDiff)) {
            dotDiff = dotCheck;
            closest = i;
        }
    }

    struct Vector2 crossDirection;

    int faceId = closest;

    if (dotDiff < 0.0f) {
        faceId += cylinder->edgeCount;
        vector2Negate(&cylinder->edgeVectors[closest], &crossDirection);
    } else {
        crossDirection = cylinder->edgeVectors[closest];
    }

    int nextId = faceId + 1;

    if (faceId == cylinder->edgeCount * 2) {
        nextId = 0;
    }

    vector3AddScaled(output, &basis->x, crossDirection.x * cylinder->radius, output);
    vector3AddScaled(output, &basis->z, crossDirection.y * cylinder->radius, output);

    return (centerDir ? 0x1 : 0x2) | (1 << (faceId + 2)) | (1 << (nextId + 2));
}