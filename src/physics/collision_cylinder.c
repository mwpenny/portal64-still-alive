#include "collision_cylinder.h"

#include "../math/mathf.h"
#include "contact_solver.h"
#include "collision_quad.h"
#include "raycasting.h"
#include "line.h"

struct ColliderCallbacks gCollisionCylinderCallbacks = {
    NULL,
    raycastCylinder,
    collisionCylinderSolidMofI,
    collisionCylinderBoundingBox,
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