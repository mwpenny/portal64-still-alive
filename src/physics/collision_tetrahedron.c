#include "collision_tetrahedron.h"

#include "collision_object.h"
#include "raycasting.h"

float collisionTetrahedronSolidMofI(struct ColliderTypeData* typeData, float mass) {
    struct CollisionTetrahedron* tetrahedron = typeData->data;

    float height = 2 * tetrahedron->dimensions.y;
    float halfWidth = tetrahedron->dimensions.x;
    float halfDepth = tetrahedron->dimensions.z;

    float sideLenSqrd = (height * height) + MAX(halfWidth * halfWidth, halfDepth * halfDepth);
    return mass * sideLenSqrd * (1.0f / 20.0f);
}

void collisionTetrahedronBoundingBox(struct ColliderTypeData* typeData, struct Transform* transform, struct Box3D* box) {
    struct CollisionTetrahedron* tetrahedron = typeData->data;
    struct Vector3 halfSize;
    quatRotatedBoundingBoxSize(&transform->rotation, &tetrahedron->dimensions, &halfSize);
    vector3Sub(&transform->position, &halfSize, &box->min);
    vector3Add(&transform->position, &halfSize, &box->max);
}

int collisionTetrahedronMinkowskiSupport(void* data, struct Basis* basis, struct Vector3* direction, struct Vector3* output) {
    struct CollisionTetrahedron* tetrahedron = data;

    // 2 current vertices to compare
    struct Vector3 vertices[2];
    float distances[2];

    // Bottom back
    vector3Scale(&basis->y, &vertices[1], -tetrahedron->dimensions.y);
    vector3AddScaled(&vertices[1], &basis->z, -tetrahedron->dimensions.z, &vertices[0]);
    distances[0] = vector3Dot(direction, &vertices[0]);

    // Bottom front
    int xDir = vector3Dot(&basis->x, direction) > 0.0f;
    vector3AddScaled(&vertices[1], &basis->z, tetrahedron->dimensions.z, &vertices[1]);
    vector3AddScaled(&vertices[1], &basis->x, xDir ? tetrahedron->dimensions.x : -tetrahedron->dimensions.x, &vertices[1]);
    distances[1] = vector3Dot(direction, &vertices[1]);

    // Direction is not normalized but only the comparison matters, not value
    int topIndex = (distances[0] > distances[1]) ? 1 : 0;
    int bottomIndex = topIndex ^ 1;

    vector3Scale(&basis->y, &vertices[topIndex], tetrahedron->dimensions.y);
    if (vector3Dot(direction, &vertices[topIndex]) > distances[bottomIndex]) {
        *output = vertices[topIndex];

        // Top vertex touches all faces but bottom
        return 0x7;
    } else {
        *output = vertices[bottomIndex];

        // Bottom back vertex touches all faces but front
        return (bottomIndex == 0) ? 0xE : (0x9 | (xDir ? 0x2 : 0x4));
    }
}

struct ColliderCallbacks gCollisionTetrahedronCallbacks = {
    NULL, // TODO
    collisionTetrahedronSolidMofI,
    collisionTetrahedronBoundingBox,
    collisionTetrahedronMinkowskiSupport,
};
