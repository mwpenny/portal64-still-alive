#include "collision_cylinder.h"

#include "../math/mathf.h"
#include "contact_solver.h"
#include "collision_quad.h"

int _collisionCylinderParallel(struct CollisionCylinder* cylinder, struct Transform* cylinderTransform, struct Vector3* centerAxis, struct Vector3* crossAxis, float normalDotProduct, struct CollisionQuad* quad, struct ContactConstraintState* output) {
    struct Vector3 edgeEndpoint;
    vector3AddScaled(&cylinderTransform->position, centerAxis, normalDotProduct > 0.0f ? -cylinder->halfHeight : cylinder->halfHeight, &edgeEndpoint);
    vector3Add(&edgeEndpoint, crossAxis, &edgeEndpoint);

    float edgeDistance = planePointDistance(&quad->plane, &edgeEndpoint);

    if (edgeDistance > NEGATIVE_PENETRATION_BIAS) {
        return 0;
    }

    int edgesToCheck = collisionQuadDetermineEdges(&edgeEndpoint, quad);

    output->contactCount = 0;

    if (!edgesToCheck) {
        collisionQuadInitializeNormalContact(quad, output);

        struct ContactState* contact = &output->contacts[output->contactCount];

        ++output->contactCount;

        vector3Sub(&edgeEndpoint, &cylinderTransform->position, &contact->rb);
        vector3AddScaled(&edgeEndpoint, &quad->plane.normal, -edgeDistance, &contact->ra);

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

    vector3AddScaled(&edgeEndpoint, centerAxis,  (normalDotProduct > 0.0f ? 2.0f : -2.0f) * cylinder->halfHeight, &edgeEndpoint);
    edgeDistance = planePointDistance(&quad->plane, &edgeEndpoint);

    if (edgeDistance > NEGATIVE_PENETRATION_BIAS) {
        return edgesToCheck;
    }

    int otherEdgesToCheck = collisionQuadDetermineEdges(&edgeEndpoint, quad);

    if (!otherEdgesToCheck) {
        if (output->contactCount == 0) {
            collisionQuadInitializeNormalContact(quad, output);
        }
        
        struct ContactState* contact = &output->contacts[output->contactCount];

        ++output->contactCount;

        vector3Sub(&edgeEndpoint, &cylinderTransform->position, &contact->rb);
        vector3AddScaled(&edgeEndpoint, &quad->plane.normal, -edgeDistance, &contact->ra);

        contact->id = 1;
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

int collisionCylinderCollideQuad(void* data, struct Transform* cylinderTransform, struct CollisionQuad* quad, struct ContactConstraintState* output) {
    struct Vector3 centerAxis;
    quatMultVector(&cylinderTransform->rotation, &gUp, &centerAxis);

    float normalDotProduct = vector3Dot(&centerAxis, &quad->plane.normal);

    struct Vector3 capCenterTowardsPlane;
    vector3AddScaled(&quad->plane.normal, &centerAxis, -normalDotProduct, &capCenterTowardsPlane);

    float magSqrd = vector3MagSqrd(&capCenterTowardsPlane);

    int edgesToCheck;

    struct CollisionCylinder* cylinder = (struct CollisionCylinder*)data;

    if (fabsf(magSqrd) > 0.7f) {
        // TODO treat as upright
        edgesToCheck = 0;
    } else {
        vector3Scale(&capCenterTowardsPlane, &capCenterTowardsPlane, 1.0f / sqrtf(magSqrd));
        edgesToCheck = _collisionCylinderParallel(cylinder, cylinderTransform, &centerAxis, &capCenterTowardsPlane, normalDotProduct, quad, output);
    }

    if (!edgesToCheck) {
        return output->contactCount > 0;
    }

    // TODO check edges and points of quad

    return 0;
}