#include "collision_box.h"

#include "raycasting.h"
#include "../math/mathf.h"

#include <math.h>

struct ColliderCallbacks gCollisionBoxCallbacks = {
    raycastBox,
    collisionBoxSolidMofI,
    collisionBoxBoundingBox,
    collisionBoxMinkowsiSum,
};

float collisionBoxSolidMofI(struct ColliderTypeData* typeData, float mass) {
    struct CollisionBox* collisionBox = (struct CollisionBox*)typeData->data;

    float xx = collisionBox->sideLength.x * collisionBox->sideLength.x;
    float yy = collisionBox->sideLength.y * collisionBox->sideLength.y;
    float zz = collisionBox->sideLength.z * collisionBox->sideLength.z;

    float biggestSide = xx + yy;

    float sideCheck = yy + zz;

    if (sideCheck > biggestSide) {
        biggestSide = sideCheck;
    }

    sideCheck = zz + xx;

    if (sideCheck > biggestSide) {
        biggestSide = sideCheck;
    }


    return mass * biggestSide * (1.0f / 12.0f);
}

void collisionBoxBoundingBox(struct ColliderTypeData* typeData, struct Transform* transform, struct Box3D* box) {
    struct CollisionBox* collisionBox = (struct CollisionBox*)typeData->data;
    struct Vector3 halfSize;
    quatRotatedBoundingBoxSize(&transform->rotation, &collisionBox->sideLength, &halfSize);
    vector3Sub(&transform->position, &halfSize, &box->min);
    vector3Add(&transform->position, &halfSize, &box->max);
}

int collisionBoxMinkowsiSum(void* data, struct Basis* basis, struct Vector3* direction, struct Vector3* output) {
    struct CollisionBox* collisionBox = (struct CollisionBox*)data;
    int xDir = vector3Dot(&basis->x, direction) > 0.0f;
    int yDir = vector3Dot(&basis->y, direction) > 0.0f;
    int zDir = vector3Dot(&basis->z, direction) > 0.0f;

    vector3Scale(&basis->x, output, xDir ? collisionBox->sideLength.x : -collisionBox->sideLength.x);
    vector3AddScaled(output, &basis->y, yDir ? collisionBox->sideLength.y : -collisionBox->sideLength.y, output);
    vector3AddScaled(output, &basis->z, zDir ? collisionBox->sideLength.z : -collisionBox->sideLength.z, output);

    return (xDir ? 0x1 : 0x2) | (yDir ? 0x4 : 0x8) | (zDir ? 0x10 : 0x20);
}