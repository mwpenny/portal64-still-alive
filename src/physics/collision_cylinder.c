#include "collision_cylinder.h"

#include "../math/mathf.h"
#include "contact_solver.h"
#include "collision_quad.h"

int _collisionPointCheckOverlapWithQuad(struct Vector3* pointToCheck, struct Vector3* colliderCenter, struct CollisionQuad* quad, struct ContactConstraintState* output) {
    float edgeDistance = planePointDistance(&quad->plane, pointToCheck);

    if (edgeDistance > NEGATIVE_PENETRATION_BIAS) {
        return POINT_NO_OVERLAP;
    }

    int edgesToCheck = collisionQuadDetermineEdges(pointToCheck, quad);

    output->contactCount = 0;

    if (!edgesToCheck) {
        if (output->contactCount == 0) {
            collisionQuadInitializeNormalContact(quad, output);
        }

        struct ContactState* contact = &output->contacts[output->contactCount];

        ++output->contactCount;

        vector3Sub(pointToCheck, colliderCenter, &contact->rb);
        vector3AddScaled(pointToCheck, &quad->plane.normal, -edgeDistance, &contact->ra);

        contact->id = 0;
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

    int edgesToCheck = _collisionPointCheckOverlapWithQuad(&edgeEndpoint, &cylinderTransform->position, quad, output);

    if (edgesToCheck == POINT_NO_OVERLAP) {
        return 0;
    }

    vector3AddScaled(&edgeEndpoint, centerAxis,  (normalDotProduct > 0.0f ? 2.0f : -2.0f) * cylinder->halfHeight, &edgeEndpoint);

    int otherEdgesToCheck = _collisionPointCheckOverlapWithQuad(&edgeEndpoint, &cylinderTransform->position, quad, output);

    if (otherEdgesToCheck != POINT_NO_OVERLAP) {
        edgesToCheck |= otherEdgesToCheck;
    }

    return edgesToCheck;
}

int _collisionCylinderPerpendicular(struct CollisionCylinder* cylinder, struct Transform* cylinderTransform, struct Vector3* centerAxis, struct Vector3* crossAxis, float normalDotProduct, struct CollisionQuad* quad, struct ContactConstraintState* output) {
    struct Vector3 edgeEndpoint;
    struct Vector3 centerPoint;
    vector3AddScaled(&cylinderTransform->position, centerAxis, normalDotProduct > 0.0f ? -cylinder->halfHeight : cylinder->halfHeight, &centerPoint);
    vector3Add(&centerPoint, crossAxis, &edgeEndpoint);

    int edgesToCheck = _collisionPointCheckOverlapWithQuad(&edgeEndpoint, &cylinderTransform->position, quad, output);

    if (edgesToCheck == POINT_NO_OVERLAP) {
        return 0;
    }

    struct Vector3 rotatedCrossAxis;
    vector3Cross(centerAxis, crossAxis, &rotatedCrossAxis);
    vector3Add(&centerPoint, &rotatedCrossAxis, &edgeEndpoint);
    int otherEdgesToCheck = _collisionPointCheckOverlapWithQuad(&edgeEndpoint, &cylinderTransform->position, quad, output);

    if (otherEdgesToCheck == POINT_NO_OVERLAP) {
        return edgesToCheck;
    }
    edgesToCheck |= otherEdgesToCheck;

    vector3Negate(&rotatedCrossAxis, &rotatedCrossAxis);
    vector3Add(&centerPoint, &rotatedCrossAxis, &edgeEndpoint);
    otherEdgesToCheck = _collisionPointCheckOverlapWithQuad(&edgeEndpoint, &cylinderTransform->position, quad, output);

    if (otherEdgesToCheck == POINT_NO_OVERLAP) {
        return edgesToCheck;
    }
    edgesToCheck |= otherEdgesToCheck;

    vector3Negate(crossAxis, crossAxis);
    vector3Add(&centerPoint, crossAxis, &edgeEndpoint);
    otherEdgesToCheck = _collisionPointCheckOverlapWithQuad(&edgeEndpoint, &cylinderTransform->position, quad, output);

    if (otherEdgesToCheck != POINT_NO_OVERLAP) {
        edgesToCheck |= otherEdgesToCheck;
    }

    return edgesToCheck;
}

int collisionCylinderCollideQuad(void* data, struct Transform* cylinderTransform, struct CollisionQuad* quad, struct ContactConstraintState* output) {
    struct Vector3 centerAxis;
    quatMultVector(&cylinderTransform->rotation, &gUp, &centerAxis);

    float normalDotProduct = vector3Dot(&centerAxis, &quad->plane.normal);

    struct Vector3 capCenterTowardsPlane;
    vector3AddScaled(&quad->plane.normal, &centerAxis, -normalDotProduct, &capCenterTowardsPlane);

    float magSqrd = vector3MagSqrd(&capCenterTowardsPlane);

    int edgesToCheck;

    struct CollisionCylinder* cylinder = (struct CollisionCylinder*)data;

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
        }
        edgesToCheck = _collisionCylinderPerpendicular(cylinder, cylinderTransform, &centerAxis, &capCenterTowardsPlane, normalDotProduct, quad, output);
    } else {
        vector3Scale(&capCenterTowardsPlane, &capCenterTowardsPlane, cylinder->radius / sqrtf(magSqrd));
        edgesToCheck = _collisionCylinderParallel(cylinder, cylinderTransform, &centerAxis, &capCenterTowardsPlane, normalDotProduct, quad, output);
    }

    if (!edgesToCheck) {
        return output->contactCount > 0;
    }

    // TODO check edges and points of quad

    return 0;
}