
#include "collision_quad.h"

#include "collision_box.h"
#include "../math/mathf.h"

#define EDGE_ZERO_BIAS  0.001f

#define POINT_NO_OVERLAP    -1

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

int _collsionBuildQuadContact(struct Transform* boxTransform, struct CollisionQuad* quad, struct Vector3* point, struct ContactConstraintState* output, int id) {
    struct Vector3 worldPoint;
    struct ContactState* contact = &output->contacts[output->contactCount];

    quatMultVector(&boxTransform->rotation, point, &contact->rb);
    vector3Add(&contact->rb, &boxTransform->position, &worldPoint);
    float penetration = planePointDistance(&quad->plane, &worldPoint);

    if (penetration >= NEGATIVE_PENETRATION_BIAS) {
        return POINT_NO_OVERLAP;
    }

    int edgeMask = collisionQuadDetermineEdges(&worldPoint, quad);

    if (edgeMask) {
        return edgeMask;
    }

    vector3AddScaled(&worldPoint, &quad->plane.normal, -penetration, &contact->ra);

    ++output->contactCount;
    contact->id = id;
    contact->penetration = penetration;
    contact->bias = 0;
    contact->normalMass = 0;
    contact->tangentMass[0] = 0.0f;
    contact->tangentMass[1] = 0.0f;
    contact->normalImpulse = 0.0f;
    contact->tangentImpulse[0] = 0.0f;
    contact->tangentImpulse[1] = 0.0f;

    return 0;
}


struct CubeEdge {
    char corneredge;
};

char otherAxis[] = {
    1,
    0,
    0,
};

char secondOtherAxis[] = {
    2,
    2,
    1,
};

#define ID_FROM_AXIS_DIRS(positiveX, positiveY, positiveZ) (((positiveX) ? 1 : 0) | ((positiveY) ? 2 : 0) | ((positiveZ) ? 4 : 0))
#define ID_SEGEMENT_FROM_AXIS(axisNumber, isPositive) ((isPositive) ? 0 : (1 << axisNumber))

struct CollisionEdge {
    struct Vector3 origin;
    struct Vector3 direction;
    float length;
};

// contactPoint(Pa, Da, Pb, Db)
// offset = Pa - Pb
// Dota = Da * offset
// Dotb = Db * offset
// edgesDot = Da * Db
// denomInv = 1.0f / (1 - edgesDot * edgesDot)
// a = (edgesDot * Dotb - Dota) * denomInv
// b = (Dotb - edgesDot * Dota) * denomInv
// nearestPointA = Pa + Da * a
// nearestPointB = Pb + Db * b

struct ContactState* _collisionEdgeEdge(struct CollisionEdge* quadEdge, struct CollisionEdge* cubeEdge, struct ContactConstraintState* output) {
    float edgesDot = vector3Dot(&quadEdge->direction, &cubeEdge->direction);

    float denomInv = 1.0f - edgesDot * edgesDot;

    if (denomInv < 0.0001f) {
        return NULL;
    }

    denomInv = 1.0f / denomInv;

    struct Vector3 offset;
    vector3Sub(&cubeEdge->origin, &quadEdge->origin, &offset);

    float cubeDot = vector3Dot(&cubeEdge->direction, &offset);
    float edgeDot = vector3Dot(&quadEdge->direction, &offset);

    float quadLerp = (edgeDot - edgesDot * cubeDot) * denomInv;

    if (quadLerp < -EDGE_ZERO_BIAS || quadLerp > quadEdge->length + EDGE_ZERO_BIAS) {
        return NULL;
    }

    float cubeLerp = (edgesDot * edgeDot - cubeDot) * denomInv;

    if (cubeLerp < -EDGE_ZERO_BIAS || cubeLerp > cubeEdge->length + EDGE_ZERO_BIAS) {
        return NULL;
    }

    if (output->contactCount == MAX_CONTACTS_PER_MANIFOLD) {
        return NULL;
    }

    struct ContactState* result = &output->contacts[output->contactCount];

    vector3AddScaled(&quadEdge->origin, &quadEdge->direction, quadLerp, &result->rb);
    vector3AddScaled(&cubeEdge->origin, &cubeEdge->direction, cubeLerp, &result->ra);

    return result;
}

void _collisionBoxCollideEdge(struct CollisionBox* box, struct Transform* boxTransform, int cornerIds, struct CollisionEdge* edge, struct Vector3* edgeDirection, struct ContactConstraintState* output) {
    struct Quaternion inverseBoxRotation;
    quatConjugate(&boxTransform->rotation, &inverseBoxRotation);

    for (int axis = 0; axis < 3; ++axis) {
        struct CollisionEdge cubeEdge;
        cubeEdge.direction = gZeroVec;
        VECTOR3_AS_ARRAY(&cubeEdge.direction)[axis] = 1.0f;
        cubeEdge.length = VECTOR3_AS_ARRAY(&box->sideLength)[axis] * 2.0f;

        int firstAxis = otherAxis[axis];
        int secondAxis = secondOtherAxis[axis];

        VECTOR3_AS_ARRAY(&cubeEdge.origin)[axis] = -VECTOR3_AS_ARRAY(&box->sideLength)[axis];

        for (int corner = 0; corner < 4; ++corner) {
            int isFirstPositive = corner & 1;
            int isSecondPositive = corner & 2;
            int cornerID = ID_SEGEMENT_FROM_AXIS(firstAxis, isFirstPositive) | ID_SEGEMENT_FROM_AXIS(secondAxis, isSecondPositive);

            int cornerIDMask = (1 << cornerID) | (1 << (cornerID | ID_SEGEMENT_FROM_AXIS(axis, 0)));
            if ((cornerIDMask & cornerIds) == 0) {
                continue;
            }

            VECTOR3_AS_ARRAY(&cubeEdge.origin)[firstAxis] = isFirstPositive ? VECTOR3_AS_ARRAY(&box->sideLength)[firstAxis] : -VECTOR3_AS_ARRAY(&box->sideLength)[firstAxis];
            VECTOR3_AS_ARRAY(&cubeEdge.origin)[secondAxis] = isSecondPositive ? VECTOR3_AS_ARRAY(&box->sideLength)[secondAxis] : -VECTOR3_AS_ARRAY(&box->sideLength)[secondAxis];

            struct ContactState* contact = _collisionEdgeEdge(edge, &cubeEdge, output);

            if (!contact) {
                continue;
            }

            // check if contact point is inside of box
            if (fabsf(contact->ra.x) > box->sideLength.x + NEGATIVE_PENETRATION_BIAS ||
                fabsf(contact->ra.y) > box->sideLength.y + NEGATIVE_PENETRATION_BIAS ||
                fabsf(contact->ra.z) > box->sideLength.z + NEGATIVE_PENETRATION_BIAS) {
                continue;
            }

            if (output->contactCount == 0) {
                vector3Cross(&edge->direction, &cubeEdge.direction, &output->normal);
                vector3Normalize(&output->normal, &output->normal);

                if (vector3Dot(edgeDirection, &output->normal) < 0.0f) {
                    vector3Negate(&output->normal, &output->normal);
                }

                quatMultVector(&boxTransform->rotation, &output->normal, &output->normal);
                output->tangentVectors[0] = edge->direction;
                vector3Cross(&output->normal, &edge->direction, &output->tangentVectors[1]);
            }

            if (vector3Dot(&contact->ra, edgeDirection) - vector3Dot(&contact->rb, edgeDirection) > 0) {
                continue;
            }

            // rotate from cube space to world space
            quatMultVector(&boxTransform->rotation, &contact->ra, &contact->ra);
            quatMultVector(&boxTransform->rotation, &contact->rb, &contact->rb);

            contact->penetration = vector3Dot(&contact->ra, &output->normal) - vector3Dot(&contact->rb, &output-> normal);
            if (contact->penetration >= NEGATIVE_PENETRATION_BIAS) {
                continue;
            }


            // move edge contact to world space
            vector3Add(&boxTransform->position, &contact->ra, &contact->ra);

            ++output->contactCount;
            contact->id = 8 + axis + corner * 4;
            contact->bias = 0;
            contact->normalMass = 0;
            contact->tangentMass[0] = 0.0f;
            contact->tangentMass[1] = 0.0f;
            contact->normalImpulse = 0.0f;
            contact->tangentImpulse[0] = 0.0f;
            contact->tangentImpulse[1] = 0.0f;

        }
    }

    return;
}

void _collisionBoxCollidePoint(struct CollisionBox* box, struct Transform* boxTransform, struct Vector3* localPoint, int id, struct ContactConstraintState* output) {
    float minDepth = localPoint->x > 0.0f ? localPoint->x - box->sideLength.x : (-box->sideLength.x - localPoint->x);
    int minAxis = 0;

    if (minDepth > NEGATIVE_PENETRATION_BIAS) {
        return;
    }

    float check = localPoint->y > 0.0f ? localPoint->y - box->sideLength.y : (-box->sideLength.y - localPoint->y);

    if (check > NEGATIVE_PENETRATION_BIAS) {
        return;
    }

    if (check > minDepth) {
        minDepth = check;
        minAxis = 1;
    }


    check = localPoint->z > 0.0f ? localPoint->z - box->sideLength.z : (-box->sideLength.z - localPoint->z);

    if (check > NEGATIVE_PENETRATION_BIAS) {
        return;
    }

    if (check > minDepth) {
        minDepth = check;
        minAxis = 2;
    }


    if (output->contactCount == MAX_CONTACTS_PER_MANIFOLD) {
        return;
    }

    struct ContactState* contact = &output->contacts[output->contactCount];

    int positiveAxis = VECTOR3_AS_ARRAY(localPoint)[minAxis] > 0.0f;

    contact->rb = *localPoint;
    contact->ra = *localPoint;
    VECTOR3_AS_ARRAY(&contact->ra)[minAxis] = positiveAxis ? VECTOR3_AS_ARRAY(&box->sideLength)[minAxis] : -VECTOR3_AS_ARRAY(&box->sideLength)[minAxis];

    if (output->contactCount == 0) {
        output->normal = gZeroVec;
        VECTOR3_AS_ARRAY(&output->normal)[minAxis] = positiveAxis ? 1.0f : -1.0f;
        quatMultVector(&boxTransform->rotation, &output->normal, &output->normal);
    }

    // rotate from cube space to world space
    quatMultVector(&boxTransform->rotation, &contact->ra, &contact->ra);
    quatMultVector(&boxTransform->rotation, &contact->rb, &contact->rb);

    contact->penetration = vector3Dot(&contact->ra, &output->normal) - vector3Dot(&contact->rb, &output-> normal);
    if (contact->penetration >= NEGATIVE_PENETRATION_BIAS) {
        return;
    }

    // move edge contact to world space
    vector3Add(&boxTransform->position, &contact->ra, &contact->ra);

    ++output->contactCount;
    contact->id = 24 + minAxis + positiveAxis * 3 + id * 6;
    contact->bias = 0;
    contact->normalMass = 0;
    contact->tangentMass[0] = 0.0f;
    contact->tangentMass[1] = 0.0f;
    contact->normalImpulse = 0.0f;
    contact->tangentImpulse[0] = 0.0f;
    contact->tangentImpulse[1] = 0.0f;
}

int collisionBoxCollideQuad(void* data, struct Transform* boxTransform, struct CollisionQuad* quad, struct ContactConstraintState* output) {
    struct CollisionBox* box = (struct CollisionBox*)data;

    float boxDistance = planePointDistance(&quad->plane, &boxTransform->position);
    float maxBoxReach = vector3MagSqrd(&box->sideLength);

    if (boxDistance > maxBoxReach || boxDistance < 0.0f) {
        return 0;
    }

    struct Quaternion inverseBoxRotation;

    quatConjugate(&boxTransform->rotation, &inverseBoxRotation);

    struct Vector3 normalInBoxSpace;

    quatMultVector(&inverseBoxRotation, &quad->plane.normal, &normalInBoxSpace);

    struct Vector3 deepestCorner;

    int id = 0;

    for (int axis = 0; axis < 3; ++axis) {
        float normalValue = VECTOR3_AS_ARRAY(&normalInBoxSpace)[axis];
        if (normalValue < 0) {
            VECTOR3_AS_ARRAY(&deepestCorner)[axis] = VECTOR3_AS_ARRAY(&box->sideLength)[axis];
        } else {
            VECTOR3_AS_ARRAY(&deepestCorner)[axis] = -VECTOR3_AS_ARRAY(&box->sideLength)[axis];
            id |= 1 << axis;
        }
    }

    output->contactCount = 0;
    output->normal = quad->plane.normal;
    // TODO actually calculate tangent 
    output->tangentVectors[0] = quad->edgeA;
    output->tangentVectors[1] = quad->edgeB;

    output->restitution = 0.0f;
    output->friction = 1.0f;

    int edges = _collsionBuildQuadContact(boxTransform, quad, &deepestCorner, output, id);
    int cornerIds = 0;

    if (edges == POINT_NO_OVERLAP) {
        return 0;
    }

    if (edges > 0) {
        cornerIds |= (1 << id);
    }

    struct Vector3 sideLengthProjected;
    vector3Multiply(&box->sideLength, &normalInBoxSpace, &sideLengthProjected);

    int minAxis = 0;
    float minAxisDistnace = fabsf(sideLengthProjected.x);
    int maxAxis = 0;
    float maxAxisDistnace = minAxisDistnace;

    for (int axis = 1; axis < 3; ++axis) {
        float length = fabsf(VECTOR3_AS_ARRAY(&sideLengthProjected)[axis]);

        if (length < minAxisDistnace) {
            minAxisDistnace = length;
            minAxis = axis;
        }

        if (length > maxAxisDistnace) {
            maxAxisDistnace = length;
            maxAxis = axis;
        }
    }

    int midAxis = 3 - maxAxis - minAxis;

    struct Vector3 nextFurthestPoint = deepestCorner;

    // TODO roll up into a loop
    int nextId = id ^ (1 << minAxis);
    VECTOR3_AS_ARRAY(&nextFurthestPoint)[minAxis] = -VECTOR3_AS_ARRAY(&nextFurthestPoint)[minAxis];
    int pointEdges = _collsionBuildQuadContact(boxTransform, quad, &nextFurthestPoint, output, nextId);
    if (pointEdges > 0) {
        edges |= pointEdges;
        cornerIds |= (1 << nextId);
    }

    nextId = nextId ^ (1 << midAxis);
    VECTOR3_AS_ARRAY(&nextFurthestPoint)[midAxis] = -VECTOR3_AS_ARRAY(&nextFurthestPoint)[midAxis];
    pointEdges =_collsionBuildQuadContact(boxTransform, quad, &nextFurthestPoint, output, nextId);
    if (pointEdges > 0) {
        edges |= pointEdges;
        cornerIds |= (1 << nextId);
    }

    nextId = nextId ^ (1 << minAxis);
    VECTOR3_AS_ARRAY(&nextFurthestPoint)[minAxis] = -VECTOR3_AS_ARRAY(&nextFurthestPoint)[minAxis];
    pointEdges = _collsionBuildQuadContact(boxTransform, quad, &nextFurthestPoint, output, nextId);
    if (pointEdges > 0) {
        edges |= pointEdges;
        cornerIds |= (1 << nextId);
    }

    edges &= quad->enabledEdges;

    if (edges && cornerIds) {
        struct CollisionEdge edge;
        struct Vector3 edgeDirection;

        struct Vector3 localOrigin;
        struct Vector3 localEdgeA;
        struct Vector3 localEdgeB;

        quatMultVector(&inverseBoxRotation, &quad->edgeA, &localEdgeA);
        quatMultVector(&inverseBoxRotation, &quad->edgeB, &localEdgeB);

        vector3Sub(&quad->corner, &boxTransform->position, &localOrigin);
        quatMultVector(&inverseBoxRotation, &localOrigin, &localOrigin);

        /**
         *        -------
         *        |  3  |
         *      ^ |0   2|
         *      | |  1  |
         * edge b -------
         * edge a -->
         */

        // TODO roll up into a loop
        if (edges & (1 << 0)) {
            edge.origin = localOrigin;
            edge.direction = localEdgeB;
            edge.length = quad->edgeBLength;
            vector3Negate(&localEdgeA, &edgeDirection);
            _collisionBoxCollideEdge(box, boxTransform, cornerIds, &edge, &edgeDirection, output);
        }

        if (edges & (1 << 1)) {
            edge.origin = localOrigin;
            edge.direction = localEdgeA;
            edge.length = quad->edgeALength;
            vector3Negate(&localEdgeB, &edgeDirection);
            _collisionBoxCollideEdge(box, boxTransform, cornerIds, &edge, &edgeDirection, output);
        }

        if (edges & (1 << 2)) {
            vector3AddScaled(&localOrigin, &localEdgeA, quad->edgeALength, &edge.origin);
            edge.direction = localEdgeB;
            edge.length = quad->edgeBLength;
            _collisionBoxCollideEdge(box, boxTransform, cornerIds, &edge, &localEdgeA, output);
        }

        if (edges & (1 << 3)) {
            vector3AddScaled(&localOrigin, &localEdgeB, quad->edgeBLength, &edge.origin);
            edge.direction = localEdgeA;
            edge.length = quad->edgeALength;
            _collisionBoxCollideEdge(box, boxTransform, cornerIds, &edge, &localEdgeB, output);
        }

        if (edges & ((1 << 0) | (1 << 1))) {
            _collisionBoxCollidePoint(box, boxTransform, &localOrigin, 0, output);
        }

        if (edges & ((1 << 1) | (1 << 2))) {
            struct Vector3 origin;
            vector3AddScaled(&localOrigin, &localEdgeA, quad->edgeALength, &origin);
            _collisionBoxCollidePoint(box, boxTransform, &origin, 1, output);
        }

        if (edges & ((1 << 2) | (1 << 3))) {
            struct Vector3 origin;
            vector3AddScaled(&localOrigin, &localEdgeA, quad->edgeALength, &origin);
            vector3AddScaled(&origin, &localEdgeB, quad->edgeBLength, &origin);
            _collisionBoxCollidePoint(box, boxTransform, &origin, 2, output);
        }

        if (edges & ((1 << 3) | (1 << 0))) {
            struct Vector3 origin;
            vector3AddScaled(&localOrigin, &localEdgeB, quad->edgeBLength, &origin);
            _collisionBoxCollidePoint(box, boxTransform, &origin, 3, output);
        }
    }

    return output->contactCount > 0;
}