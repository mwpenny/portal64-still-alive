#include "collision_cylinder.h"

#include "../math/mathf.h"
#include "contact_solver.h"
#include "collision_quad.h"
#include "raycasting.h"
#include "line.h"

struct ColliderCallbacks gCollisionCylinderCallbacks = {
    NULL,
    collisionCylinderCollideQuad,
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

int _collisionPointCheckOverlapWithQuad(struct Vector3* pointToCheck, struct Vector3* colliderCenter, struct CollisionQuad* quad, struct ContactConstraintState* output, int id) {
    float edgeDistance = planePointDistance(&quad->plane, pointToCheck);

    if (edgeDistance > NEGATIVE_PENETRATION_BIAS) {
        return POINT_NO_OVERLAP;
    }

    int edgesToCheck = collisionQuadDetermineEdges(pointToCheck, quad);

    if (!edgesToCheck) {
        if (output->contactCount == 0) {
            collisionQuadInitializeNormalContact(quad, output);
        }

        struct ContactState* contact = &output->contacts[output->contactCount];

        ++output->contactCount;

        vector3Sub(pointToCheck, colliderCenter, &contact->rb);
        vector3AddScaled(pointToCheck, &quad->plane.normal, -edgeDistance, &contact->ra);

        contact->id = id;
        contact->penetration = edgeDistance;
        contact->bias = 0;
        contact->normalMass = 0;
        contact->tangentMass[0] = 0.0f;
        contact->tangentMass[1] = 0.0f;
        contact->normalImpulse = 0.0f;
        contact->tangentImpulse[0] = 0.0f;
        contact->tangentImpulse[1] = 0.0f;
    }

    return edgesToCheck;
}

int _collisionCylinderParallel(struct CollisionCylinder* cylinder, struct Transform* cylinderTransform, struct Vector3* centerAxis, struct Vector3* crossAxis, float normalDotProduct, struct CollisionQuad* quad, struct ContactConstraintState* output) {
    struct Vector3 edgeEndpoint;
    vector3AddScaled(&cylinderTransform->position, centerAxis, normalDotProduct > 0.0f ? -cylinder->halfHeight : cylinder->halfHeight, &edgeEndpoint);
    vector3Add(&edgeEndpoint, crossAxis, &edgeEndpoint);

    int edgesToCheck = _collisionPointCheckOverlapWithQuad(&edgeEndpoint, &cylinderTransform->position, quad, output, 0);

    if (edgesToCheck == POINT_NO_OVERLAP) {
        return 0;
    }

    vector3AddScaled(&edgeEndpoint, centerAxis,  (normalDotProduct > 0.0f ? 2.0f : -2.0f) * cylinder->halfHeight, &edgeEndpoint);

    int otherEdgesToCheck = _collisionPointCheckOverlapWithQuad(&edgeEndpoint, &cylinderTransform->position, quad, output, 1);

    if (otherEdgesToCheck != POINT_NO_OVERLAP) {
        edgesToCheck |= otherEdgesToCheck;
    }

    return edgesToCheck;
}

int _collisionCylinderPerpendicular(struct CollisionCylinder* cylinder, struct Transform* cylinderTransform, struct Vector3* centerAxis, struct Vector3* crossAxis, float normalDotProduct, struct CollisionQuad* quad, struct ContactConstraintState* output) {
    float centerDistance = planePointDistance(&quad->plane, &cylinderTransform->position);

    if (centerDistance < -cylinder->radius) {
        return 0;
    }

    struct Vector3 edgeEndpoint;
    struct Vector3 centerPoint;
    vector3AddScaled(&cylinderTransform->position, centerAxis, normalDotProduct > 0.0f ? -cylinder->halfHeight : cylinder->halfHeight, &centerPoint);
    vector3Add(&centerPoint, crossAxis, &edgeEndpoint);

    int edgesToCheck = _collisionPointCheckOverlapWithQuad(&edgeEndpoint, &cylinderTransform->position, quad, output, 0);

    if (edgesToCheck == POINT_NO_OVERLAP) {
        return 0;
    }

    struct Vector3 rotatedCrossAxis;
    vector3Cross(centerAxis, crossAxis, &rotatedCrossAxis);
    vector3Add(&centerPoint, &rotatedCrossAxis, &edgeEndpoint);
    int otherEdgesToCheck = _collisionPointCheckOverlapWithQuad(&edgeEndpoint, &cylinderTransform->position, quad, output, 2);

    if (otherEdgesToCheck == POINT_NO_OVERLAP) {
        return edgesToCheck;
    }
    edgesToCheck |= otherEdgesToCheck;

    vector3Negate(&rotatedCrossAxis, &rotatedCrossAxis);
    vector3Add(&centerPoint, &rotatedCrossAxis, &edgeEndpoint);
    otherEdgesToCheck = _collisionPointCheckOverlapWithQuad(&edgeEndpoint, &cylinderTransform->position, quad, output, 3);

    if (otherEdgesToCheck == POINT_NO_OVERLAP) {
        return edgesToCheck;
    }
    edgesToCheck |= otherEdgesToCheck;

    vector3Negate(crossAxis, crossAxis);
    vector3Add(&centerPoint, crossAxis, &edgeEndpoint);
    otherEdgesToCheck = _collisionPointCheckOverlapWithQuad(&edgeEndpoint, &cylinderTransform->position, quad, output, 4);

    if (otherEdgesToCheck != POINT_NO_OVERLAP) {
        edgesToCheck |= otherEdgesToCheck;
    }

    return edgesToCheck;
}

#define EDGE_LERP_BIAS  0.00001f

void collisionCylinderSingleCap(struct CollisionCylinder* cylinder, struct Transform* cylinderTransform, struct Vector3* centerAxis, float capDistance, float invDot, int idOffset, struct CollisionEdge* edge, struct Vector3* edgeDirection, struct ContactConstraintState* output) {
    float dOffset = vector3Dot(&cylinderTransform->position, centerAxis);
    float lineDot = vector3Dot(&edge->origin, centerAxis);

    float distance = -(lineDot - (dOffset + capDistance)) * invDot;

    if (distance < EDGE_LERP_BIAS || distance > edge->length + EDGE_LERP_BIAS) {
        return;
    }

    struct ContactState* contact = &output->contacts[output->contactCount];

    vector3AddScaled(&cylinderTransform->position, centerAxis, capDistance, &contact->rb);
    vector3AddScaled(&edge->origin, &edge->direction, distance, &contact->ra);

    struct Vector3 offset;
    vector3Sub(&contact->rb, &contact->ra, &offset);
    float offsetSqrd = vector3MagSqrd(&offset);

    if (offsetSqrd > cylinder->radius * cylinder->radius) {
        return;
    }

    if (vector3Dot(&offset, edgeDirection) < 0.0f) {
        return;
    }

    if (output->contactCount == 0) {
        if (offsetSqrd < 0.00001f) {
            vector3Cross(centerAxis, edgeDirection, &output->normal);
        } else {
            vector3Sub(&contact->ra, &contact->ra, &output->normal);
        }
        vector3Normalize(&output->normal, &output->normal);
    
        output->tangentVectors[0] = *centerAxis;
        vector3Cross(&output->normal, &output->tangentVectors[0], &output->tangentVectors[1]);
        output->restitution = 0.0f;
        output->friction = 1.0f;
    }

    vector3AddScaled(&contact->rb, &output->normal, -cylinder->radius, &contact->rb);

    contact->penetration = vector3Dot(&contact->ra, &output->normal) - vector3Dot(&contact->rb, &output-> normal);
    if (contact->penetration >= NEGATIVE_PENETRATION_BIAS) {
        return;
    }
    
    contact->id = idOffset;
    contact->bias = 0;
    contact->normalMass = 0;
    contact->tangentMass[0] = 0.0f;
    contact->tangentMass[1] = 0.0f;
    contact->normalImpulse = 0.0f;
    contact->tangentImpulse[0] = 0.0f;
    contact->tangentImpulse[1] = 0.0f;

    ++output->contactCount;
}

int collisionCylinderCap(struct CollisionCylinder* cylinder, struct Transform* cylinderTransform, struct Vector3* centerAxis, float normalDotProduct, int idOffset, struct CollisionEdge* edge, struct Vector3* edgeDirection, struct ContactConstraintState* output) {
    if (fabsf(normalDotProduct) < 0.00001f) {
        return 0;
    }

    float invDot = 1.0f / normalDotProduct;

    collisionCylinderSingleCap(cylinder, cylinderTransform, centerAxis, cylinder->halfHeight, invDot, idOffset + 1, edge, edgeDirection, output);
    collisionCylinderSingleCap(cylinder, cylinderTransform, centerAxis, -cylinder->halfHeight, invDot, idOffset + 2, edge, edgeDirection, output);

    return output->contactCount > 0;
}

int collisionCylinderEdge(struct CollisionCylinder* cylinder, struct Transform* cylinderTransform, struct Vector3* centerAxis, float normalDotProduct, int idOffset, struct CollisionEdge* edge, struct Vector3* edgeDirection, struct ContactConstraintState* output) {
    float cylinderLerp;
    float edgeLerp;
    
    if (!lineNearestApproach(&cylinderTransform->position, centerAxis, &edge->origin, &edge->direction, &cylinderLerp, &edgeLerp)) {
        return collisionCylinderCap(cylinder, cylinderTransform, centerAxis, normalDotProduct, idOffset, edge, edgeDirection, output);
    }

    struct ContactState* contact = &output->contacts[output->contactCount];

    vector3AddScaled(&cylinderTransform->position, centerAxis, cylinderLerp, &contact->rb);
    vector3AddScaled(&edge->origin, &edge->direction, edgeLerp, &contact->ra);

    struct Vector3 offset;
    vector3Sub(&contact->rb, &contact->ra, &offset);
    float offsetSqrd = vector3MagSqrd(&offset);

    if (offsetSqrd > cylinder->radius * cylinder->radius) {
        return 0;
    }

    if (cylinderLerp > -cylinder->halfHeight - EDGE_LERP_BIAS && cylinderLerp < cylinder->halfHeight + EDGE_LERP_BIAS &&
        edgeLerp > -EDGE_LERP_BIAS && edgeLerp < edge->length + EDGE_LERP_BIAS) {

        if (vector3Dot(&offset, edgeDirection) < 0.0f) {
            return 0;
        }

        struct Vector3 offsetNormalized;

        if (offsetSqrd < 0.00001f) {
            vector3Cross(centerAxis, &edge->direction, &offsetNormalized);
            vector3Normalize(&offsetNormalized, &offsetNormalized);
        } else {
            vector3Scale(&offset, &offsetNormalized, 1.0f / sqrtf(offsetSqrd));
        }

        if (output->contactCount == 0) {
            output->normal = offsetNormalized;
            output->tangentVectors[0] = *centerAxis;
            vector3Cross(&output->normal, &output->tangentVectors[0], &output->tangentVectors[1]);
            output->restitution = 0.0f;
            output->friction = 1.0f;
        }

        vector3AddScaled(&contact->rb, &offsetNormalized, -cylinder->radius, &contact->rb);

        contact->penetration = vector3Dot(&contact->ra, &output->normal) - vector3Dot(&contact->rb, &output-> normal);
        if (contact->penetration >= NEGATIVE_PENETRATION_BIAS) {
            return 0;
        }

        contact->id = idOffset;
        contact->bias = 0;
        contact->normalMass = 0;
        contact->tangentMass[0] = 0.0f;
        contact->tangentMass[1] = 0.0f;
        contact->normalImpulse = 0.0f;
        contact->tangentImpulse[0] = 0.0f;
        contact->tangentImpulse[1] = 0.0f;

        ++output->contactCount;
        return 1;
    }

    return collisionCylinderCap(cylinder, cylinderTransform, centerAxis, normalDotProduct, idOffset, edge, edgeDirection, output);
}

int collisionCylinderCollideQuad(void* data, struct Transform* cylinderTransform, struct CollisionQuad* quad, struct ContactConstraintState* output) {
    struct Vector3 centerAxis;
    quatMultVector(&cylinderTransform->rotation, &gUp, &centerAxis);

    float normalDotProduct = vector3Dot(&centerAxis, &quad->plane.normal);

    struct Vector3 capCenterTowardsPlane;
    vector3AddScaled(&quad->plane.normal, &centerAxis, -normalDotProduct, &capCenterTowardsPlane);
    vector3Negate(&capCenterTowardsPlane, &capCenterTowardsPlane);

    float magSqrd = vector3MagSqrd(&capCenterTowardsPlane);

    int edgesToCheck;

    struct CollisionCylinder* cylinder = (struct CollisionCylinder*)data;

    output->contactCount = 0;

    if (fabsf(magSqrd) < 0.7f) {
        if (magSqrd < 0.0001f) {
            // if the two shapes are too well aligned then pick
            // an arbitrary axis to use to construct face points
            if (fabsf(centerAxis.x) > fabsf(centerAxis.z)) {
                vector3Cross(&gForward, &centerAxis, &capCenterTowardsPlane);
            } else {
                vector3Cross(&gRight, &centerAxis, &capCenterTowardsPlane);
            }
            vector3Scale(&capCenterTowardsPlane, &capCenterTowardsPlane, cylinder->radius / sqrtf(vector3MagSqrd(&capCenterTowardsPlane)));
        } else {
            vector3Scale(&capCenterTowardsPlane, &capCenterTowardsPlane, cylinder->radius / sqrtf(magSqrd));
        }
        edgesToCheck = _collisionCylinderPerpendicular(cylinder, cylinderTransform, &centerAxis, &capCenterTowardsPlane, normalDotProduct, quad, output);
    } else {
        vector3Scale(&capCenterTowardsPlane, &capCenterTowardsPlane, cylinder->radius / sqrtf(magSqrd));
        edgesToCheck = _collisionCylinderParallel(cylinder, cylinderTransform, &centerAxis, &capCenterTowardsPlane, normalDotProduct, quad, output);
    }

    if (!edgesToCheck) {
        return output->contactCount > 0;
    }

    struct CollisionEdge edge;
    struct Vector3 edgeDirection;

    if (edgesToCheck & (1 << 0)) {
        edge.origin = quad->corner;
        edge.direction = quad->edgeB;
        edge.length = quad->edgeBLength;
        vector3Negate(&quad->edgeA, &edgeDirection);
        collisionCylinderEdge(cylinder, cylinderTransform, &centerAxis, normalDotProduct, 5, &edge, &edgeDirection, output);
    }

    if (edgesToCheck & (1 << 1)) {
        edge.origin = quad->corner;
        edge.direction = quad->edgeA;
        edge.length = quad->edgeALength;
        vector3Negate(&quad->edgeB, &edgeDirection);
        collisionCylinderEdge(cylinder, cylinderTransform, &centerAxis, normalDotProduct, 8, &edge, &edgeDirection, output);
    }

    if (edgesToCheck & (1 << 2)) {
        vector3AddScaled(&quad->corner, &quad->edgeA, quad->edgeALength, &edge.origin);
        edge.direction = quad->edgeB;
        edge.length = quad->edgeBLength;
        collisionCylinderEdge(cylinder, cylinderTransform, &centerAxis, normalDotProduct, 11, &edge, &edgeDirection, output);
    }

    if (edgesToCheck & (1 << 3)) {
        vector3AddScaled(&quad->corner, &quad->edgeB, quad->edgeBLength, &edge.origin);
        edge.direction = quad->edgeA;
        edge.length = quad->edgeALength;
        collisionCylinderEdge(cylinder, cylinderTransform, &centerAxis, normalDotProduct, 14, &edge, &edgeDirection, output);
    }
    
    // TODO check edges and points of quad

    return 0;
}