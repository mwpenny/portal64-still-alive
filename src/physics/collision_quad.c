
#include "collision_quad.h"

#include "collision_box.h"
#include "../math/mathf.h"
#include "./line.h"

#define EDGE_ZERO_BIAS  0.001f

void collisionQuadInitializeNormalContact(struct CollisionQuad* quad, struct ContactManifold* output) {
    output->normal = quad->plane.normal;
    output->tangentVectors[0] = quad->edgeA;
    output->tangentVectors[1] = quad->edgeB;

    output->restitution = 0.0f;
    output->friction = 1.0f;
}

/**
 *        -------
 *        |  3  |
 *      ^ |0   2|
 *      | |  1  |
 * edge b -------
 * edge a -->
 */

int collisionQuadDetermineEdges(struct Vector3* worldPoint, struct CollisionQuad* quad) {
    struct Vector3 relative;
    vector3Sub(worldPoint, &quad->corner, &relative);
    float edgeDistance = vector3Dot(&relative, &quad->edgeA);

    int edgeMask = 0;

    if (edgeDistance < -EDGE_ZERO_BIAS) {
        edgeMask |= 1 << 0;
    }

    if (edgeDistance > quad->edgeALength + EDGE_ZERO_BIAS) {
        edgeMask |= 1 << 2;
    }

    edgeDistance = vector3Dot(&relative, &quad->edgeB);

    if (edgeDistance < -EDGE_ZERO_BIAS) {
        edgeMask |= 1 << 1;
    }

    if (edgeDistance > quad->edgeBLength + EDGE_ZERO_BIAS) {
        edgeMask |= 1 << 3;
    }

    return edgeMask;
}